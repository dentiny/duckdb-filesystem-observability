#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <algorithm>

namespace duckdb {

class P2Quantile {
public:
    explicit P2Quantile(double quantile) : q(quantile), n(0) {
        markers.fill({0.0, 0});
        desired.fill(0.0);
        q_probs = {0.0, q/2.0, q, (1.0+q)/2.0, 1.0};
    }

    // Add the given value to quantile.
    void Add(double x);

    // Return the current quantile estimate.
    //
    // If fewer than 5 samples have been observed (`n < 5`), the estimator is still in its initialization phase. In that case we simply return
    // the median of the values collected so far (approximated as buffer[n/2]) instead of using the P² algorithm.
    //
    // Once 5 or more samples are available, the P² algorithm is active. In this case the quantile estimate is always represented by the
    // 3rd marker (markers[2]), which the algorithm maintains as an approximation of the target quantile (p50, p75, p90, etc., depending
    // on the instance).
    double get() const {
        return (n < 5) ? buffer[n/2] : markers[2].height;
    }

private:
    struct Marker {
        double height;
        int pos;
    };

    double q;
    int n;
    std::array<Marker, 5> markers;
    std::array<double, 5> desired;
    std::array<double, 5> buffer;
    std::array<double, 5> q_probs;

    void Init();

    double Parabolic(int i, int s) const;

    double Linear(int i, int s) const;
};

}  // namespace duckdb
