// Function which get TCP connection status.

#pragma once

#include "duckdb/function/table_function.hpp"

namespace duckdb {

// Get the table function to get system TCP connection status.
TableFunction GetTcpConnectionStatusFunc();

} // namespace duckdb
