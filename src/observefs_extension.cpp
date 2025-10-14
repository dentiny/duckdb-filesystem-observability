#define DUCKDB_EXTENSION_MAIN

#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/opener_file_system.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/common/unique_ptr.hpp"
#include "fake_filesystem.hpp"
#include "filesystem_ref_registry.hpp"
#include "hffs.hpp"
#include "httpfs_extension.hpp"
#include "observefs_extension.hpp"
#include "observability_filesystem.hpp"
#include "s3fs.hpp"
#include "tcp_connection_query_function.hpp"

namespace duckdb {

// Get database instance from expression state.
// Returned instance ownership lies in the given [`state`].
static DatabaseInstance &GetDatabaseInstance(ExpressionState &state) {
	auto *executor = state.root.executor;
	auto &client_context = executor->GetContext();
	return *client_context.db.get();
}

// Clear observability data for all filesystems.
static void ClearObservabilityData(const DataChunk &args, ExpressionState &state, Vector &result) {
	auto observefs_instances = ObservabilityFsRefRegistry::Get().GetAllObservabilityFs();
	for (auto *cur_fs : observefs_instances) {
		cur_fs->ClearObservabilityData();
	}

	constexpr bool SUCCESS = true;
	result.Reference(Value(SUCCESS));
}

static void GetProfileStats(const DataChunk &args, ExpressionState &state, Vector &result) {
	string latest_stat;
	const auto &observefs_instances = ObservabilityFsRefRegistry::Get().GetAllObservabilityFs();
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
static void WrapFileSystem(const DataChunk &args, ExpressionState &state, Vector &result) {
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
	ObservabilityFsRefRegistry::Get().Register(observe_filesystem.get());
	vfs.RegisterSubSystem(std::move(observe_filesystem));

	constexpr bool SUCCESS = true;
	result.Reference(Value(SUCCESS));
}

// List all registered function for the database instance.
static void ListRegisteredFileSystems(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &result_entries = ListVector::GetEntry(result);

	// duckdb instance has a opener filesystem, which is a wrapper around virtual filesystem.
	auto &duckdb_instance = GetDatabaseInstance(state);
	auto &opener_filesystem = duckdb_instance.GetFileSystem().Cast<OpenerFileSystem>();
	auto &vfs = opener_filesystem.GetFileSystem();
	auto filesystems = vfs.ListSubSystems();
	std::sort(filesystems.begin(), filesystems.end());

	// Set filesystem instances.
	ListVector::Reserve(result, filesystems.size());
	ListVector::SetListSize(result, filesystems.size());
	auto data = FlatVector::GetData<string_t>(result_entries);
	for (int idx = 0; idx < filesystems.size(); ++idx) {
		data[idx] = StringVector::AddString(result_entries, std::move(filesystems[idx]));
	}

	// Define the list element (offset + length)
	auto list_data = FlatVector::GetData<list_entry_t>(result);
	list_data[0].offset = 0;
	list_data[0].length = filesystems.size();

	// Set result as valid.
	FlatVector::SetValidity(result, ValidityMask(filesystems.size()));
}

// Extract or get httpfs filesystem.
static unique_ptr<FileSystem> ExtractOrCreateHttpfs(FileSystem &vfs) {
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
static unique_ptr<FileSystem> ExtractOrCreateHuggingfs(FileSystem &vfs) {
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
static unique_ptr<FileSystem> ExtractOrCreateS3fs(FileSystem &vfs, DatabaseInstance &instance) {
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

static void LoadInternal(ExtensionLoader &loader) {
	ObservabilityFsRefRegistry::Get().Reset();

	// Register filesystem instance to instance.
	auto &duckdb_instance = loader.GetDatabaseInstance();
	auto &opener_filesystem = duckdb_instance.GetFileSystem().Cast<OpenerFileSystem>();
	auto &vfs = opener_filesystem.GetFileSystem();

	// TODO(hjiang): Register a fake filesystem at extension load for testing purpose. This is not ideal since
	// additional necessary instance is shipped in the extension. Local filesystem is not viable because it's not
	// registered in virtual filesystem. A better approach is find another filesystem not in httpfs extension.
	vfs.RegisterSubSystem(make_uniq<ObserveHttpfsFakeFileSystem>());

	// By default register all filesystem instances inside of httpfs.
	//
	// Register http filesystem.
	auto http_fs = ExtractOrCreateHttpfs(vfs);
	auto observability_httpfs_filesystem = make_uniq<ObservabilityFileSystem>(std::move(http_fs));
	ObservabilityFsRefRegistry::Get().Register(observability_httpfs_filesystem.get());
	vfs.RegisterSubSystem(std::move(observability_httpfs_filesystem));

	// Register hugging filesystem.
	auto hf_fs = ExtractOrCreateHuggingfs(vfs);
	auto observability_hf_filesystem = make_uniq<ObservabilityFileSystem>(std::move(hf_fs));
	ObservabilityFsRefRegistry::Get().Register(observability_hf_filesystem.get());
	vfs.RegisterSubSystem(std::move(observability_hf_filesystem));

	// Register s3 filesystem.
	auto s3_fs = ExtractOrCreateS3fs(vfs, duckdb_instance);
	auto observability_s3_filesystem = make_uniq<ObservabilityFileSystem>(std::move(s3_fs));
	ObservabilityFsRefRegistry::Get().Register(observability_s3_filesystem.get());
	vfs.RegisterSubSystem(std::move(observability_s3_filesystem));

	// Register observability data cleanup function.
	ScalarFunction clear_cache_function("observefs_clear", /*arguments=*/ {},
	                                    /*return_type=*/LogicalType::BOOLEAN, ClearObservabilityData);
	loader.RegisterFunction(clear_cache_function);

	// Register profile collector metrics.
	// A commonly-used SQL is `COPY (SELECT observefs_get_profile()) TO '/tmp/output.txt';`.
	ScalarFunction get_profile_stats_function("observefs_get_profile", /*arguments=*/ {},
	                                          /*return_type=*/LogicalType::VARCHAR, GetProfileStats);
	loader.RegisterFunction(get_profile_stats_function);

	// Register a function to list all existing filesystem instances, which is useful for wrapping.
	ScalarFunction list_registered_filesystem_function("observefs_list_registered_filesystems",
	                                                   /*arguments=*/ {},
	                                                   /*return_type=*/LogicalType::LIST(LogicalType::VARCHAR),
	                                                   ListRegisteredFileSystems);
	loader.RegisterFunction(list_registered_filesystem_function);

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

	// Register TCP connection status function.
	// WARNING: It works only on linux platform, it displays nothing on MacOs.
	loader.RegisterFunction(GetTcpConnectionNumFunc());
}

void ObservefsExtension::Load(ExtensionLoader &loader) {
	// To achieve full compatibility for duckdb-httpfs extension, all related functions/types/... should be supported,
	// so we load it first.
	httpfs_extension = make_uniq<HttpfsExtension>();
	// It's possible httpfs is already loaded beforehand, simply capture exception and proceed.
	try {
		httpfs_extension->Load(loader);
	} catch (...) {
	}

	LoadInternal(loader);
}
std::string ObservefsExtension::Name() {
	return "observefs";
}

std::string ObservefsExtension::Version() const {
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
