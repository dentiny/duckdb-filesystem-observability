#include "operation_size_collector.hpp"

#include "duckdb/common/string_util.hpp"
#include "no_destructor.hpp"

namespace duckdb {

namespace {
const NoDestructor<string> SIZE_HISTOGRAM_ITEM {"request_size"};
const NoDestructor<string> SIZE_HISTOGRAM_UNIT {""};

// Heuristic estimation for request size.
constexpr double MIN_REQUEST_SIZE = 0;
constexpr double MAX_REQUEST_SIZE = 6 * 1024 * 1024;
constexpr int REQUEST_HIST_BUCKET_NUM = 128;
} // namespace

OperationSizeCollector::OperationSizeCollector() {
	for (size_t ii = 0; ii < kIoOperationCount; ++ii) {
		request_size_histograms[ii] = make_uniq<Histogram>(MIN_REQUEST_SIZE, MAX_REQUEST_SIZE, REQUEST_HIST_BUCKET_NUM);
	}
}

void OperationSizeCollector::RecordOperationSize(IoOperation io_oper, int64_t request_size) {
	std::lock_guard<std::mutex> lck(mu);
	request_size_histograms[static_cast<idx_t>(io_oper)]->Add(request_size);
}

string OperationSizeCollector::GetHumanReadableStats() {
	std::lock_guard<std::mutex> lck(mu);
	string stats;

	// Record IO operation size.
	for (idx_t cur_oper_idx = 0; cur_oper_idx < kIoOperationCount; ++cur_oper_idx) {
		// Check histogram.
		auto &cur_histogram = request_size_histograms[static_cast<idx_t>(cur_oper_idx)];
		if (cur_histogram->counts() == 0) {
			continue;
		}
		stats += StringUtil::Format("\n%s operation histogram is %s", OPER_NAMES[cur_oper_idx],
		                            cur_histogram->FormatString());
	}

	return stats;
}

} // namespace duckdb
