#pragma once

#include <cstdint>
#include <mutex>

#include "duckdb/common/string.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "duckdb/common/vector.hpp"
#include "histogram.hpp"
#include "operation_latency_collector.hpp"
#include "operation_size_collector.hpp"

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

	// Record operation start without size.
	LatencyGuardWrapper RecordOperationStart(IoOperation io_oper, const string &filepath);
	// Record operation size with size.
	LatencyGuardWrapper RecordOperationStart(IoOperation io_oper, const string &filepath, int64_t bytes_to_read);

	// Represent stats in human-readable format.
	// If no stats collected, an empty string will be returned.
	std::string GetHumanReadableStats();

	// Reset all recorded metrics.
	void Reset();

private:
	LatencyGuardWrapper RecordOperationStartWithLock(IoOperation io_oper, const string &filepath);

	// Overall latency histogram.
	std::mutex mu;
	unique_ptr<OperationLatencyCollector> overall_latency_collector_;
	// Bucket-wise latency histogram.
	unordered_map<string, unique_ptr<OperationLatencyCollector>> bucket_latency_collector_;	
	// Operation size collector.
	unique_ptr<OperationSizeCollector> operation_size_collector_;
};

} // namespace duckdb
