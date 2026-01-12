#include "io_operation.hpp"

namespace duckdb {

const std::array<const char *, kIoOperationCount> OPER_NAMES = {"open", "read",          "write",     "list",
                                                                "glob", "get_file_size", "file_sync", "remove_file"};

} // namespace duckdb
