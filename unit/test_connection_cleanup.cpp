#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "duckdb/common/vector.hpp"
#include "duckdb/main/connection.hpp"
#include "duckdb/main/connection_manager.hpp"
#include "duckdb/main/database.hpp"
#include "observefs_instance_state.hpp"

using namespace duckdb;

TEST_CASE("Multiple connections registered in profile manager are all removed when destructed",
          "[connection cleanup]") {
	DuckDB db(nullptr);
	auto &instance = db.instance;

	// Load the observefs extension to register the connection cleanup callback
	Connection initial_con(db);
	initial_con.Query("LOAD observefs;");

	// Get the instance state created by the extension
	auto instance_state = GetInstanceStateShared(*instance);
	REQUIRE(instance_state);

	constexpr size_t num_connections = 5;
	vector<unique_ptr<Connection>> connections;
	connections.reserve(num_connections);

	for (size_t idx = 0; idx < num_connections; ++idx) {
		connections.emplace_back(make_uniq<Connection>(db));
		auto result = connections.back()->Query("SET observefs_enable_metrics = true");
		REQUIRE(!result->HasError());
	}

	auto &connection_manager = ConnectionManager::Get(*instance);
	// Note: connection count includes the initial_con + num_connections
	REQUIRE(connection_manager.GetConnectionCount() == num_connections + 1);
	REQUIRE(instance_state->metrics_collector_manager.GetMetricsCollectorCount() == num_connections);

	// Destroy the test connections
	connections.clear();
	REQUIRE(connection_manager.GetConnectionCount() == 1); // Only initial_con remains
	REQUIRE(instance_state->metrics_collector_manager.GetMetricsCollectorCount() == 0);
}

int main(int argc, char **argv) {
	int result = Catch::Session().run(argc, argv);
	return result;
}
