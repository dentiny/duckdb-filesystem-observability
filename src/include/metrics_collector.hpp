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

	std::string GenerateOperId() const;
	void RecordOperationStart(OperationLatencyHistogram::IoOperation io_oper, const std::string &oper_id);
	void RecordOperationEnd(OperationLatencyHistogram::IoOperation io_oper, const std::string &oper_id);

	std::string GetHumanReadableStats();

	// Reset all recorded metrics.
	void Reset();

private:
<<<<<<< HEAD
	// TODO(hjiang): Add per-bucket historgram metrics.
	//
	// Overall latency histogram.
	unique_ptr<OperationLatencyHistogram> overall_latency_histogram_;
=======
    // TODO(hjiang): Add per-bucket historgram metrics.
    //
    // Overall latency histogram.
    std::mutex latency_histogram_mu;
    unique_ptr<OperationLatencyHistogram> overall_latency_histogram_;
>>>>>>> b2fd077 (fix and integrate metrics)
};

} // namespace duckdb
