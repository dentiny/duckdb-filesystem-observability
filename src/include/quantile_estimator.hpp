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

    void Add(float x) {
        std::lock_guard<std::mutex> lck(mu);
        if (!estimators.empty()) {
            for (auto& e : estimators) {
                e.Add(x); 
            }
            return;
        }

        // If inline memory storage reaches threshold, switch to stats based method.
        if (quantile_lite.GetNumCollected() >= LARGE_SCALE_DATA_POINT_THRESHOLD) {
            InitializeP2QuantileWithLock();
            for (auto& e : estimators) {
                e.Add(x); 
            }
            return;
        }
        
        // Fallback to in-memory storage.
        quantile_lite.Add(x);
    }

    float p50() {
        std::lock_guard<std::mutex> lck(mu);
        if (estimators.empty()) {
            return quantile_lite.p50();
        }
        return estimators[0].Get(); 
    }
    float p75() {
        std::lock_guard<std::mutex> lck(mu);
        if (estimators.empty()) {
            return quantile_lite.p75();
        }
        return estimators[1].Get(); 
    }
    float p90() {
        std::lock_guard<std::mutex> lck(mu);
        if (estimators.empty()) {
            return quantile_lite.p90();
        }
        return estimators[2].Get(); 
    }
    float p95() {
        std::lock_guard<std::mutex> lck(mu);
        if (estimators.empty()) {
            return quantile_lite.p95();
        }
        return estimators[3].Get(); 
    }
    float p99() {
        std::lock_guard<std::mutex> lck(mu);
        if (estimators.empty()) {
            return quantile_lite.p99();
        }
        return estimators[4].Get(); 
    }

private:
    // Initialize P2 quantile.
    void InitializeP2QuantileWithLock() {
        D_ASSERT(estimators.empty());
        constexpr std::array<double, 5> QUANTILES {0.50, 0.75, 0.90, 0.95, 0.99};
        estimators.reserve(QUANTILES.size());
        for (double q : QUANTILES) {
            estimators.emplace_back(q);
        }

        auto data_points = quantile_lite.Extract();
        for (auto& e : estimators) {
            e.BulkAdd(data_points);
        }
    }

    // Used to trigger large-scale data point ingestion.
    inline static constexpr size_t LARGE_SCALE_DATA_POINT_THRESHOLD = 512;
    mutable std::mutex mu;
    // Used for small scale data points, where P² algorithm doesn't work well and memory footprint is acceptable.
    QuantileLite quantile_lite;
    // Used for large scale data points, where P² algorithm generates precise-enough quantile result.
    vector<P2Quantile> estimators;
};

} // namespace duckdb
