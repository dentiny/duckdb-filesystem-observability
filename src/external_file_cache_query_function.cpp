#include "external_file_cache_query_function.hpp"

#include "external_file_cache_stats_recorder.hpp"

namespace duckdb {

namespace {

struct CacheAccessData : public GlobalTableFunctionState {
	CacheAccessRecord cache_access_record;

	// Used to decide whether to emit.
	bool emitted = false;
};

unique_ptr<FunctionData> ExternalFileCacheAccessBind(ClientContext &context, TableFunctionBindInput &input,
                                                     vector<LogicalType> &return_types, vector<string> &names) {
	D_ASSERT(return_types.empty());
	D_ASSERT(names.empty());

	return_types.reserve(3);
	names.reserve(3);

	return_types.emplace_back(LogicalType::UBIGINT);
	names.emplace_back("cache hit count");

	return_types.emplace_back(LogicalType::UBIGINT);
	names.emplace_back("cache miss count");

	return_types.emplace_back(LogicalType::UBIGINT);
	names.emplace_back("cache partial hit");

	return nullptr;
}

unique_ptr<GlobalTableFunctionState> ExternalFileCacheAccessInit(ClientContext &context,
                                                                 TableFunctionInitInput &input) {
	auto result = make_uniq<CacheAccessData>();
	auto &external_file_cache_stats_recorder = GetExternalFileCacheStatsRecorder();
	result->cache_access_record = external_file_cache_stats_recorder.GetCacheAccessRecord();
	return std::move(result);
}

void ExternalFileCacheAccessFunc(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	auto &data = data_p.global_state->Cast<CacheAccessData>();

	// All entries have been emitted.
	if (data.emitted) {
		return;
	}
	data.emitted = true;

	// Start filling in the result buffer.
	output.SetValue(/*col_idx=*/0, /*index=*/0, Value::UBIGINT(data.cache_access_record.hit_count));
	output.SetValue(/*col_idx=*/1, /*index=*/0, Value::UBIGINT(data.cache_access_record.miss_count));
	output.SetValue(/*col_idx=*/2, /*index=*/0, Value::UBIGINT(data.cache_access_record.partial_hit_count));
	output.SetCardinality(/*count=*/1);
}
} // namespace

TableFunction ExternalFileCacheAccessQueryFunc() {
	TableFunction external_file_cache_access_query_func {/*name=*/"observefs_external_file_cache_access_record",
	                                                     /*arguments=*/ {},
	                                                     /*function=*/ExternalFileCacheAccessFunc,
	                                                     /*bind=*/ExternalFileCacheAccessBind,
	                                                     /*init_global=*/ExternalFileCacheAccessInit};
	return external_file_cache_access_query_func;
}

} // namespace duckdb
