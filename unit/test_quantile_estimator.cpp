#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <algorithm>
#include <random>

#include "quantile_estimator.hpp"
#include "duckdb/common/vector.hpp"

using namespace duckdb; // NOLINT

namespace {
vector<int> GetRandomNumbers() {
	constexpr size_t NUM_VALUE = 1000;
	vector<int> v;
	v.reserve(NUM_VALUE);
	for (int i = 1; i <= NUM_VALUE; ++i) {
		v.push_back(i);
	}
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(v.begin(), v.end(), g);
	return v;
}
} // namespace

TEST_CASE("Quantile test", "[quantile test]") {
	constexpr double MAX_TOLERABLE_DIFF = 10;

	QuantileEstimator qe;
	const auto values = GetRandomNumbers();
	for (int cur_val : values) {
		qe.Add(cur_val);
	}

	const double p50 = qe.p50();
	REQUIRE(p50 >= 500 - MAX_TOLERABLE_DIFF);
	REQUIRE(p50 <= 500 + MAX_TOLERABLE_DIFF);

	const double p75 = qe.p75();
	REQUIRE(p75 >= 750 - MAX_TOLERABLE_DIFF);
	REQUIRE(p75 <= 750 + MAX_TOLERABLE_DIFF);

	const double p90 = qe.p90();
	REQUIRE(p90 >= 900 - MAX_TOLERABLE_DIFF);
	REQUIRE(p90 <= 900 + MAX_TOLERABLE_DIFF);

	const double p95 = qe.p95();
	REQUIRE(p95 >= 950 - MAX_TOLERABLE_DIFF);
	REQUIRE(p95 <= 950 + MAX_TOLERABLE_DIFF);

	const double p99 = qe.p99();
	REQUIRE(p99 >= 990 - MAX_TOLERABLE_DIFF);
	REQUIRE(p99 <= 990 + MAX_TOLERABLE_DIFF);
}

int main(int argc, char **argv) {
	int result = Catch::Session().run(argc, argv);
	return result;
}
