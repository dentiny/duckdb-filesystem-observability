#include "observefs_functions.hpp"

#include "duckdb/common/opener_file_system.hpp"
#include "duckdb/common/types/data_chunk.hpp"
#include "duckdb/common/types/value.hpp"
#include "duckdb/common/vector.hpp"
#include "duckdb/execution/expression_executor_state.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension/extension_loader.hpp"
#include "duckdb/parser/parsed_data/create_scalar_function_info.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "external_file_cache_query_function.hpp"
#include "external_file_cache_stats_recorder.hpp"
#include "filesystem_status_query_function.hpp"
#include "observefs_instance_state.hpp"
#include "observability_filesystem.hpp"

namespace duckdb {

namespace {

constexpr bool SUCCESS = true;

DatabaseInstance &GetDatabaseInstance(ExpressionState &state) {
	auto *executor = state.root.executor;
	auto &client_context = executor->GetContext();
	return *client_context.db.get();
}

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

void WrapFileSystem(const DataChunk &args, ExpressionState &state, Vector &result) {
	D_ASSERT(args.ColumnCount() == 1);
	const string filesystem_name = args.GetValue(/*col_idx=*/0, /*index=*/0).ToString();

	auto &duckdb_instance = GetDatabaseInstance(state);
	auto &opener_filesystem = duckdb_instance.GetFileSystem().Cast<OpenerFileSystem>();
	auto &vfs = opener_filesystem.GetFileSystem();
	auto internal_filesystem = vfs.ExtractSubSystem(filesystem_name);
	if (internal_filesystem == nullptr) {
		throw InvalidInputException("Filesystem %s hasn't been registered yet!", filesystem_name);
	}

	auto observe_filesystem = make_uniq<ObservabilityFileSystem>(std::move(internal_filesystem), vfs);
	auto &instance_state = GetInstanceStateOrThrow(duckdb_instance);
	instance_state.registry.Register(observe_filesystem.get());
	vfs.RegisterSubSystem(std::move(observe_filesystem));

	result.Reference(Value(SUCCESS));
}

void ClearExternalFileCacheStatsRecord(DataChunk &args, ExpressionState &state, Vector &result) {
	GetExternalFileCacheStatsRecorder().ClearCacheAccessRecord();
	result.Reference(Value(SUCCESS));
}

FunctionDescription GetFunctionDescription(vector<LogicalType> parameter_types, vector<string> parameter_names,
                                           string description, string example) {
	FunctionDescription result;
	result.parameter_types = std::move(parameter_types);
	result.parameter_names = std::move(parameter_names);
	result.description = std::move(description);
	result.examples.emplace_back(std::move(example));
	result.categories = {"filesystem", "observability"};
	return result;
}

void RegisterScalarFunction(ExtensionLoader &loader, ScalarFunction function, FunctionDescription description) {
	CreateScalarFunctionInfo info(std::move(function));
	info.descriptions.emplace_back(std::move(description));
	loader.RegisterFunction(std::move(info));
}

void RegisterTableFunction(ExtensionLoader &loader, TableFunction function, FunctionDescription description) {
	CreateTableFunctionInfo info(std::move(function));
	info.descriptions.emplace_back(std::move(description));
	loader.RegisterFunction(std::move(info));
}

} // namespace

void RegisterObservefsFunctions(ExtensionLoader &loader) {
	RegisterScalarFunction(
	    loader, ScalarFunction("observefs_clear", {}, LogicalType {LogicalTypeId::BOOLEAN}, ClearObservabilityData),
	    GetFunctionDescription({}, {}, "Clears collected I/O observability metrics for all wrapped filesystems.",
	                           "observefs_clear()"));

	RegisterScalarFunction(
	    loader, ScalarFunction("observefs_get_profile", {}, LogicalType {LogicalTypeId::VARCHAR}, GetProfileStats),
	    GetFunctionDescription({}, {}, "Returns a human-readable profile of I/O metrics for wrapped filesystems.",
	                           "observefs_get_profile()"));

	RegisterTableFunction(loader, ListRegisteredFileSystemsQueryFunc(),
	                      GetFunctionDescription({}, {}, "Lists the filesystem implementations registered with DuckDB.",
	                                             "observefs_list_registered_filesystems()"));

	RegisterScalarFunction(
	    loader,
	    ScalarFunction("observefs_wrap_filesystem", {LogicalTypeId::VARCHAR}, LogicalTypeId::BOOLEAN, WrapFileSystem),
	    GetFunctionDescription({LogicalTypeId::VARCHAR}, {"filesystem_name"},
	                           "Wraps a registered filesystem with I/O observability instrumentation.",
	                           "observefs_wrap_filesystem('AzureBlobStorageFileSystem')"));

	RegisterScalarFunction(loader,
	                       ScalarFunction("observefs_clear_external_file_cache_access_record", {},
	                                      LogicalTypeId::BOOLEAN, ClearExternalFileCacheStatsRecord),
	                       GetFunctionDescription({}, {},
	                                              "Clears the recorded external file cache hit and miss counts.",
	                                              "observefs_clear_external_file_cache_access_record()"));

	RegisterTableFunction(
	    loader, ExternalFileCacheAccessQueryFunc(),
	    GetFunctionDescription({}, {}, "Returns recorded external file cache hit, miss, and partial-hit counts.",
	                           "observefs_external_file_cache_access_record()"));
}

} // namespace duckdb
