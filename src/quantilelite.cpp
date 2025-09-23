#include "quantilelite.hpp"

#include <algorithm>

namespace duckdb {

float QuantileLite::Quantile(float q) const {
	if (samples.empty()) {
		return 0.0;
	}
	const size_t k = static_cast<size_t>(q * (samples.size() - 1));
	std::nth_element(samples.begin(), samples.begin() + k, samples.end());
	return samples[k];
}

} // namespace duckdb
