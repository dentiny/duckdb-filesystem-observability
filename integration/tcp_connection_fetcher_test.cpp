#include "tcp_connection_fetcher.hpp"

#include <iostream>

using namespace duckdb; // NOLINT

int main() {
	const auto tcp_conns = GetTcpConnectionStatus();
	for (const auto &[ip, cnt] : tcp_conns) {
		std::cout << "IP " << ip << " has " << cnt << " TCP connections" << std::endl;
	}
	return 0;
}
