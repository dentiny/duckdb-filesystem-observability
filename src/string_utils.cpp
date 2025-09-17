#include "string_utils.hpp"

#include "duckdb/common/string_util.hpp"

namespace duckdb {

namespace {
const char *S3_PREFIX = "s3://";
const char *GCS_PREFIX = "gs://";

// Precondition: [`filepath`] starts with [`s3://`].
std::string GetS3Bucket(const std::string &filepath) {
	std::string rest = filepath.substr(5);
	auto pos = rest.find('/');
	return (pos == std::string::npos) ? rest : rest.substr(0, pos);
}

// Precondition: [`filepath`] starts with [`gs://`].
std::string GetGcsBucket(const std::string &filepath) {
	std::string rest = filepath.substr(5);
	auto pos = rest.find('/');
	return (pos == std::string::npos) ? rest : rest.substr(0, pos);
}

} // namespace

std::string GetObjectStorageBucket(const std::string &filepath) {
	if (StringUtil::StartsWith(filepath, S3_PREFIX)) {
		return GetS3Bucket(filepath);
	}
	if (StringUtil::StartsWith(filepath, GCS_PREFIX)) {
		return GetGcsBucket(filepath);
	}
	return "";
}

} // namespace duckdb
