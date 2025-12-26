#include "metrics_collector.hpp"

#include <utility>

#include "string_utils.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/common/string_util.hpp"

namespace duckdb {

void LatencyGuardWrapper::TakeGuard(LatencyGuard latency_guard) {
	latency_guards.emplace_back(std::move(latency_guard));
}

MetricsCollector::MetricsCollector()
    : overall_latency_collector(make_uniq<OperationLatencyCollector>()),
      operation_size_collector(make_uniq<OperationSizeCollector>()) {
}

LatencyGuardWrapper MetricsCollector::RecordOperationStart(IoOperation io_oper, const string &filepath) {
	std::lock_guard<std::mutex> lck(mu);
	return RecordOperationStartWithLock(std::move(io_oper), filepath);
}

LatencyGuardWrapper MetricsCollector::RecordOperationStart(IoOperation io_oper, const string &filepath,
                                                           int64_t bytes_to_read) {
	std::lock_guard<std::mutex> lck(mu);
	operation_size_collector->RecordOperationSize(io_oper, bytes_to_read);
	return RecordOperationStartWithLock(std::move(io_oper), filepath);
}

LatencyGuardWrapper MetricsCollector::RecordOperationStartWithLock(IoOperation io_oper, const string &filepath) {
	const auto bucket = GetObjectStorageBucket(filepath);

	LatencyGuardWrapper guard_wrapper {};
	auto overall_latency_guard = overall_latency_collector->RecordOperationStart(io_oper);
	guard_wrapper.TakeGuard(std::move(overall_latency_guard));

	if (!bucket.empty()) {
		auto &cur_bucket_hist = bucket_latency_collector[bucket];
		if (cur_bucket_hist == nullptr) {
			cur_bucket_hist = make_uniq<OperationLatencyCollector>();
		}
		auto bucket_latency_guard = cur_bucket_hist->RecordOperationStart(io_oper);
		guard_wrapper.TakeGuard(std::move(bucket_latency_guard));
	}

	return guard_wrapper;
}

string MetricsCollector::GetHumanReadableStats() {
	std::lock_guard<std::mutex> lck(mu);

	string human_readable_stats;

	// Collect latency stats.
	const string overall_latency_stats_str = overall_latency_collector->GetHumanReadableStats();
	if (!overall_latency_stats_str.empty()) {
		human_readable_stats += StringUtil::Format("Overall latency: \n%s\n", overall_latency_stats_str);
	}

	for (const auto &bucket_and_histogram : bucket_latency_collector) {
		human_readable_stats += StringUtil::Format("  Bucket: %s\n", bucket_and_histogram.first);
		human_readable_stats +=
		    StringUtil::Format("  Latency: %s\n", bucket_and_histogram.second->GetHumanReadableStats());
	}

	// Collect request size stats.
	const auto size_stats = operation_size_collector->GetHumanReadableStats();
	if (!size_stats.empty()) {
		human_readable_stats += StringUtil::Format("\nRequest size: %s\n", size_stats);
	}

	return human_readable_stats;
}

void MetricsCollector::Reset() {
	std::lock_guard<std::mutex> lck(mu);
	overall_latency_collector = make_uniq<OperationLatencyCollector>();
	bucket_latency_collector.clear();
}

} // namespace duckdb
