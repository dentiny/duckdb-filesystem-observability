#include "quantile_estimator.hpp"

#include "duckdb/common/string_util.hpp"

namespace duckdb {

void QuantileEstimator::Add(float x) {
	std::lock_guard<std::mutex> lck(mu);
	if (!estimators.empty()) {
		for (auto &e : estimators) {
			e.Add(x);
		}
		return;
	}

	// If inline memory storage reaches threshold, switch to stats based method.
	if (quantile_lite.GetNumCollected() >= LARGE_SCALE_DATA_POINT_THRESHOLD) {
		InitializeP2QuantileWithLock();
		for (auto &e : estimators) {
			e.Add(x);
		}
		return;
	}

	// Fallback to in-memory storage.
	quantile_lite.Add(x);
}

float QuantileEstimator::p50() const {
	std::lock_guard<std::mutex> lck(mu);
	if (estimators.empty()) {
		return quantile_lite.p50();
	}
	return estimators[0].Get();
}
float QuantileEstimator::p75() const {
	std::lock_guard<std::mutex> lck(mu);
	if (estimators.empty()) {
		return quantile_lite.p75();
	}
	return estimators[1].Get();
}
float QuantileEstimator::p90() const {
	std::lock_guard<std::mutex> lck(mu);
	if (estimators.empty()) {
		return quantile_lite.p90();
	}
	return estimators[2].Get();
}
float QuantileEstimator::p95() const {
	std::lock_guard<std::mutex> lck(mu);
	if (estimators.empty()) {
		return quantile_lite.p95();
	}
	return estimators[3].Get();
}
float QuantileEstimator::p99() const {
	std::lock_guard<std::mutex> lck(mu);
	if (estimators.empty()) {
		return quantile_lite.p99();
	}
	return estimators[4].Get();
}

void QuantileEstimator::InitializeP2QuantileWithLock() {
	D_ASSERT(estimators.empty());
	constexpr std::array<double, 5> QUANTILES {0.50, 0.75, 0.90, 0.95, 0.99};
	estimators.reserve(QUANTILES.size());
	for (double q : QUANTILES) {
		estimators.emplace_back(q);
	}

	auto data_points = quantile_lite.Extract();
	for (auto &e : estimators) {
		e.BulkAdd(data_points);
	}
}

string QuantileEstimator::FormatString() const {
	string stats;
	stats += StringUtil::Format("\nP50 %s %f %s", quantile_name, p50(), quantile_unit);
	stats += StringUtil::Format("\nP75 %s %f %s", quantile_name, p75(), quantile_unit);
	stats += StringUtil::Format("\nP90 %s %f %s", quantile_name, p90(), quantile_unit);
	stats += StringUtil::Format("\nP95 %s %f %s", quantile_name, p95(), quantile_unit);
	stats += StringUtil::Format("\nP99 %s %f %s", quantile_name, p99(), quantile_unit);
	return stats;
}

} // namespace duckdb
