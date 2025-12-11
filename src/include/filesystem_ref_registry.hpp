// CacheFsRefRegistry is a registry which stores references for all cache filesystems.
// The class is thread-safe.

#pragma once

#include <mutex>

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
	mutable std::mutex mu;
	// The ownership lies in db instance.
	vector<ObservabilityFileSystem *> observability_filesystems;
};

} // namespace duckdb
