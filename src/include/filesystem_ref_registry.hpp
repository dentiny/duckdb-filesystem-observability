// CacheFsRefRegistry is a singleton registry which stores references for all cache filesystems.
// The class is not thread-safe.

#pragma once

#include "duckdb/common/vector.hpp"

namespace duckdb {

// Forward declaration.
class ObservabilityFileSystem;

class ObservabilityFsRefRegistry {
public:
	// Get the singleton instance for the registry.
	static ObservabilityFsRefRegistry &Get();

	// Register the cache filesystem to the registry.
	void Register(ObservabilityFileSystem *fs);

	// Reset the registry.
	void Reset();

	// Get all cache filesystems.
	const vector<ObservabilityFileSystem *> &GetAllObservabilityFs() const;

private:
	ObservabilityFsRefRegistry() = default;

	// The ownership lies in db instance.
	vector<ObservabilityFileSystem *> observability_filesystems;
};

} // namespace duckdb
