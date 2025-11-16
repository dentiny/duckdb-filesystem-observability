#include "include/io_operation.hpp"

namespace duckdb {

const std::array<const char *, kIoOperationCount> OPER_NAMES = {"open", "read", "list", "glob", "get_file_size"};

} // namespace duckdb
