#pragma once

#include <array>
#include <cstddef>
#include <mutex>

#include "duckdb/common/helper.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "duckdb/common/vector.hpp"
#include "histogram.hpp"

namespace duckdb {

class OperationLatencyHistogram {
public:
<<<<<<< HEAD
	// TODO(hjiang): Add more IO operations.
	enum class IoOperation {
		kOpen = 0,
		kRead = 1,
		kUnknown,
	};
=======
    // TODO(hjiang): Add more IO operations.
    enum class IoOperation {
        kOpen = 0,
        kRead = 1,
        kUnknown = 2,
    };
>>>>>>> b2fd077 (fix and integrate metrics)

	OperationLatencyHistogram();
	~OperationLatencyHistogram() = default;

	std::string GenerateOperId() const;
	void RecordOperationStart(IoOperation io_oper, const std::string &oper_id);
	void RecordOperationEnd(IoOperation io_oper, const std::string &oper_id);

	std::string GetHumanReadableStats();

private:
	static constexpr auto kIoOperationCount = static_cast<size_t>(IoOperation::kUnknown);

	// Operation names, indexed by operation enums.
	inline static const vector<const char *> OPER_NAMES = []() {
		vector<const char *> oper_names;
		oper_names.reserve(kIoOperationCount);
		oper_names.emplace_back("open");
		oper_names.emplace_back("read");
		return oper_names;
	}();

	struct OperationStats {
		// Accounted as time elapsed since unix epoch in milliseconds.
		int64_t start_timestamp = 0;
	};
<<<<<<< HEAD

	// Only records finished operations, which maps from io operation to histogram.
	std::mutex histogram_mu;
=======
    
    // Only records finished operations, which maps from io operation to histogram.
    std::mutex histogram_mu;
>>>>>>> b2fd077 (fix and integrate metrics)
	std::array<unique_ptr<Histogram>, kIoOperationCount> histograms;

	// Ongoing operations.
	std::mutex ongoing_mu;
	using OperationStatsMap = unordered_map<string /*oper_id*/, OperationStats>;
	std::array<OperationStatsMap, kIoOperationCount> operation_events;
};

} // namespace duckdb
