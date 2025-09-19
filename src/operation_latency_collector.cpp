#include "operation_latency_collector.hpp"

#include "no_destructor.hpp"
#include "time_utils.hpp"

namespace duckdb {

namespace {
// Heuristic estimation of single IO request latency, out of which range are classified as outliers.
constexpr double MIN_READ_LATENCY_MILLISEC = 0;
constexpr double MAX_READ_LATENCY_MILLISEC = 1000;
constexpr int READ_LATENCY_NUM_BKT = 100;

constexpr double MIN_OPEN_LATENCY_MILLISEC = 0;
constexpr double MAX_OPEN_LATENCY_MILLISEC = 1000;
constexpr int OPEN_LATENCY_NUM_BKT = 100;

constexpr double MIN_GLOB_LATENCY_MILLISEC = 0;
constexpr double MAX_GLOB_LATENCY_MILLISEC = 1000;
constexpr int GLOB_LATENCY_NUM_BKT = 100;

constexpr double MIN_LIST_LATENCY_MILLISEC = 0;
constexpr double MAX_LIST_LATENCY_MILLISEC = 3000;
constexpr int LIST_LATENCY_NUM_BKT = 100;

const NoDestructor<string> LATENCY_HISTOGRAM_ITEM {"latency"};
const NoDestructor<string> LATENCY_HISTOGRAM_UNIT {"millisec"};
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
	latency_collector[static_cast<idx_t>(IoOperation::kOpen)].histogram =
	    make_uniq<Histogram>(MIN_OPEN_LATENCY_MILLISEC, MAX_OPEN_LATENCY_MILLISEC, OPEN_LATENCY_NUM_BKT);
	latency_collector[static_cast<idx_t>(IoOperation::kOpen)].histogram->SetStatsDistribution(*LATENCY_HISTOGRAM_ITEM,
	                                                                         *LATENCY_HISTOGRAM_UNIT);
	latency_collector[static_cast<idx_t>(IoOperation::kOpen)].quantile_estimator = make_uniq<QuantileEstimator>(*LATENCY_HISTOGRAM_ITEM,
	                                                                         *LATENCY_HISTOGRAM_UNIT);

	latency_collector[static_cast<idx_t>(IoOperation::kRead)].histogram =
	    make_uniq<Histogram>(MIN_READ_LATENCY_MILLISEC, MAX_READ_LATENCY_MILLISEC, READ_LATENCY_NUM_BKT);
	latency_collector[static_cast<idx_t>(IoOperation::kRead)].histogram->SetStatsDistribution(*LATENCY_HISTOGRAM_ITEM,
	                                                                         *LATENCY_HISTOGRAM_UNIT);
	latency_collector[static_cast<idx_t>(IoOperation::kRead)].quantile_estimator = make_uniq<QuantileEstimator>(*LATENCY_HISTOGRAM_ITEM,
	                                                                         *LATENCY_HISTOGRAM_UNIT);

	latency_collector[static_cast<idx_t>(IoOperation::kList)].histogram =
	    make_uniq<Histogram>(MIN_LIST_LATENCY_MILLISEC, MAX_LIST_LATENCY_MILLISEC, LIST_LATENCY_NUM_BKT);
	latency_collector[static_cast<idx_t>(IoOperation::kList)].histogram->SetStatsDistribution(*LATENCY_HISTOGRAM_ITEM,
	                                                                             *LATENCY_HISTOGRAM_UNIT);
    latency_collector[static_cast<idx_t>(IoOperation::kList)].quantile_estimator = make_uniq<QuantileEstimator>(*LATENCY_HISTOGRAM_ITEM,
	                                                                         *LATENCY_HISTOGRAM_UNIT);
}

LatencyGuard OperationLatencyCollector::RecordOperationStart(IoOperation io_oper) {
	return LatencyGuard {*this, io_oper};
}

void OperationLatencyCollector::RecordOperationEnd(IoOperation io_oper, int64_t latency_millisec) {
	std::lock_guard<std::mutex> lck(latency_collector_mu);
	auto &cur_histogram = latency_collector[static_cast<idx_t>(io_oper)].histogram;
	cur_histogram->Add(latency_millisec);
	auto& cur_quantile_estimator = latency_collector[static_cast<idx_t>(io_oper)].quantile_estimator;
	cur_quantile_estimator->Add(static_cast<float>(latency_millisec));
}

std::string OperationLatencyCollector::GetHumanReadableStats() {
	std::lock_guard<std::mutex> lck(latency_collector_mu);
	std::string stats;

	// Record IO operation latency.
	for (idx_t cur_oper_idx = 0; cur_oper_idx < kIoOperationCount; ++cur_oper_idx) {
		// Check histogram.
		auto &cur_histogram = latency_collector[static_cast<idx_t>(cur_oper_idx)].histogram;
		if (cur_histogram->counts() == 0) {
			continue;
		}
		stats += StringUtil::Format("\n%s operation histogram is %s",
		                           OPER_NAMES[cur_oper_idx], cur_histogram->FormatString());

		// Check important quantiles.
		auto& cur_quantile_estimator = latency_collector[static_cast<idx_t>(cur_oper_idx)].quantile_estimator;
		stats += StringUtil::Format("\n%s operation quantile is %s", OPER_NAMES[cur_oper_idx], cur_quantile_estimator->FormatString());
	}

	return stats;
}

} // namespace duckdb
