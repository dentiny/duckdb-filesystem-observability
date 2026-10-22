#include "observefs_instance_state.hpp"

#include "duckdb/main/database.hpp"

namespace duckdb {

void SetInstanceState(DatabaseInstance &instance, shared_ptr<ObservefsInstanceState> state) {
	instance.GetObjectCache().Put(string(ObservefsInstanceState::CACHE_KEY), std::move(state));
}

shared_ptr<ObservefsInstanceState> GetInstanceStateShared(DatabaseInstance &instance) {
	return instance.GetObjectCache().Get<ObservefsInstanceState>(string(ObservefsInstanceState::CACHE_KEY));
}

ObservefsInstanceState &GetInstanceStateOrThrow(DatabaseInstance &instance) {
	auto state = instance.GetObjectCache().Get<ObservefsInstanceState>(string(ObservefsInstanceState::CACHE_KEY));
	if (state == nullptr) {
		throw InternalException("observefs instance state not found - extension not properly loaded");
	}
	return *state;
}

} // namespace duckdb
