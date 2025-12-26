// Necessary changes to add a new IO operation:
// 1. Add new IO operations to [`IoOperation`] enum class
// 2. Add estimated latency to [`kLatencyHeuristics`]

#pragma once

#include <array>
#include <cstddef>
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

constexpr size_t kIoOperationCount = static_cast<size_t>(IoOperation::kUnknown);

// Operation names, indexed by operation enums.
extern const std::array<const char *, kIoOperationCount> OPER_NAMES;

} // namespace duckdb
