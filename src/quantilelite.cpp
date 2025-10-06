#include "quantilelite.hpp"

#include <algorithm>
#include <cmath>

namespace duckdb {

float QuantileLite::Quantile(float q) const {
	if (samples.empty()) {
		return 0.0;
	}
	std::sort(samples.begin(), samples.end());

	const size_t n = samples.size();
	if (n == 1) {
		return samples[0];
	}

	const float pos = q * (n - 1);
	const size_t lower = static_cast<size_t>(std::floor(pos));
	const size_t upper = static_cast<size_t>(std::ceil(pos));
	const float frac = pos - lower;

	if (upper == lower) {
		return samples[lower];
	}
	return samples[lower] + frac * (samples[upper] - samples[lower]);
}

} // namespace duckdb
