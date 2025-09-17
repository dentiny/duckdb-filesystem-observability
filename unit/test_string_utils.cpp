#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "duckdb/common/string.hpp"
#include "string_utils.hpp"

using namespace duckdb; // NOLINT

TEST_CASE("Get bucket name test", "[string utils test]") {
	// Local filepath.
	{
		const string filepath = "/tmp/local/file";
		REQUIRE(GetObjectStorageBucket(filepath) == "");
	}
	// S3 filepath.
	{
		const string filepath = "s3://bucket/directory/object";
		REQUIRE(GetObjectStorageBucket(filepath) == "bucket");
	}
	// GCS filepath.
	{
		const string filepath = "gs://bucket/directory/object";
		REQUIRE(GetObjectStorageBucket(filepath) == "bucket");
	}
}

int main(int argc, char **argv) {
	int result = Catch::Session().run(argc, argv);
	return result;
}
