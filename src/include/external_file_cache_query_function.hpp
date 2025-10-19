#pragma once

#include "duckdb/function/table_function.hpp"

namespace duckdb {

// Table function to get external file cache access records.
TableFunction ExternalFileCacheAccessQueryFunc();

} // namespace duckdb
