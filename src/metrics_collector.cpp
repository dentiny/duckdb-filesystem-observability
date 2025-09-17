#include "metrics_collector.hpp"

#include <utility>

#include "duckdb/common/types/uuid.hpp"

namespace duckdb {

MetricsCollector::MetricsCollector() : overall_latency_histogram_(make_uniq<OperationLatencyHistogram>()) {
}

std::string MetricsCollector::GenerateOperId() const {
	return UUID::ToString(UUID::GenerateRandomUUID());
}

LatencyGuard MetricsCollector::RecordOperationStart(IoOperation io_oper) {
	std::lock_guard<std::mutex> lck(latency_histogram_mu);
	overall_latency_histogram_->RecordOperationStart(std::move(io_oper));
}

std::string MetricsCollector::GetHumanReadableStats() {
	std::lock_guard<std::mutex> lck(latency_histogram_mu);
	return overall_latency_histogram_->GetHumanReadableStats();
}

void MetricsCollector::Reset() {
	std::lock_guard<std::mutex> lck(latency_histogram_mu);
	overall_latency_histogram_ = make_uniq<OperationLatencyHistogram>();
}

} // namespace duckdb
