#include "string_utils.hpp"

#include <string_view>

namespace duckdb {

namespace {
constexpr std::string_view S3_PREFIX = "s3://";
constexpr std::string_view GCS_PREFIX = "gs://";
} // namespace

string GetObjectStorageBucket(const string &filepath) {
	std::string_view path(filepath);
	for (const auto prefix : {S3_PREFIX, GCS_PREFIX}) {
		if (path.substr(0, prefix.size()) == prefix) {
			path.remove_prefix(prefix.size());
			return string(path.substr(0, path.find('/')));
		}
	}
	return "";
}

} // namespace duckdb
