#include "observefs_extension_callback.hpp"

#include "duckdb/main/client_context.hpp"
#include "observefs_instance_state.hpp"

namespace duckdb {

ObservefsExtensionCallback::ObservefsExtensionCallback(weak_ptr<ObservefsInstanceState> instance_state_p,
                                                       connection_t connection_id_p)
    : instance_state(std::move(instance_state_p)), connection_id(connection_id_p) {
}

ObservefsExtensionCallback::~ObservefsExtensionCallback() {
}

void ObservefsExtensionCallback::OnConnectionClosed(ClientContext &context) {
	const auto connection_id = context.GetConnectionId();
	auto state = instance_state.lock();
	if (state == nullptr) {
		// Instance state is gone, nothing to clean up
		return;
	}
	state->metrics_collector_manager.RemoveMetricsCollector(connection_id);
}

} // namespace duckdb
