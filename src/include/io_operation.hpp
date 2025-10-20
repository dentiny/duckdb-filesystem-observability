#pragma once

#include <array>
#include <cstdint>

namespace duckdb {

// IO operation types.
//
// TODO(hjiang): Add more IO operations.
enum class IoOperation {
	kOpen = 0,
	kRead = 1,
	kList = 2,
	kGlob = 3,
	kGetFileSize = 4,
	kUnknown = 5,
};

inline constexpr auto kIoOperationCount = static_cast<size_t>(IoOperation::kUnknown);

// Operation names, indexed by operation enums.
inline constexpr std::array<const char *, kIoOperationCount> OPER_NAMES = {"open", "read", "list", "glob",
                                                                           "get_file_size"};

} // namespace duckdb
