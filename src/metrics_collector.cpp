#include "metrics_collector.hpp"

#include <utility>

#include "duckdb/common/types/uuid.hpp"

namespace duckdb {

MetricsCollector::MetricsCollector() : overall_latency_histogram_(make_uniq<OperationLatencyHistogram>()) {
}

std::string MetricsCollector::GenerateOperId() const {
	return UUID::ToString(UUID::GenerateRandomUUID());
}

void MetricsCollector::RecordOperationStart(OperationLatencyHistogram::IoOperation io_oper,
                                            const std::string &oper_id) {
	overall_latency_histogram_->RecordOperationStart(std::move(io_oper), oper_id);
}
void MetricsCollector::RecordOperationEnd(OperationLatencyHistogram::IoOperation io_oper, const std::string &oper_id) {
	overall_latency_histogram_->RecordOperationEnd(std::move(io_oper), oper_id);
}

std::string MetricsCollector::GetHumanReadableStats() {
	return overall_latency_histogram_->GetHumanReadableStats();
}

void MetricsCollector::Reset() {
	overall_latency_histogram_ = make_uniq<OperationLatencyHistogram>();
}

} // namespace duckdb
