#include "external_file_cache_stats_recorder.hpp"

#include <algorithm>

#include "duckdb/storage/external_file_cache.hpp"

namespace duckdb {

namespace {
// Record cache access stats information, we need to know the cache block information from external file cache.
// It's not cheap to load such information, so amotize the cost and only load at intervals.
//
// If current cache entry size is larger than [`CACHE_LOAD_THRESHOLD`], load cache information at intervals
// [`LARGE_LOAD_CACHE_INTERVAL`]. Otherwise load cache information at smaller intervals [`SMALL_LOAD_CACHE_INTERVAL`].
constexpr idx_t CACHE_LOAD_THRESHOLD = 64;
constexpr idx_t SMALL_LOAD_CACHE_INTERVAL = 4;
constexpr idx_t LARGE_LOAD_CACHE_INTERVAL = 32;

// Global stats recorder.
unique_ptr<ExternalFileCacheStatsRecorder> g_stats_recorder;

} // namespace

CacheAccessRecord ExternalFileCacheStatsRecorder::GetCacheAccessRecord() const {
	std::lock_guard<std::mutex> lck(mu);
	return cache_access_record;
}

void ExternalFileCacheStatsRecorder::Enable() {
	std::lock_guard<std::mutex> lck(mu);
	enabled = true;
}
void ExternalFileCacheStatsRecorder::Disable() {
	std::lock_guard<std::mutex> lck(mu);
	enabled = false;
}
void ExternalFileCacheStatsRecorder::AccessRead(const string &filepath, idx_t start_offset, idx_t bytes_to_read) {
	CachedFileInformation cur_cache_file_info;
	cur_cache_file_info.path = filepath;
	cur_cache_file_info.nr_bytes = bytes_to_read;
	cur_cache_file_info.location = start_offset;
	cur_cache_file_info.loaded = false; // Fill in "false" for binary search.

	std::lock_guard<std::mutex> lck(mu);
	if (!enabled || !external_file_cache->IsEnabled()) {
		return;
	}

	// Check current access.
	UpdateCacheAccessRecordWithLock(cur_cache_file_info);

	// Trigger cache information update if necessary.
	++cur_load_iteration;
	if (cache_blocks.size() <= CACHE_LOAD_THRESHOLD) {
		if (cur_load_iteration % SMALL_LOAD_CACHE_INTERVAL == 0) {
			UpdateExternalFileCacheWithLock();
		}
		return;
	}

	if (cur_load_iteration % LARGE_LOAD_CACHE_INTERVAL == 0) {
		UpdateExternalFileCacheWithLock();
	}
}

void ExternalFileCacheStatsRecorder::UpdateCacheAccessRecordWithLock(const CachedFileInformation &cache_file_info) {
	auto iter = std::lower_bound(cache_blocks.begin(), cache_blocks.end(), cache_file_info,
	                             [](const auto &lhs, const auto &rhs) {
		                             return std::tie(lhs.path, lhs.location, lhs.nr_bytes, lhs.loaded) <
		                                    std::tie(rhs.path, rhs.location, rhs.nr_bytes, rhs.loaded);
	                             });

	if (iter == cache_blocks.end()) {
		++cache_access_record.miss_count;
		return;
	}
	if (iter->path != cache_file_info.path) {
		++cache_access_record.miss_count;
		return;
	}
	if (!iter->loaded) {
		++cache_access_record.miss_count;
		return;
	}

	const auto cached_start_offset = iter->location;
	const auto cached_end_offset = iter->location + iter->nr_bytes;
	const auto request_start_offset = cache_file_info.location;
	const auto request_end_offset = cache_file_info.location + cache_file_info.nr_bytes;
	if (request_start_offset >= cached_end_offset) {
		++cache_access_record.miss_count;
		return;
	}
	if (request_end_offset <= cached_start_offset) {
		++cache_access_record.miss_count;
		return;
	}

	// Theere's overlap between cached content and requested one.
	if (request_start_offset >= cached_start_offset && request_end_offset <= cached_end_offset) {
		++cache_access_record.hit_count;
		return;
	}

	++cache_access_record.partial_hit_count;
}

void ExternalFileCacheStatsRecorder::UpdateExternalFileCacheWithLock() {
	auto cache_file_information = external_file_cache->GetCachedFileInformation();
	std::sort(cache_file_information.begin(), cache_file_information.end(), [](const auto &lhs, const auto &rhs) {
		return std::tie(lhs.path, lhs.location, lhs.nr_bytes, lhs.loaded) <
		       std::tie(rhs.path, rhs.location, rhs.nr_bytes, rhs.loaded);
	});
}

void ExternalFileCacheStatsRecorder::ClearCacheAccessRecord() {
	std::lock_guard<std::mutex> lck(mu);
	cache_access_record.hit_count = 0;
	cache_access_record.miss_count = 0;
	cache_access_record.partial_hit_count = 0;
}

void ExternalFileCacheStatsRecorder::ResetExternalFileCache(ExternalFileCache &cache) {
	std::lock_guard<std::mutex> lck(mu);
	external_file_cache = &cache;
}

ExternalFileCacheStatsRecorder &GetExternalFileCacheStatsRecorder() {
	D_ASSERT(g_stats_recorder != nullptr);
	return *g_stats_recorder;
}

void InitOrResetExternalFileCache(ExternalFileCache &cache) {
	if (g_stats_recorder == nullptr) {
		g_stats_recorder = make_uniq<ExternalFileCacheStatsRecorder>(cache);
	} else {
		g_stats_recorder->ResetExternalFileCache(cache);
	}
}

} // namespace duckdb
