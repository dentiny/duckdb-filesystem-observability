#pragma once

#include <cstdint>
#include <mutex>

#include "duckdb/common/helper.hpp"
#include "duckdb/common/string.hpp"
#include "histogram.hpp"
#include "io_operation.hpp"

namespace duckdb {

class OperationSizeCollector {
public:
	OperationSizeCollector();
	~OperationSizeCollector() = default;

	void RecordOperationSize(IoOperation io_oper, int64_t request_size);

	// Collect human-readable stats for operation size.
	string GetHumanReadableStats();

private:
	std::mutex mu;
	std::array<unique_ptr<Histogram>, kIoOperationCount> request_size_histograms;
};

} // namespace duckdb
