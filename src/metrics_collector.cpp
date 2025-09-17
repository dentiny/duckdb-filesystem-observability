#include "metrics_collector.hpp"

#include <utility>

#include "string_utils.hpp"
#include "duckdb/common/string_util.hpp"

namespace duckdb {

void LatencyGuardWrapper::TakeGuard(LatencyGuard latency_guard) {
	latency_guards.emplace_back(std::move(latency_guard));
}

MetricsCollector::MetricsCollector() : overall_latency_histogram_(make_uniq<OperationLatencyHistogram>()) {
}

LatencyGuardWrapper MetricsCollector::RecordOperationStart(IoOperation io_oper, const string &filepath) {
	const auto bucket = GetObjectStorageBucket(filepath);
	LatencyGuardWrapper guard_wrapper {};

	std::lock_guard<std::mutex> lck(latency_histogram_mu);
	auto overall_latency_guard = overall_latency_histogram_->RecordOperationStart(io_oper);
	guard_wrapper.TakeGuard(std::move(overall_latency_guard));

	if (!bucket.empty()) {
		auto bucket_latency_guard = bucket_latency_histogram_[bucket]->RecordOperationStart(io_oper);
		guard_wrapper.TakeGuard(std::move(bucket_latency_guard));
	}

	return guard_wrapper;
}

std::string MetricsCollector::GetHumanReadableStats() {
	std::lock_guard<std::mutex> lck(latency_histogram_mu);
	std::string human_readable_stats;
	human_readable_stats +=
	    StringUtil::Format("Overall latency: \n%s\n", overall_latency_histogram_->GetHumanReadableStats());
	for (const auto &[bucket, histogram] : bucket_latency_histogram_) {
		human_readable_stats += StringUtil::Format("  Bucket: %s\n", bucket);
		human_readable_stats += StringUtil::Format("  Latency: %s\n", histogram->GetHumanReadableStats());
	}
	return human_readable_stats;
}

void MetricsCollector::Reset() {
	std::lock_guard<std::mutex> lck(latency_histogram_mu);
	overall_latency_histogram_ = make_uniq<OperationLatencyHistogram>();
}

} // namespace duckdb
