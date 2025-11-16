// Necessary changes to add a new IO operation:
// 1. Add new IO operations to [`IoOperation`] enum class
// 2. Add estimated latency to [`kLatencyHeuristics`]

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

extern constexpr size_t kIoOperationCount;

// Operation names, indexed by operation enums.
extern const std::array<const char *, static_cast<size_t>(IoOperation::kUnknown)> OPER_NAMES;

} // namespace duckdb
