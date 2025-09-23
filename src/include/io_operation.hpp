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
	kUnknown = 3,
};

inline constexpr auto kIoOperationCount = static_cast<size_t>(IoOperation::kUnknown);

// Operation names, indexed by operation enums.
inline constexpr std::array<const char *, kIoOperationCount> OPER_NAMES = {"open", "read", "list"};

} // namespace duckdb
