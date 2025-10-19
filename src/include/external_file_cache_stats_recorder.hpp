// Stats recorder for external file cache cache access situation.
// Notice, for performance consideration the cache access stats are fetched periodically, so access stats are not
// guaranteed to be 100% precise.

#pragma once

#include <mutex>

#include "duckdb/common/string.hpp"
#include "duckdb/common/typedefs.hpp"
#include "duckdb/common/vector.hpp"
#include "duckdb/storage/buffer/temporary_file_information.hpp"

namespace duckdb {

// Forward declaration.
class ExternalFileCache;

struct CacheAccessRecord {
	// Number of complete cache hits.
	idx_t hit_count = 0;
	// Number of complete cache misses.
	idx_t miss_count = 0;
	// Number of partial cache hits.
	idx_t partial_hit_count = 0;
};

class ExternalFileCacheStatsRecorder {
public:
	explicit ExternalFileCacheStatsRecorder(ExternalFileCache &external_file_cache_p)
	    : external_file_cache(external_file_cache_p) {
	}

	// Record read operation in the stats recorder.
	void AccessRead(const string &filepath, idx_t start_offset, idx_t bytes_to_read);

	// Get cache access records.
	CacheAccessRecord GetCacheAccessRecord() const;

private:
	// Update cache access.
	void UpdateCacheAccessRecordWithLock(const CachedFileInformation &cache_file_info);

	// Load cache information from external file cache.
	void UpdateExternalFileCacheWithLock();

	ExternalFileCache &external_file_cache;
	// Currently cached blocks in external file cache.
	vector<CachedFileInformation> cache_blocks;
	// Used to decide whether stats recorder should load fresh cache information.
	idx_t last_load_iteration = 0;
	idx_t cur_load_iteration = 0;
	// Cache access record.
	CacheAccessRecord cache_access_record;
	mutable std::mutex mu;
};

} // namespace duckdb
