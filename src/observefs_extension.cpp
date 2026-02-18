#define DUCKDB_EXTENSION_MAIN

#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/helper.hpp"
#include "duckdb/common/opener_file_system.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/common/unique_ptr.hpp"
#include "duckdb/storage/external_file_cache.hpp"
#include "external_file_cache_query_function.hpp"
#include "external_file_cache_stats_recorder.hpp"
#include "fake_filesystem.hpp"
#include "filesystem_ref_registry.hpp"
#include "filesystem_status_query_function.hpp"
#include "hffs.hpp"
#include "httpfs_extension.hpp"
#include "observefs_extension.hpp"
#include "observefs_instance_state.hpp"
#include "observability_filesystem.hpp"
#include "s3fs.hpp"

namespace duckdb {

namespace {

// "httpfs" extension name.
constexpr const char *HTTPFS_EXTENSION = "httpfs";
// Indicates successful query.
constexpr bool SUCCESS = true;

// Get database instance from expression state.
// Returned instance ownership lies in the given [`state`].
DatabaseInstance &GetDatabaseInstance(ExpressionState &state) {
	auto *executor = state.root.executor;
	auto &client_context = executor->GetContext();
	return *client_context.db.get();
}

// Clear observability data for all filesystems.
void ClearObservabilityData(const DataChunk &args, ExpressionState &state, Vector &result) {
	auto &duckdb_instance = GetDatabaseInstance(state);
	auto &instance_state = GetInstanceStateOrThrow(duckdb_instance);
	auto observefs_instances = instance_state.registry.GetAllObservabilityFs();
	for (auto *cur_fs : observefs_instances) {
		cur_fs->ClearObservabilityData();
	}

	result.Reference(Value(SUCCESS));
}

void GetProfileStats(const DataChunk &args, ExpressionState &state, Vector &result) {
	string latest_stat;
	auto &duckdb_instance = GetDatabaseInstance(state);
	auto &instance_state = GetInstanceStateOrThrow(duckdb_instance);
	const auto &observefs_instances = instance_state.registry.GetAllObservabilityFs();
	for (auto *cur_filesystem : observefs_instances) {
		latest_stat += StringUtil::Format("Current filesystem: %s\n", cur_filesystem->GetName());
		const auto cur_stats_str = cur_filesystem->GetHumanReadableStats();
		if (cur_stats_str.empty()) {
			latest_stat += "No interested IO operations issued.";
		} else {
			latest_stat += cur_stats_str;
		}
		latest_stat += "\n";
	}
	result.Reference(Value(std::move(latest_stat)));
}

// Wrap the filesystem with extension cache filesystem.
// Throw exception if the requested filesystem hasn't been registered into duckdb instance.
void WrapFileSystem(const DataChunk &args, ExpressionState &state, Vector &result) {
	D_ASSERT(args.ColumnCount() == 1);
	const string filesystem_name = args.GetValue(/*col_idx=*/0, /*index=*/0).ToString();

	// duckdb instance has a opener filesystem, which is a wrapper around virtual filesystem.
	auto &duckdb_instance = GetDatabaseInstance(state);
	auto &opener_filesystem = duckdb_instance.GetFileSystem().Cast<OpenerFileSystem>();
	auto &vfs = opener_filesystem.GetFileSystem();
	auto internal_filesystem = vfs.ExtractSubSystem(filesystem_name);
	if (internal_filesystem == nullptr) {
		throw InvalidInputException("Filesystem %s hasn't been registered yet!", filesystem_name);
	}

	auto observe_filesystem = make_uniq<ObservabilityFileSystem>(std::move(internal_filesystem));
	auto &instance_state = GetInstanceStateOrThrow(duckdb_instance);
	instance_state.registry.Register(observe_filesystem.get());
	vfs.RegisterSubSystem(std::move(observe_filesystem));

	result.Reference(Value(SUCCESS));
}

void ClearExternalFileCacheStatsRecord(DataChunk &args, ExpressionState &state, Vector &result) {
	GetExternalFileCacheStatsRecorder().ClearCacheAccessRecord();
	result.Reference(Value(SUCCESS));
}

// Extract or get httpfs filesystem.
unique_ptr<FileSystem> ExtractOrCreateHttpfs(FileSystem &vfs) {
	auto filesystems = vfs.ListSubSystems();
	auto iter = std::find_if(filesystems.begin(), filesystems.end(), [](const auto &cur_fs_name) {
		// Wrapped filesystem made by extensions could ends with httpfs filesystem.
		return StringUtil::EndsWith(cur_fs_name, "HTTPFileSystem");
	});
	if (iter == filesystems.end()) {
		return make_uniq<HTTPFileSystem>();
	}
	auto httpfs = vfs.ExtractSubSystem(*iter);
	D_ASSERT(httpfs != nullptr);
	return httpfs;
}

// Extract or get hugging filesystem.
unique_ptr<FileSystem> ExtractOrCreateHuggingfs(FileSystem &vfs) {
	auto filesystems = vfs.ListSubSystems();
	auto iter = std::find_if(filesystems.begin(), filesystems.end(), [](const auto &cur_fs_name) {
		// Wrapped filesystem made by extensions could ends with httpfs filesystem.
		return StringUtil::EndsWith(cur_fs_name, "HuggingFaceFileSystem");
	});
	if (iter == filesystems.end()) {
		return make_uniq<HuggingFaceFileSystem>();
	}
	auto hf_fs = vfs.ExtractSubSystem(*iter);
	D_ASSERT(hf_fs != nullptr);
	return hf_fs;
}

// Extract or get s3 filesystem.
unique_ptr<FileSystem> ExtractOrCreateS3fs(FileSystem &vfs, DatabaseInstance &instance) {
	auto filesystems = vfs.ListSubSystems();
	auto iter = std::find_if(filesystems.begin(), filesystems.end(), [](const auto &cur_fs_name) {
		// Wrapped filesystem made by extensions could ends with s3 filesystem.
		return StringUtil::EndsWith(cur_fs_name, "S3FileSystem");
	});
	if (iter == filesystems.end()) {
		return make_uniq<S3FileSystem>(BufferManager::GetBufferManager(instance));
	}
	auto s3_fs = vfs.ExtractSubSystem(*iter);
	D_ASSERT(s3_fs != nullptr);
	return s3_fs;
}

// Whether `httpfs` extension has already been loaded.
bool IsHttpfsExtensionLoaded(DatabaseInstance &db_instance) {
	auto &extension_manager = db_instance.GetExtensionManager();
	const auto loaded_extensions = extension_manager.GetExtensions();
	return std::find(loaded_extensions.begin(), loaded_extensions.end(), HTTPFS_EXTENSION) != loaded_extensions.end();
}

// Ensure httpfs extension is loaded, loading it if necessary
void EnsureHttpfsExtensionLoaded(ExtensionLoader &loader, DatabaseInstance &instance) {
	const bool httpfs_extension_loaded = IsHttpfsExtensionLoaded(instance);
	if (httpfs_extension_loaded) {
		return;
	}
	auto httpfs_extension = make_uniq<HttpfsExtension>();
	httpfs_extension->Load(loader);

	// Register into extension manager to keep compatibility as httpfs.
	auto &extension_manager = ExtensionManager::Get(instance);
	auto extension_active_load = extension_manager.BeginLoad(HTTPFS_EXTENSION);
	// Manually fill in the extension install info to finalize extension load.
	ExtensionInstallInfo extension_install_info;
	extension_install_info.mode = ExtensionInstallMode::UNKNOWN;
	extension_active_load->FinishLoad(extension_install_info);
}

void LoadInternal(ExtensionLoader &loader) {
	// Register filesystem instance to instance.
	auto &duckdb_instance = loader.GetDatabaseInstance();
	auto &opener_filesystem = duckdb_instance.GetFileSystem().Cast<OpenerFileSystem>();
	auto &vfs = opener_filesystem.GetFileSystem();

	// To achieve full compatibility for duckdb-httpfs extension, all related functions/types/... should be supported,
	// so we load it first if not already loaded.
	EnsureHttpfsExtensionLoaded(loader, duckdb_instance);

	auto instance_state = make_shared_ptr<ObservefsInstanceState>();
	SetInstanceState(duckdb_instance, instance_state);

	auto &external_file_cache = ExternalFileCache::Get(duckdb_instance);
	InitOrResetExternalFileCache(external_file_cache);

	// TODO(hjiang): Register a fake filesystem at extension load for testing purpose. This is not ideal since
	// additional necessary instance is shipped in the extension. Local filesystem is not viable because it's not
	// registered in virtual filesystem. A better approach is find another filesystem not in httpfs extension.
	vfs.RegisterSubSystem(make_uniq<ObserveHttpfsFakeFileSystem>());

	// By default register all filesystem instances inside of httpfs.
	//
	// Register http filesystem.
	auto http_fs = ExtractOrCreateHttpfs(vfs);
	auto observability_httpfs_filesystem = make_uniq<ObservabilityFileSystem>(std::move(http_fs));
	instance_state->registry.Register(observability_httpfs_filesystem.get());
	vfs.RegisterSubSystem(std::move(observability_httpfs_filesystem));

	// Register hugging filesystem.
	auto hf_fs = ExtractOrCreateHuggingfs(vfs);
	auto observability_hf_filesystem = make_uniq<ObservabilityFileSystem>(std::move(hf_fs));
	instance_state->registry.Register(observability_hf_filesystem.get());
	vfs.RegisterSubSystem(std::move(observability_hf_filesystem));

	// Register s3 filesystem.
	auto s3_fs = ExtractOrCreateS3fs(vfs, duckdb_instance);
	auto observability_s3_filesystem = make_uniq<ObservabilityFileSystem>(std::move(s3_fs));
	instance_state->registry.Register(observability_s3_filesystem.get());
	vfs.RegisterSubSystem(std::move(observability_s3_filesystem));
	auto &config = DBConfig::GetConfig(duckdb_instance);

	auto enable_external_file_cache_stats_callback = [](ClientContext &context, SetScope scope, Value &parameter) {
		const auto to_enable = parameter.GetValue<bool>();
		if (to_enable) {
			GetExternalFileCacheStatsRecorder().Enable();
		} else {
			GetExternalFileCacheStatsRecorder().Disable();
		}
	};
	config.AddExtensionOption(
	    "observefs_enable_external_file_cache_stats", "Whether to enable stats record for external file cache.",
	    LogicalType {LogicalTypeId::BOOLEAN}, true, std::move(enable_external_file_cache_stats_callback));

	// Register observability data cleanup function.
	ScalarFunction clear_cache_function("observefs_clear", /*arguments=*/ {},
	                                    /*return_type=*/LogicalType {LogicalTypeId::BOOLEAN}, ClearObservabilityData);
	loader.RegisterFunction(clear_cache_function);

	// Register profile collector metrics.
	// A commonly-used SQL is `COPY (SELECT observefs_get_profile()) TO '/tmp/output.txt';`.
	ScalarFunction get_profile_stats_function("observefs_get_profile", /*arguments=*/ {},
	                                          /*return_type=*/LogicalType {LogicalTypeId::VARCHAR}, GetProfileStats);
	loader.RegisterFunction(get_profile_stats_function);

	// Register a function to list all existing filesystem instances, which is useful for wrapping.
	loader.RegisterFunction(ListRegisteredFileSystemsQueryFunc());

	// Register a function to wrap all duckdb-vfs-compatible filesystems. By default only httpfs filesystem instances
	// are wrapped. Usage for the target filesystem can be used as normal.
	//
	// Example usage:
	// D. LOAD azure;
	// -- Wrap filesystem with its name.
	// D. SELECT observefs_wrap_filesystem('AzureBlobStorageFileSystem');
	ScalarFunction wrap_cache_filesystem_function("observefs_wrap_filesystem",
	                                              /*arguments=*/ {LogicalTypeId::VARCHAR},
	                                              /*return_type=*/LogicalTypeId::BOOLEAN, WrapFileSystem);
	loader.RegisterFunction(wrap_cache_filesystem_function);

	// Register a function to clear external file cache stats record.
	ScalarFunction clear_external_file_cache_access_record_function("observefs_clear_external_file_cache_access_record",
	                                                                /*arguments=*/ {},
	                                                                /*return_type=*/LogicalTypeId::BOOLEAN,
	                                                                ClearExternalFileCacheStatsRecord);
	loader.RegisterFunction(clear_external_file_cache_access_record_function);

	// Register external file cache access query function.
	loader.RegisterFunction(ExternalFileCacheAccessQueryFunc());
}

} // namespace

void ObservefsExtension::Load(ExtensionLoader &loader) {
	LoadInternal(loader);
}
string ObservefsExtension::Name() {
	return "observefs";
}

string ObservefsExtension::Version() const {
#ifdef EXT_VERSION_QUACK
	return EXT_VERSION_QUACK;
#else
	return "";
#endif
}

} // namespace duckdb

extern "C" {
DUCKDB_CPP_EXTENSION_ENTRY(observefs, loader) {
	duckdb::ObservefsExtension().Load(loader);
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
