#include "metrics_collector.hpp"

#include <utility>

#include "duckdb/common/types/uuid.hpp"

namespace duckdb {

MetricsCollector::MetricsCollector() : overall_latency_histogram_(make_uniq<OperationLatencyHistogram>()) {
}

std::string MetricsCollector::GenerateOperId() const {
	return UUID::ToString(UUID::GenerateRandomUUID());
}

<<<<<<< HEAD
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
=======
void MetricsCollector::RecordOperationStart(OperationLatencyHistogram::IoOperation io_oper, const std::string &oper_id) {
    std::lock_guard<std::mutex> lck(latency_histogram_mu);
    overall_latency_histogram_->RecordOperationStart(std::move(io_oper), oper_id);
}
void MetricsCollector::RecordOperationEnd(OperationLatencyHistogram::IoOperation io_oper, const std::string &oper_id) {
    std::lock_guard<std::mutex> lck(latency_histogram_mu);
    overall_latency_histogram_->RecordOperationEnd(std::move(io_oper), oper_id);
}

std::string MetricsCollector::GetHumanReadableStats() {
    std::lock_guard<std::mutex> lck(latency_histogram_mu);
    return overall_latency_histogram_->GetHumanReadableStats();
}

void MetricsCollector::Reset() {
    std::lock_guard<std::mutex> lck(latency_histogram_mu);
    overall_latency_histogram_ = make_uniq<OperationLatencyHistogram>();
>>>>>>> b2fd077 (fix and integrate metrics)
}

} // namespace duckdb
