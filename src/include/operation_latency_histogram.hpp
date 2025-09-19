#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>

#include "duckdb/common/helper.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "duckdb/common/vector.hpp"
#include "histogram.hpp"
#include "quantile_estimator.hpp"

namespace duckdb {

// Forward declaration.
class OperationLatencyHistogram;

// IO operation types.
//
// TODO(hjiang): Add more IO operations.
enum class IoOperation {
	kOpen = 0,
	kRead = 1,
	kList = 2,
	kUnknown = 3,
};

// A RAII guard to measure latency for IO operations.
class LatencyGuard {
public:
	LatencyGuard(OperationLatencyHistogram &latency_collector_p, IoOperation io_operation_p);
	~LatencyGuard();

	LatencyGuard(const LatencyGuard &) = delete;
	LatencyGuard &operator=(const LatencyGuard &) = delete;
	LatencyGuard(LatencyGuard &&) = default;
	LatencyGuard &operator=(LatencyGuard &&) = default;

private:
	OperationLatencyHistogram &latency_collector;
	IoOperation io_operation = IoOperation::kUnknown;
	int64_t start_timestamp = 0;
};

class OperationLatencyHistogram {
public:
	OperationLatencyHistogram();
	~OperationLatencyHistogram() = default;

	LatencyGuard RecordOperationStart(IoOperation io_oper);

	// Represent stats in human-readable format.
	std::string GetHumanReadableStats();

private:
	friend class LatencyGuard;

	struct LatencyStatsCollector {
		unique_ptr<Histogram> histogram;
		unique_ptr<QuantileEstimator> quantile_estimator;
	};

	// Mark the end of the a completed IO operation, disregard it's successful or not.
	void RecordOperationEnd(IoOperation io_oper, int64_t latency_millisec);

	static constexpr auto kIoOperationCount = static_cast<size_t>(IoOperation::kUnknown);

	// Operation names, indexed by operation enums.
	inline static const vector<const char *> OPER_NAMES = []() {
		vector<const char *> oper_names;
		oper_names.reserve(kIoOperationCount);
		oper_names.emplace_back("open");
		oper_names.emplace_back("read");
		oper_names.emplace_back("list");
		return oper_names;
	}();

	// Only records finished operations, which maps from io operation to histogram.
	std::mutex latency_collector_mu;
	std::array<LatencyStatsCollector, kIoOperationCount> latency_collector;
};

} // namespace duckdb
