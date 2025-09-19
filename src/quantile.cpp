#include "quantile.hpp"

namespace duckdb {

void P2Quantile::Add(double x) {
    if (n < 5) {
        buffer[n++] = x;
        if (n == 5) {
            Init();
        }
        return;
    }

    // Find k.
    int k = 0;
    if (x < markers[0].height) { markers[0].height = x; k = 0; }
    else if (x < markers[1].height) { k = 0; }
    else if (x < markers[2].height) { k = 1; }
    else if (x < markers[3].height) { k = 2; }
    else if (x < markers[4].height) { k = 3; }
    else { markers[4].height = x; k = 3; }

    for (int i = k+1; i < 5; i++) {
        markers[i].pos++;
    }

    // Update desired positions.
    for (int i = 0; i < 5; i++)
        desired[i] = 1 + (n - 1) * q_probs[i];

    // Adjust markers.
    for (int i = 1; i < 4; i++) {
        double d = desired[i] - markers[i].pos;
        if ((d >= 1 && markers[i+1].pos - markers[i].pos > 1) ||
            (d <= -1 && markers[i-1].pos - markers[i].pos < -1)) {
            int s = (d >= 0) ? 1 : -1;
            double newH = Parabolic(i, s);
            if (markers[i-1].height < newH && newH < markers[i+1].height) {
                markers[i].height = newH;
            }
            else {
                markers[i].height = Linear(i, s);
            }
            markers[i].pos += s;
        }
    }
    n++;
}

void P2Quantile::Init() {
    std::sort(buffer.begin(), buffer.begin() + 5);
    for (int i = 0; i < 5; i++) {
        markers[i] = {buffer[i], i+1};
    }
    for (int i = 0; i < 5; i++) {
        desired[i] = 1 + (n - 1) * q_probs[i];
    }
}

double P2Quantile::Parabolic(int i, int s) const {
    double m1 = markers[i-1].pos, m = markers[i].pos, m2 = markers[i+1].pos;
    double q1 = markers[i-1].height, q_ = markers[i].height, q2 = markers[i+1].height;
    return q_ + s / (m2 - m1) *
        ((m - m1 + s) * (q2 - q_) / (m2 - m) +
            (m2 - m - s) * (q_ - q1) / (m - m1));
}

double P2Quantile::Linear(int i, int s) const {
    return markers[i].height + s *
        (markers[i+s].height - markers[i].height) /
        (markers[i+s].pos - markers[i].pos);
}

}  // namespace duckdb
