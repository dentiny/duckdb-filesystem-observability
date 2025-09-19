#pragma once

#include <array>
#include <vector>

#include "quantile.hpp"

namespace duckdb {

class QuantileEstimator {
public:
    QuantileEstimator() {
        constexpr std::array<double, 5> QUANTILES {0.50, 0.75, 0.90, 0.95, 0.99};
        estimators.reserve(QUANTILES.size());
        for (double q : QUANTILES) {
            estimators.emplace_back(q);
        }
    }
    void Add(double x) { for (auto& e : estimators) e.Add(x); }
    double p50() const { return estimators[0].get(); }
    double p75() const { return estimators[1].get(); }
    double p90() const { return estimators[2].get(); }
    double p95() const { return estimators[3].get(); }
    double p99() const { return estimators[4].get(); }

private:
    std::vector<P2Quantile> estimators;
};

} // namespace duckdb
