#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>

#include "duckdb/common/helper.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "duckdb/common/vector.hpp"
#include "histogram.hpp"
#include "io_operation.hpp"
#include "quantile_estimator.hpp"

namespace duckdb {

// Forward declaration.
class OperationLatencyCollector;

// Heuristic estimation of single IO request latency, out of which range are classified as outliers.
struct LatencyHeuristic {
	double min_latency_ms;
	double max_latency_ms;
	int num_buckets;
};

extern const std::array<LatencyHeuristic, static_cast<size_t>(IoOperation::kUnknown)> kLatencyHeuristics;

// A RAII guard to measure latency for IO operations.
class LatencyGuard {
public:
	LatencyGuard(OperationLatencyCollector &latency_collector_p, IoOperation io_operation_p);
	~LatencyGuard();

	LatencyGuard(const LatencyGuard &) = delete;
	LatencyGuard &operator=(const LatencyGuard &) = delete;
	LatencyGuard(LatencyGuard &&) = default;
	LatencyGuard &operator=(LatencyGuard &&) = default;

private:
	OperationLatencyCollector &latency_collector;
	IoOperation io_operation = IoOperation::kUnknown;
	int64_t start_timestamp = 0;
};

class OperationLatencyCollector {
public:
	OperationLatencyCollector();
	~OperationLatencyCollector() = default;

	LatencyGuard RecordOperationStart(IoOperation io_oper);

	// Represent stats in human-readable format.
	// Return empty string if no stats.
	std::string GetHumanReadableStats();

private:
	friend class LatencyGuard;

	struct LatencyStatsCollector {
		unique_ptr<Histogram> histogram;
		unique_ptr<QuantileEstimator> quantile_estimator;
	};

	// Mark the end of the a completed IO operation, disregard it's successful or not.
	void RecordOperationEnd(IoOperation io_oper, int64_t latency_millisec);

	// Only records finished operations, which maps from io operation to histogram.
	std::mutex latency_collector_mu;
	std::array<LatencyStatsCollector, kIoOperationCount> latency_collector;
};

} // namespace duckdb
