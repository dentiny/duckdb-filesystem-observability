#pragma once

#include <mutex>

#include "duckdb/common/string.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "duckdb/common/vector.hpp"
#include "histogram.hpp"
#include "operation_latency_collector.hpp"

namespace duckdb {

// A RAII wrapper, which manages one or more latency guards.
class LatencyGuardWrapper {
public:
	LatencyGuardWrapper() = default;
	~LatencyGuardWrapper() = default;

	LatencyGuardWrapper(const LatencyGuardWrapper &) = delete;
	LatencyGuardWrapper &operator=(const LatencyGuardWrapper &) = delete;

	LatencyGuardWrapper(LatencyGuardWrapper &&) = default;
	LatencyGuardWrapper &operator=(LatencyGuardWrapper &&) = default;

	// Take the ownership of the given [`latency_guard`].
	void TakeGuard(LatencyGuard latency_guard);

private:
	vector<LatencyGuard> latency_guards;
};

class MetricsCollector {
public:
	MetricsCollector();
	~MetricsCollector() = default;

	LatencyGuardWrapper RecordOperationStart(IoOperation io_oper, const string &filepath);

	// Represent stats in human-readable format.
	std::string GetHumanReadableStats();

	// Reset all recorded metrics.
	void Reset();

private:
	// Overall latency histogram.
	std::mutex latency_histogram_mu;
	unique_ptr<OperationLatencyCollector> overall_latency_histogram_;
	// Bucket-wise latency histogram.
	unordered_map<string, unique_ptr<OperationLatencyCollector>> bucket_latency_histogram_;
};

} // namespace duckdb
