#pragma once

namespace duckdb {

// Return whether the given number A is equal to number B, in a tolerable error range.
bool IsDoubleEqual(double actual, double expected, bool print_if_unequal = false);

} // namespace duckdb
