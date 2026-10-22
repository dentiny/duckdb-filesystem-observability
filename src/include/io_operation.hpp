// Necessary changes to add a new IO operation:
// 1. Add new IO operations to [`IoOperation`] enum class
// 2. Add estimated latency to [`kLatencyHeuristics`]

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace duckdb {

// IO operation types.
//
// TODO(hjiang): Add more IO operations.
enum class IoOperation {
	kOpen = 0,
	kRead = 1,
	kWrite = 2,
	kList = 3,
	kGlob = 4,
	kStats = 5,
	kFileSync = 6,
	kRemoveFile = 7,
	kUnknown = 8,
};

constexpr size_t kIoOperationCount = static_cast<size_t>(IoOperation::kUnknown);

// Operation names, indexed by operation enums.
inline constexpr std::array<std::string_view, kIoOperationCount> OPER_NAMES = {
    "open", "read", "write", "list", "glob", "get_file_size", "file_sync", "remove_file"};

} // namespace duckdb
