#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/unordered_map.hpp"

namespace duckdb {

// Get TCP connection status grouped by IP.
unordered_map<string, int> GetTcpConnectionStatus();

} // namespace duckdb
