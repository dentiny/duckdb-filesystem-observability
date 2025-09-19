#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <algorithm>
#include <random>

#include "quantile_estimator.hpp"
#include "duckdb/common/vector.hpp"

using namespace duckdb; // NOLINT

namespace {
const std::string METRICS_NAME = "metrics_name";
const std::string METRICS_UNIT = "metrics_unit";

// Generate random numbers from [1, max_val].
vector<int> GetRandomNumbers(int max_val) {
	vector<int> numbers;
	numbers.reserve(max_val);
	for (int val = 1; val <= max_val; ++val) {
		numbers.push_back(val);
	}
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(numbers.begin(), numbers.end(), g);
	return numbers;
}
} // namespace

TEST_CASE("Small scale quantile test", "[quantile test]") {
	constexpr size_t NUM_VALUE = 50;
	constexpr double MAX_TOLERABLE_DIFF = 1.0;

	QuantileEstimator qe {METRICS_NAME, METRICS_UNIT};
	const auto values = GetRandomNumbers(NUM_VALUE);
	for (int cur_val : values) {
		qe.Add(cur_val);
	}

	const double p50 = qe.p50();
	REQUIRE(p50 >= 25.5 - MAX_TOLERABLE_DIFF);
	REQUIRE(p50 <= 25.5 + MAX_TOLERABLE_DIFF);

	const double p75 = qe.p75();
	REQUIRE(p75 >= 37.5 - MAX_TOLERABLE_DIFF);
	REQUIRE(p75 <= 37.5 + MAX_TOLERABLE_DIFF);

	const double p90 = qe.p90();
	REQUIRE(p90 >= 45.0 - MAX_TOLERABLE_DIFF);
	REQUIRE(p90 <= 45.0 + MAX_TOLERABLE_DIFF);

	const double p95 = qe.p95();
	REQUIRE(p95 >= 47.5 - MAX_TOLERABLE_DIFF);
	REQUIRE(p95 <= 47.5 + MAX_TOLERABLE_DIFF);

	const double p99 = qe.p99();
	REQUIRE(p99 >= 49.5 - MAX_TOLERABLE_DIFF);
	REQUIRE(p99 <= 49.5 + MAX_TOLERABLE_DIFF);
}

TEST_CASE("Large scale quantile test", "[quantile test]") {
	constexpr double MAX_TOLERABLE_DIFF = 10;
	constexpr size_t NUM_VALUE = 1000;

	QuantileEstimator qe {METRICS_NAME, METRICS_UNIT};
	const auto values = GetRandomNumbers(NUM_VALUE);
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
