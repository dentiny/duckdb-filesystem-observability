#include "operation_latency_histogram.hpp"

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

const NoDestructor<string> LATENCY_HISTOGRAM_ITEM {"latency"};
const NoDestructor<string> LATENCY_HISTOGRAM_UNIT {"millisec"};
} // namespace

LatencyGuard::LatencyGuard(OperationLatencyHistogram &latency_collector_p, IoOperation io_operation_p)
    : latency_collector(latency_collector_p), io_operation(io_operation_p),
      start_timestamp(GetSteadyNowMilliSecSinceEpoch()) {
}

LatencyGuard::~LatencyGuard() {
	const auto now = GetSteadyNowMilliSecSinceEpoch();
	const auto latency_millisec = now - start_timestamp;
	latency_collector.RecordOperationEnd(io_operation, latency_millisec);
}

OperationLatencyHistogram::OperationLatencyHistogram() {
	histograms[static_cast<idx_t>(IoOperation::kOpen)] =
	    make_uniq<Histogram>(MIN_OPEN_LATENCY_MILLISEC, MAX_OPEN_LATENCY_MILLISEC, OPEN_LATENCY_NUM_BKT);
	histograms[static_cast<idx_t>(IoOperation::kOpen)]->SetStatsDistribution(*LATENCY_HISTOGRAM_ITEM,
	                                                                         *LATENCY_HISTOGRAM_UNIT);

	histograms[static_cast<idx_t>(IoOperation::kRead)] =
	    make_uniq<Histogram>(MIN_READ_LATENCY_MILLISEC, MAX_READ_LATENCY_MILLISEC, READ_LATENCY_NUM_BKT);
	histograms[static_cast<idx_t>(IoOperation::kRead)]->SetStatsDistribution(*LATENCY_HISTOGRAM_ITEM,
	                                                                         *LATENCY_HISTOGRAM_UNIT);
}

LatencyGuard OperationLatencyHistogram::RecordOperationStart(IoOperation io_oper) {
	return LatencyGuard {*this, io_oper};
}

void OperationLatencyHistogram::RecordOperationEnd(IoOperation io_oper, int64_t latency_millisec) {
	std::lock_guard<std::mutex> lck(histogram_mu);
	auto &cur_histogram = histograms[static_cast<idx_t>(io_oper)];
	cur_histogram->Add(latency_millisec);
}

std::string OperationLatencyHistogram::GetHumanReadableStats() {
	std::lock_guard<std::mutex> lck(histogram_mu);
	std::string stats;

	// Record IO operation latency.
	for (idx_t cur_oper_idx = 0; cur_oper_idx < kIoOperationCount; ++cur_oper_idx) {
		const auto &cur_histogram = histograms[cur_oper_idx];
		if (cur_histogram->counts() == 0) {
			continue;
		}
		stats = StringUtil::Format("%s\n"
		                           "%s operation latency is %s",
		                           stats, OPER_NAMES[cur_oper_idx], cur_histogram->FormatString());
	}

	return stats;
}

} // namespace duckdb
