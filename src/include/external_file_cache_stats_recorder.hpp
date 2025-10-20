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
	    : external_file_cache(&external_file_cache_p) {
	}

	// Enable and disable stats record.
	void Enable();
	void Disable();

	// Record read operation in the stats recorder.
	void AccessRead(const string &filepath, idx_t start_offset, idx_t bytes_to_read);

	// Get cache access records.
	CacheAccessRecord GetCacheAccessRecord() const;

	// Clear cache access record.
	void ClearCacheAccessRecord();

	// Set external file cache, used to invoke at extension reload where database instance has changed.
	// TODO(hjiang): Current approach only assumes single external file cache and only supports one dataabase instance.
	void ResetExternalFileCache(ExternalFileCache &cache);

private:
	// Update cache access.
	void UpdateCacheAccessRecordWithLock(const CachedFileInformation &cache_file_info);

	// Load cache information from external file cache.
	void UpdateExternalFileCacheWithLock();

	ExternalFileCache *external_file_cache = nullptr;
	// Currently cached blocks in external file cache.
	vector<CachedFileInformation> cache_blocks;
	// Used to decide whether stats recorder should load fresh cache information.
	idx_t last_load_iteration = 0;
	idx_t cur_load_iteration = 0;
	// Cache access record.
	CacheAccessRecord cache_access_record;
	// Whether stats record is enabled.
	bool enabled = true;
	mutable std::mutex mu;
};

// Get global stats recorder.
ExternalFileCacheStatsRecorder &GetExternalFileCacheStatsRecorder();

// Initialize or set stats recorder.
void InitOrResetExternalFileCache(ExternalFileCache &cache);

} // namespace duckdb
