#include "observefs_instance_state.hpp"

#include "duckdb/common/helper.hpp"
#include "duckdb/main/client_context.hpp"
#include "metrics_collector.hpp"

namespace duckdb {

//===--------------------------------------------------------------------===//
// InstanceMetricsCollectorManager implementation
//===--------------------------------------------------------------------===//

MetricsCollector &InstanceMetricsCollectorManager::GetOrCreateMetricsCollector(connection_t connection_id) {
	concurrency::lock_guard<concurrency::mutex> lock(mutex);
	auto it = metrics_collectors.find(connection_id);
	if (it != metrics_collectors.end()) {
		return *it->second;
	}
	// Create new metrics collector for this connection
	auto collector = make_uniq<MetricsCollector>();
	auto &collector_ref = *collector;
	metrics_collectors[connection_id] = std::move(collector);
	return collector_ref;
}

MetricsCollector *InstanceMetricsCollectorManager::GetMetricsCollector(connection_t connection_id) const {
	concurrency::lock_guard<concurrency::mutex> lock(mutex);
	auto it = metrics_collectors.find(connection_id);
	if (it != metrics_collectors.end()) {
		return it->second.get();
	}
	return nullptr;
}

void InstanceMetricsCollectorManager::ResetMetricsCollector(connection_t connection_id) {
	concurrency::lock_guard<concurrency::mutex> lock(mutex);
	auto it = metrics_collectors.find(connection_id);
	if (it != metrics_collectors.end() && it->second != nullptr) {
		it->second->Reset();
	}
}

void InstanceMetricsCollectorManager::RemoveMetricsCollector(connection_t connection_id) {
	concurrency::lock_guard<concurrency::mutex> lock(mutex);
	metrics_collectors.erase(connection_id);
}

idx_t InstanceMetricsCollectorManager::GetMetricsCollectorCount() const {
	concurrency::lock_guard<concurrency::mutex> lock(mutex);
	return metrics_collectors.size();
}

//===--------------------------------------------------------------------===//
// Instance state storage/retrieval using DuckDB's ObjectCache
//===--------------------------------------------------------------------===//

void SetInstanceState(DatabaseInstance &instance, shared_ptr<ObservefsInstanceState> state) {
	instance.GetObjectCache().Put(ObservefsInstanceState::CACHE_KEY, std::move(state));
}

shared_ptr<ObservefsInstanceState> GetInstanceStateShared(DatabaseInstance &instance) {
	return instance.GetObjectCache().Get<ObservefsInstanceState>(ObservefsInstanceState::CACHE_KEY);
}

ObservefsInstanceState &GetInstanceStateOrThrow(DatabaseInstance &instance) {
	auto state = instance.GetObjectCache().Get<ObservefsInstanceState>(ObservefsInstanceState::CACHE_KEY);
	if (state == nullptr) {
		throw InternalException("observefs instance state not found - extension not properly loaded");
	}
	return *state;
}

ObservefsInstanceState &GetInstanceStateOrThrow(ClientContext &context) {
	return GetInstanceStateOrThrow(*context.db.get());
}

MetricsCollector *GetMetricsCollector(const shared_ptr<ObservefsInstanceState> &instance_state, connection_t conn_id) {
	if (!instance_state || conn_id == DConstants::INVALID_INDEX) {
		return nullptr;
	}
	return instance_state->metrics_collector_manager.GetMetricsCollector(conn_id);
}

} // namespace duckdb
