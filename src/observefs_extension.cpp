#define DUCKDB_EXTENSION_MAIN

#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/opener_file_system.hpp"
#include "duckdb/common/string_util.hpp"
#include "fake_filesystem.hpp"
#include "filesystem_ref_registry.hpp"
#include "hffs.hpp"
#include "httpfs_extension.hpp"
#include "observefs_extension.hpp"
#include "observability_filesystem.hpp"
#include "s3fs.hpp"

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

static void LoadInternal(ExtensionLoader &loader) {
	ObservabilityFsRefRegistry::Get().Reset();

	// Register filesystem instance to instance.
	auto &instance = loader.GetDatabaseInstance();
	auto &fs = instance.GetFileSystem();

	// TODO(hjiang): Register a fake filesystem at extension load for testing purpose. This is not ideal since
	// additional necessary instance is shipped in the extension. Local filesystem is not viable because it's not
	// registered in virtual filesystem. A better approach is find another filesystem not in httpfs extension.
	fs.RegisterSubSystem(make_uniq<CacheHttpfsFakeFileSystem>());

	// By default register all filesystem instances inside of httpfs.
	auto observability_httpfs_filesystem = make_uniq<ObservabilityFileSystem>(make_uniq<HTTPFileSystem>());
	ObservabilityFsRefRegistry::Get().Register(observability_httpfs_filesystem.get());
	fs.RegisterSubSystem(std::move(observability_httpfs_filesystem));

	auto observability_hf_filesystem = make_uniq<ObservabilityFileSystem>(make_uniq<HuggingFaceFileSystem>());
	ObservabilityFsRefRegistry::Get().Register(observability_hf_filesystem.get());
	fs.RegisterSubSystem(std::move(observability_hf_filesystem));

	auto observability_s3_filesystem =
	    make_uniq<ObservabilityFileSystem>(make_uniq<S3FileSystem>(BufferManager::GetBufferManager(instance)));
	ObservabilityFsRefRegistry::Get().Register(observability_s3_filesystem.get());
	fs.RegisterSubSystem(std::move(observability_s3_filesystem));

	// Register observability data cleanup function.
	ScalarFunction clear_cache_function("observefs_clear", /*arguments=*/ {},
	                                    /*return_type=*/LogicalType::BOOLEAN, ClearObservabilityData);
	loader.RegisterFunction(clear_cache_function);

	// Register profile collector metrics.
	// A commonly-used SQL is `COPY (SELECT observefs_get_profile()) TO '/tmp/output.txt';`.
	ScalarFunction get_profile_stats_function("observefs_get_profile", /*arguments=*/ {},
	                                          /*return_type=*/LogicalType::VARCHAR, GetProfileStats);
	loader.RegisterFunction(get_profile_stats_function);

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
