// Per-instance state for observefs extension.
// State is stored in DuckDB's ObjectCache for automatic cleanup when DatabaseInstance is destroyed.

#pragma once

#include "duckdb/common/mutex.hpp"
#include "duckdb/common/shared_ptr.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/common/typedefs.hpp"
#include "duckdb/common/unique_ptr.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "duckdb/storage/object_cache.hpp"
#include "filesystem_ref_registry.hpp"

namespace duckdb {

// Forward declarations
class MetricsCollector;
class ClientContext;
class DatabaseInstance;

//===--------------------------------------------------------------------===//
// Per-connection metrics collector manager
//===--------------------------------------------------------------------===//

// Manages per-connection metrics collectors.
// Each connection has its own collector so stats can be tracked independently.
class InstanceMetricsCollectorManager {
public:
	// Gets or creates the MetricsCollector for the given connection.
	MetricsCollector &GetOrCreateMetricsCollector(connection_t connection_id);
	// Returns the metrics collector for the given connection.
	// If no collector exists for the connection, returns nullptr.
	MetricsCollector *GetMetricsCollector(connection_t connection_id) const;
	// Resets the metrics collector for the given connection.
	void ResetMetricsCollector(connection_t connection_id);
	// Removes the metrics collector for the given connection.
	void RemoveMetricsCollector(connection_t connection_id);
	// Returns the number of registered metrics collectors, exposed for testing and debugging purposes.
	idx_t GetMetricsCollectorCount() const;

private:
	mutable mutex mu;
	unordered_map<connection_t, unique_ptr<MetricsCollector>> metrics_collectors;
};

//===--------------------------------------------------------------------===//
// Main per-instance state container
// Inherits from ObjectCacheEntry for automatic cleanup when DatabaseInstance is destroyed
//===--------------------------------------------------------------------===//
struct ObservefsInstanceState : public ObjectCacheEntry {
	static constexpr const char *OBJECT_TYPE = "ObservefsInstanceState";
	static constexpr const char *CACHE_KEY = "observefs_instance_state";

	ObservabilityFsRefRegistry registry;
	InstanceMetricsCollectorManager metrics_collector_manager;

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

// Get instance state from ClientContext, throwing if not found
ObservefsInstanceState &GetInstanceStateOrThrow(ClientContext &context);

// Get the per-connection metrics collector for the given connection.
// Returns nullptr if no collector exists or connection_id is invalid.
MetricsCollector *GetMetricsCollector(const shared_ptr<ObservefsInstanceState> &instance_state, connection_t conn_id);

} // namespace duckdb
