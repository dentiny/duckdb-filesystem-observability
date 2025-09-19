// This class is used to get quantile data for small scale data points.
//
// It's NOT thread-safe.

#pragma once

#include "duckdb/common/vector.hpp"

namespace duckdb {

class QuantileLite {
public:
    QuantileLite() = default;
    ~QuantileLite() = default;

    void Add(float x) {
        samples.push_back(x);
    }

    // Extract all data points out.
    vector<float> Extract() {
        auto extracted = std::move(samples);
        samples.clear();
        return extracted;
    }

    // Get number of data points already collected.
    size_t GetNumCollected() const {
        return samples.size();
    }

    float p50() {
        return Quantile(0.50); 
    }
    float p75() {
        return Quantile(0.75); 
    }
    float p90() {
        return Quantile(0.90); 
    }
    float p95() {
        return Quantile(0.95); 
    }
    float p99() {
        return Quantile(0.99); 
    }

private:
    float Quantile(float q);

    vector<float> samples;
};

}  // namespace duckdb
