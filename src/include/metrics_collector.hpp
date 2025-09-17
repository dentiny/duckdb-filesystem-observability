#pragma once

#include <mutex>

#include "duckdb/common/string.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "histogram.hpp"
#include "operation_latency_histogram.hpp"

namespace duckdb {

class MetricsCollector {
public:
	MetricsCollector();
	~MetricsCollector() = default;

	LatencyGuard RecordOperationStart(IoOperation io_oper);

	// Represent stats in human-readable format.
	std::string GetHumanReadableStats();

	// Reset all recorded metrics.
	void Reset();

private:
	std::string GenerateOperId() const;

	// TODO(hjiang): Add per-bucket historgram metrics.
	//
	// Overall latency histogram.
	std::mutex latency_histogram_mu;
	unique_ptr<OperationLatencyHistogram> overall_latency_histogram_;
};

} // namespace duckdb
