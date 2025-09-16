#define DUCKDB_EXTENSION_MAIN

#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/opener_file_system.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/main/extension_util.hpp"
#include "observabilityfs_extension.hpp"
#include "observability_filesystem.hpp"

namespace duckdb {

// Current duckdb instance; store globally to retrieve filesystem instance inside of it.
static weak_ptr<DatabaseInstance> duckdb_instance;

// Wrap the filesystem with extension cache filesystem.
// Throw exception if the requested filesystem hasn't been registered into duckdb instance.
static void WrapCacheFileSystem(const DataChunk &args, ExpressionState &state, Vector &result) {
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
	vfs.RegisterSubSystem(std::move(observability_filesystem));

	constexpr bool SUCCESS = true;
	result.Reference(Value(SUCCESS));
}

static void LoadInternal(DatabaseInstance &instance) {
	// Register a function to wrap all duckdb-vfs-compatible filesystems. By default only httpfs filesystem instances
	// are wrapped. Usage for the target filesystem can be used as normal.
	//
	// Example usage:
	// D. LOAD azure;
	// -- Wrap filesystem with its name.
	// D. SELECT observefs_wrap_cache_filesystem('AzureBlobStorageFileSystem');
	ScalarFunction wrap_cache_filesystem_function("observabilityfs_wrap_cache_filesystem",
	                                              /*arguments=*/ {LogicalTypeId::VARCHAR},
	                                              /*return_type=*/LogicalTypeId::BOOLEAN, WrapCacheFileSystem);
	ExtensionUtil::RegisterFunction(instance, wrap_cache_filesystem_function);
}

void ObservabilityfsExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string ObservabilityfsExtension::Name() {
	return "observabilityfs";
}

std::string ObservabilityfsExtension::Version() const {
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
	db_wrapper.LoadExtension<duckdb::ObservabilityfsExtension>();
}

DUCKDB_EXTENSION_API const char *quack_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
