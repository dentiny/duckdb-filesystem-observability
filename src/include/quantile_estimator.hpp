#pragma once

#include <array>
#include <mutex>

#include "duckdb/common/vector.hpp"
#include "quantile.hpp"
#include "quantilelite.hpp"

namespace duckdb {

class QuantileEstimator {
public:
    QuantileEstimator() = default;
    ~QuantileEstimator() = default;

    // Add the given value to quantile calculator.
    void Add(float x);

    float p50();
    float p75();
    float p90();
    float p95();
    float p99();

private:
    // Initialize P2 quantile.
    void InitializeP2QuantileWithLock();

    // Used to trigger large-scale data point ingestion.
    inline static constexpr size_t LARGE_SCALE_DATA_POINT_THRESHOLD = 512;
    mutable std::mutex mu;
    // Used for small scale data points, where P² algorithm doesn't work well and memory footprint is acceptable.
    QuantileLite quantile_lite;
    // Used for large scale data points, where P² algorithm generates precise-enough quantile result.
    vector<P2Quantile> estimators;
};

} // namespace duckdb
