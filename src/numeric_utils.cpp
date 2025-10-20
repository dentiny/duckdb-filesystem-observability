#include "numeric_utils.hpp"

#include <cmath>

namespace duckdb {

namespace {
constexpr double MAX_ERROR_RANGE = 0.01;
} // namespace

bool IsDoubleEqual(double actual, double expected, bool print_if_unequal) {
	const bool is_equal = std::fabs(actual - expected) <= MAX_ERROR_RANGE;
	if (!is_equal && print_if_unequal) {
		std::cerr << "Actual value = " << actual << ", expected value = " << expected << std::endl;
	}
	return is_equal;
}

} // namespace duckdb
