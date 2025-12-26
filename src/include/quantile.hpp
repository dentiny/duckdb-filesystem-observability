// This class is used to get quantile data for large scale data points via P2 algorithm.
//
// It's NOT thread-safe.

#pragma once

#include <array>
#include <algorithm>

#include "duckdb/common/vector.hpp"

namespace duckdb {

class P2Quantile {
public:
	explicit P2Quantile(double quantile) : q(quantile), n(0) {
		markers.fill({0.0, 0});
		desired.fill(0.0);
		q_probs = {0.0, q / 2.0, q, (1.0 + q) / 2.0, 1.0};
	}

	// Add the given value to quantile.
	void Add(float x);

	// Bulk ingest data points to quantile.
	void BulkAdd(vector<float> data_points);

	// Return the current quantile estimate.
	//
	// Quantile estimate is always represented by the 3rd marker (markers[2]), which the algorithm maintains as an
	// approximation of the target quantile (p50, p75, p90, etc., depending on the instance).
	float Get() const {
		return markers[2].height;
	}

private:
	struct Marker {
		float height;
		int pos;
	};

	// P50, P75, P90, P95, P99
	static constexpr size_t QUANTILE_COUNT = 5;
	float q = 0;
	int n = 0;
	std::array<Marker, QUANTILE_COUNT> markers;
	std::array<float, QUANTILE_COUNT> desired;
	std::array<double, QUANTILE_COUNT> q_probs;

	float Parabolic(int i, int s) const;

	float Linear(int i, int s) const;
};

} // namespace duckdb
