# Per-Connection Metrics Implementation

This document describes the per-connection metrics feature implemented in the observefs extension, based on the reference implementation in `duck-read-cache-fs`.

## Overview

The extension now supports tracking filesystem I/O metrics independently for each database connection. This allows multiple concurrent connections to have their own isolated metrics without interference.

## Key Components

### 1. InstanceMetricsCollectorManager

Located in `src/include/observefs_instance_state.hpp` and `src/observefs_instance_state.cpp`.

This manager:
- Stores a `MetricsCollector` instance for each connection ID
- Provides thread-safe access to per-connection collectors
- Automatically cleans up collectors when connections close

**Key Methods:**
- `GetOrCreateMetricsCollector(connection_t)` - Gets existing or creates new collector
- `GetMetricsCollector(connection_t)` - Returns collector or nullptr
- `RemoveMetricsCollector(connection_t)` - Removes collector for connection
- `GetMetricsCollectorCount()` - Returns number of active collectors (for testing)

### 2. ObservefsInstanceState

Enhanced to include the `InstanceMetricsCollectorManager`:

```cpp
struct ObservefsInstanceState : public ObjectCacheEntry {
    ObservabilityFsRefRegistry registry;
    InstanceMetricsCollectorManager metrics_collector_manager;  // NEW
    // ...
};
```

### 3. ObservabilityFileSystem

Updated to track connection IDs and use per-connection metrics:

- `ObservabilityFileSystemHandle` now stores `connection_id`
- All I/O operations (Read, Write, Open, etc.) look up the connection-specific `MetricsCollector`
- Metrics are only collected if a collector exists for the connection

### 4. Connection Lifecycle Management

**ObservefsExtensionCallback** (new file):
- Implements `ClientContextState` to hook into connection lifecycle
- Automatically removes metrics collectors when connections close
- Prevents memory leaks from abandoned connections

### 5. Configuration

New extension option: `observefs_enable_metrics` (boolean, default: false)

**Usage:**
```sql
-- Enable metrics for current connection
SET observefs_enable_metrics = true;

-- Get metrics for current connection
SELECT observefs_get_profile();

-- Disable metrics
SET observefs_enable_metrics = false;
```

## Implementation Details

### Connection ID Tracking

1. When a file is opened via `OpenFile()`, the connection ID is extracted from the `FileOpener`
2. The connection ID is stored in the `ObservabilityFileSystemHandle`
3. All subsequent operations on that handle use the stored connection ID

### Metrics Collection Flow

```
User Query (Connection 1)
    ↓
FileSystem Operation (e.g., Read)
    ↓
Get Connection ID from Handle
    ↓
Lookup MetricsCollector for Connection 1
    ↓
Record Operation Start (creates LatencyGuard)
    ↓
Perform Actual I/O
    ↓
LatencyGuard Destructor Records Latency
```

### Thread Safety

- `InstanceMetricsCollectorManager` uses a mutex to protect the collector map
- Each `MetricsCollector` has its own internal synchronization
- Safe for concurrent access from multiple connections

## Testing

Test file: `unit/test_connection_cleanup.cpp`

**Test Cases:**
1. **Connection Cleanup**: Verifies that metrics collectors are automatically removed when connections are destroyed
2. **Independent Tracking**: Confirms that multiple connections maintain separate metrics

## Comparison with duck-read-cache-fs

| duck-read-cache-fs | observefs |
|-------------------|-----------|
| `BaseProfileCollector` | `MetricsCollector` |
| `InstanceProfileCollectorManager` | `InstanceMetricsCollectorManager` |
| `CacheHttpfsExtensionCallback` | `ObservefsExtensionCallback` |
| `cache_httpfs_profile_type` setting | `observefs_enable_metrics` setting |
| `CacheFileSystem` tracks conn IDs | `ObservabilityFileSystem` tracks conn IDs |

## Files Modified/Added

### New Files:
- `src/include/observefs_extension_callback.hpp`
- `src/observefs_extension_callback.cpp`
- `src/include/thread_annotation.hpp`
- `unit/test_connection_cleanup.cpp`

### Modified Files:
- `src/include/observefs_instance_state.hpp` - Added `InstanceMetricsCollectorManager`
- `src/observefs_instance_state.cpp` - Implemented manager and helper functions
- `src/include/observability_filesystem.hpp` - Added connection ID support
- `src/observability_filesystem.cpp` - Updated all I/O ops for per-connection metrics
- `src/observefs_extension.cpp` - Added `observefs_enable_metrics` configuration
- `CMakeLists.txt` - Added new source files and test

## Future Enhancements

Potential improvements:
1. Add SQL function to query specific connection's metrics by connection ID
2. Add function to list all active connections and their metric counts
3. Support global metrics aggregation across all connections
4. Add metrics export/serialization for external monitoring tools
