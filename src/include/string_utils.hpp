// This file contains string related util functions.

#pragma once

#include "duckdb/common/string.hpp"

namespace duckdb {

// Get object storage bucket name, currently only supports S3 and GCS.
// If the given filepath is unknown, return empty string.
// TODO(hjiang): std::opional is a more proper return type.
std::string GetObjectStorageBucket(const std::string &filepath);

} // namespace duckdb
