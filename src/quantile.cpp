#include "quantile.hpp"

namespace duckdb {

void P2Quantile::Add(float x) {
    int k = 0;
    if (x < markers[0].height) { 
        markers[0].height = x; 
        k = 0; 
    }
    else if (x < markers[1].height) { 
        k = 0; 
    }
    else if (x < markers[2].height) { 
        k = 1; 
    }
    else if (x < markers[3].height) { 
        k = 2; 
    }
    else if (x < markers[4].height) { 
        k = 3; 
    }
    else { 
        markers[4].height = x; 
        k = 3;
    }

    for (int ii = k + 1; ii < 5; ++ii) {
        markers[ii].pos++;
    }

    // Update desired positions.
    for (int ii = 0; ii < 5; ++ii)
        desired[ii] = 1 + (n - 1) * q_probs[ii];

    // Adjust markers.
    for (int ii = 1; ii < 4; ++ii) {
        double d = desired[ii] - markers[ii].pos;
        if ((d >= 1 && markers[ii + 1].pos - markers[ii].pos > 1) ||
            (d <= -1 && markers[ii - 1].pos - markers[ii].pos < -1)) {
            int s = (d >= 0) ? 1 : -1;
            double newH = Parabolic(ii, s);
            if (markers[ii - 1].height < newH && newH < markers[ii + 1].height) {
                markers[ii].height = newH;
            }
            else {
                markers[ii].height = Linear(ii, s);
            }
            markers[ii].pos += s;
        }
    }
    n++;
}

void P2Quantile::BulkAdd(vector<float> data_points) {
    for (float cur_data : data_points) {
        Add(cur_data);
    }
}

float P2Quantile::Parabolic(int i, int s) const {
    double m1 = markers[i-1].pos, m = markers[i].pos, m2 = markers[i+1].pos;
    double q1 = markers[i-1].height, q_ = markers[i].height, q2 = markers[i+1].height;
    return q_ + s / (m2 - m1) *
        ((m - m1 + s) * (q2 - q_) / (m2 - m) +
            (m2 - m - s) * (q_ - q1) / (m - m1));
}

float P2Quantile::Linear(int i, int s) const {
    return markers[i].height + s *
        (markers[i+s].height - markers[i].height) /
        (markers[i+s].pos - markers[i].pos);
}

}  // namespace duckdb
