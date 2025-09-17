#define DUCKDB_EXTENSION_MAIN

#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/opener_file_system.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/main/extension_util.hpp"
#include "fake_filesystem.hpp"
#include "filesystem_ref_registry.hpp"
#include "hffs.hpp"
#include "httpfs_extension.hpp"
#include "observefs_extension.hpp"
#include "observability_filesystem.hpp"
#include "s3fs.hpp"

namespace duckdb {

// Current duckdb instance; store globally to retrieve filesystem instance inside of it.
static weak_ptr<DatabaseInstance> duckdb_instance;

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
		latest_stat += cur_filesystem->GetHumanReadableStats();
		latest_stat += "\n";
	}
	result.Reference(Value(std::move(latest_stat)));
}

// Wrap the filesystem with extension observability filesystem.
// Throw exception if the requested filesystem hasn't been registered into duckdb instance.
static void WrapFileSystem(const DataChunk &args, ExpressionState &state, Vector &result) {
	D_ASSERT(args.ColumnCount() == 1);
	const string filesystem_name = args.GetValue(/*col_idx=*/0, /*index=*/0).ToString();

	// duckdb instance has a opener filesystem, which is a wrapper around virtual filesystem.
	auto inst = duckdb_instance.lock();
	if (!inst) {
		throw InternalException("DuckDB instance no longer alive");
	}
	auto &opener_filesystem = inst->GetFileSystem().Cast<OpenerFileSystem>();
	auto &vfs = opener_filesystem.GetFileSystem();
	auto internal_filesystem = vfs.ExtractSubSystem(filesystem_name);
	if (internal_filesystem == nullptr) {
		throw InvalidInputException("Filesystem %s hasn't been registered yet!", filesystem_name);
	}

	auto observability_filesystem = make_uniq<ObservabilityFileSystem>(std::move(internal_filesystem));
	ObservabilityFsRefRegistry::Get().Register(observability_filesystem.get());
	vfs.RegisterSubSystem(std::move(observability_filesystem));

	constexpr bool SUCCESS = true;
	result.Reference(Value(SUCCESS));
}

static void LoadInternal(DatabaseInstance &instance) {
	ObservabilityFsRefRegistry::Get().Reset();

	// Register filesystem instance to instance.
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

	// Register a function to wrap all duckdb-vfs-compatible filesystems.
	//
	// Example usage:
	// D. LOAD azure;
	// -- Wrap filesystem with its name.
	// D. SELECT observefs_wrap_filesystem('AzureBlobStorageFileSystem');
	ScalarFunction wrap_filesystem_function("observefs_wrap_filesystem",
	                                        /*arguments=*/ {LogicalTypeId::VARCHAR},
	                                        /*return_type=*/LogicalTypeId::BOOLEAN, WrapFileSystem);
	ExtensionUtil::RegisterFunction(instance, wrap_filesystem_function);

	// Register observability data cleanup function.
	ScalarFunction clear_cache_function("observefs_clear", /*arguments=*/ {},
	                                    /*return_type=*/LogicalType::BOOLEAN, ClearObservabilityData);
	ExtensionUtil::RegisterFunction(instance, clear_cache_function);

	// Register profile collector metrics.
	// A commonly-used SQL is `COPY (SELECT observefs_get_profile()) TO '/tmp/output.txt';`.
	ScalarFunction get_profile_stats_function("observefs_get_profile", /*arguments=*/ {},
	                                          /*return_type=*/LogicalType::VARCHAR, GetProfileStats);
	ExtensionUtil::RegisterFunction(instance, get_profile_stats_function);
}

void ObservefsExtension::Load(DuckDB &db) {
	duckdb_instance = db.instance;
	LoadInternal(*db.instance);
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

DUCKDB_EXTENSION_API void quack_init(duckdb::DatabaseInstance &db) {
	duckdb::DuckDB db_wrapper(db);
	db_wrapper.LoadExtension<duckdb::ObservefsExtension>();
}

DUCKDB_EXTENSION_API const char *quack_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
