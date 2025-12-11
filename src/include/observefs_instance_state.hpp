// Per-instance state for observefs extension.
// State is stored in DuckDB's ObjectCache for automatic cleanup when DatabaseInstance is destroyed.

#pragma once

#include <mutex>

#include "duckdb/common/shared_ptr.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/storage/object_cache.hpp"
#include "filesystem_ref_registry.hpp"

namespace duckdb {

//===--------------------------------------------------------------------===//
// Main per-instance state container
// Inherits from ObjectCacheEntry for automatic cleanup when DatabaseInstance is destroyed
//===--------------------------------------------------------------------===//
struct ObservefsInstanceState : public ObjectCacheEntry {
	static constexpr const char *OBJECT_TYPE = "ObservefsInstanceState";
	static constexpr const char *CACHE_KEY = "observefs_instance_state";

	ObservabilityFsRefRegistry registry;

	ObservefsInstanceState() = default;

	// ObjectCacheEntry interface
	string GetObjectType() override {
		return OBJECT_TYPE;
	}

	static string ObjectType() {
		return OBJECT_TYPE;
	}
};

//===--------------------------------------------------------------------===//
// Helper functions to access instance state
//===--------------------------------------------------------------------===//

// Store instance state in the duckdb instance.
void SetInstanceState(DatabaseInstance &instance, shared_ptr<ObservefsInstanceState> state);

// Get instance state as shared_ptr from DatabaseInstance, and returns nullptr if not set.
shared_ptr<ObservefsInstanceState> GetInstanceStateShared(DatabaseInstance &instance);

// Get instance state, throwing if not found.
ObservefsInstanceState &GetInstanceStateOrThrow(DatabaseInstance &instance);

} // namespace duckdb
