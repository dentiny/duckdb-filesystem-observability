#pragma once

#include <array>
#include <mutex>
#include <utility>

#include "duckdb/common/string.hpp"
#include "duckdb/common/vector.hpp"
#include "quantile.hpp"
#include "quantilelite.hpp"

namespace duckdb {

class QuantileEstimator {
public:
	QuantileEstimator(string name_p, string unit_p)
	    : quantile_name(std::move(name_p)), quantile_unit(std::move(unit_p)) {
	}
	~QuantileEstimator() = default;

	// Add the given value to quantile calculator.
	void Add(float x);

	float p50() const;
	float p75() const;
	float p90() const;
	float p95() const;
	float p99() const;

	// Return human-readable string for quantile stats.
	string FormatString() const;

private:
	// Initialize P2 quantile.
	void InitializeP2QuantileWithLock();

	// Metrics name and unit.
	string quantile_name;
	string quantile_unit;

	// Used to trigger large-scale data point ingestion.
	static constexpr size_t LARGE_SCALE_DATA_POINT_THRESHOLD = 512;
	mutable std::mutex mu;
	// Used for small scale data points, where P² algorithm doesn't work well and memory footprint is acceptable.
	QuantileLite quantile_lite;
	// Used for large scale data points, where P² algorithm generates precise-enough quantile result.
	vector<P2Quantile> estimators;
};

} // namespace duckdb
