#include "string_utils.hpp"

#include "duckdb/common/string_util.hpp"

namespace duckdb {

namespace {
const char *S3_PREFIX = "s3://";
const char *GCS_PREFIX = "gs://";

// Precondition: [`filepath`] starts with [`s3://`].
string GetS3Bucket(const string &filepath) {
	string rest = filepath.substr(5);
	auto pos = rest.find('/');
	return (pos == string::npos) ? rest : rest.substr(0, pos);
}

// Precondition: [`filepath`] starts with [`gs://`].
string GetGcsBucket(const string &filepath) {
	string rest = filepath.substr(5);
	auto pos = rest.find('/');
	return (pos == string::npos) ? rest : rest.substr(0, pos);
}

} // namespace

string GetObjectStorageBucket(const string &filepath) {
	if (StringUtil::StartsWith(filepath, S3_PREFIX)) {
		return GetS3Bucket(filepath);
	}
	if (StringUtil::StartsWith(filepath, GCS_PREFIX)) {
		return GetGcsBucket(filepath);
	}
	return "";
}

} // namespace duckdb
