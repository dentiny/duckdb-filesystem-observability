// CacheFsRefRegistry is a registry which stores references for all cache filesystems.
// The class is thread-safe.

#pragma once

#include "duckdb/common/mutex.hpp"

#include "duckdb/common/vector.hpp"

namespace duckdb {

// Forward declaration.
class ObservabilityFileSystem;

class ObservabilityFsRefRegistry {
public:
	// Register the cache filesystem to the registry.
	void Register(ObservabilityFileSystem *fs);

	// Reset the registry.
	void Reset();

	// Get all cache filesystems.
	vector<ObservabilityFileSystem *> GetAllObservabilityFs() const;

private:
	mutable annotated_mutex mu;
	// The ownership lies in db instance.
	vector<ObservabilityFileSystem *> observability_filesystems DUCKDB_GUARDED_BY(mu);
};

} // namespace duckdb
