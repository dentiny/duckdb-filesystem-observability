#include "tcp_connection_query_function.hpp"

#include <algorithm>
#include <array>
#include <utility>

#include "duckdb/common/helper.hpp"
#include "duckdb/common/numeric_utils.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/common/unique_ptr.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "duckdb/common/vector.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/main/client_context.hpp"
#include "tcp_connection_fetcher.hpp"

namespace duckdb {

namespace {

//===--------------------------------------------------------------------===//
// Get TCP connection number query function
//===--------------------------------------------------------------------===//

struct TcpConnectionStatus {
	string ip;
	// TCP connection count for the IP.
	int count = 0;
};

struct TcpConnectionData : public GlobalTableFunctionState {
	vector<TcpConnectionStatus> tcp_connection_status;

	// Used to record the progress of emission.
	uint64_t offset = 0;
};

unique_ptr<FunctionData> GetTcpConnectionFuncBind(ClientContext &context, TableFunctionBindInput &input,
                                                  vector<LogicalType> &return_types, vector<string> &names) {
	D_ASSERT(return_types.empty());
	D_ASSERT(names.empty());

	return_types.reserve(2);
	names.reserve(2);

	return_types.emplace_back(LogicalType::VARCHAR);
	return_types.emplace_back(LogicalType::BIGINT);
	names.emplace_back("IP");
	names.emplace_back("connection number");

	return nullptr;
}

unique_ptr<GlobalTableFunctionState> GetTcpConnectionFuncInit(ClientContext &context, TableFunctionInitInput &input) {
	auto result = make_uniq<TcpConnectionData>();
	auto &tcp_connection_status = result->tcp_connection_status;

	auto tcp_connection_map = GetTcpConnectionStatus();
	tcp_connection_status.reserve(tcp_connection_map.size());
	for (const auto &[cur_ip, cur_cnt] : tcp_connection_map) {
		TcpConnectionStatus cur_tcp_conn_status {
		    .ip = cur_ip,
		    .count = cur_cnt,
		};
		tcp_connection_status.emplace_back(std::move(cur_tcp_conn_status));
	}

	return std::move(result);
}

void GetTcpConnectionTableFunc(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	auto &data = data_p.global_state->Cast<TcpConnectionData>();

	// All entries have been emitted.
	if (data.offset >= data.tcp_connection_status.size()) {
		return;
	}

	// Start filling in the result buffer.
	idx_t count = 0;
	while (data.offset < data.tcp_connection_status.size() && count < STANDARD_VECTOR_SIZE) {
		auto &entry = data.tcp_connection_status[data.offset++];
		output.SetValue(/*col_idx=*/0, count, entry.ip);
		output.SetValue(/*col_idx=*/1, count, entry.count);
		count++;
	}
	output.SetCardinality(count);
}
} // namespace

TableFunction GetTcpConnectionStatusFunc() {
	TableFunction get_tcp_conn_query_func {/*name=*/"observefs_get_tcp_connection",
	                                       /*arguments=*/ {},
	                                       /*function=*/GetTcpConnectionTableFunc,
	                                       /*bind=*/GetTcpConnectionFuncBind,
	                                       /*init_global=*/GetTcpConnectionFuncInit};
	return get_tcp_conn_query_func;
}
} // namespace duckdb
