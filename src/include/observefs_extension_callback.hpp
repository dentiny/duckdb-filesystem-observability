#pragma once

#include "duckdb/common/shared_ptr.hpp"
#include "duckdb/common/typedefs.hpp"
#include "duckdb/planner/extension_callback.hpp"

namespace duckdb {

// Forward declaration
struct ObservefsInstanceState;
class ClientContext;

// Extension callback for observefs extension
// Handles connection lifecycle events (currently just cleanup on close)
class ObservefsExtensionCallback : public ExtensionCallback {
public:
	ObservefsExtensionCallback(weak_ptr<ObservefsInstanceState> instance_state_p, connection_t connection_id_p);
	~ObservefsExtensionCallback() override;

	void OnConnectionClosed(ClientContext &context) override;

private:
	weak_ptr<ObservefsInstanceState> instance_state;
	connection_t connection_id;
};

} // namespace duckdb
