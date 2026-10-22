#include "operation_latency_collector.hpp"

#include "duckdb/common/string_util.hpp"
#include <string_view>
#include "time_utils.hpp"

namespace duckdb {

const std::array<LatencyHeuristic, static_cast<size_t>(IoOperation::kUnknown)> kLatencyHeuristics = {{
    // kOpen
    {0, 1000, 100},
    // kRead
    {0, 1000, 100},
    // kList
    {0, 3000, 100},
    // kGlob
    {0, 3000, 100},
    // kStats,
    {0, 1000, 100},
}};

namespace {
constexpr std::string_view LATENCY_HISTOGRAM_ITEM = "latency";
constexpr std::string_view LATENCY_HISTOGRAM_UNIT = "millisec";
} // namespace

LatencyGuard::LatencyGuard(OperationLatencyCollector &latency_collector_p, IoOperation io_operation_p)
    : latency_collector(latency_collector_p), io_operation(io_operation_p),
      start_timestamp(GetSteadyNowMilliSecSinceEpoch()) {
}

LatencyGuard::~LatencyGuard() {
	const auto now = GetSteadyNowMilliSecSinceEpoch();
	const auto latency_millisec = now - start_timestamp;
	latency_collector.RecordOperationEnd(io_operation, latency_millisec);
}

OperationLatencyCollector::OperationLatencyCollector() {
	for (size_t i = 0; i < kIoOperationCount; ++i) {
		const auto &heuristic = kLatencyHeuristics[i];
		latency_collector[i].histogram =
		    make_uniq<Histogram>(heuristic.min_latency_ms, heuristic.max_latency_ms, heuristic.num_buckets);
		latency_collector[i].histogram->SetStatsDistribution(string(LATENCY_HISTOGRAM_ITEM),
		                                                     string(LATENCY_HISTOGRAM_UNIT));
		latency_collector[i].quantile_estimator =
		    make_uniq<QuantileEstimator>(string(LATENCY_HISTOGRAM_ITEM), string(LATENCY_HISTOGRAM_UNIT));
	}
}

LatencyGuard OperationLatencyCollector::RecordOperationStart(IoOperation io_oper) {
	return LatencyGuard {*this, io_oper};
}

void OperationLatencyCollector::RecordOperationEnd(IoOperation io_oper, int64_t latency_millisec) {
	annotated_lock_guard<annotated_mutex> lck(latency_collector_mu);
	auto &cur_histogram = latency_collector[static_cast<idx_t>(io_oper)].histogram;
	cur_histogram->Add(latency_millisec);
	auto &cur_quantile_estimator = latency_collector[static_cast<idx_t>(io_oper)].quantile_estimator;
	cur_quantile_estimator->Add(static_cast<float>(latency_millisec));
}

string OperationLatencyCollector::GetHumanReadableStats() {
	annotated_lock_guard<annotated_mutex> lck(latency_collector_mu);
	string stats;

	// Record IO operation latency.
	for (idx_t cur_oper_idx = 0; cur_oper_idx < kIoOperationCount; ++cur_oper_idx) {
		// Check histogram.
		auto &cur_histogram = latency_collector[static_cast<idx_t>(cur_oper_idx)].histogram;
		if (cur_histogram->counts() == 0) {
			continue;
		}
		stats += StringUtil::Format("\n\n%s operation histogram is %s", OPER_NAMES[cur_oper_idx].data(),
		                            cur_histogram->FormatString());

		// Check important quantiles.
		auto &cur_quantile_estimator = latency_collector[static_cast<idx_t>(cur_oper_idx)].quantile_estimator;
		stats += StringUtil::Format("\n%s operation quantile is %s", OPER_NAMES[cur_oper_idx].data(),
		                            cur_quantile_estimator->FormatString());
	}

	return stats;
}

} // namespace duckdb
