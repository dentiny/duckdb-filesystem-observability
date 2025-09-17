#pragma once

#include "duckdb.hpp"

namespace duckdb {

class ObservefsExtension : public Extension {
public:
	void Load(DuckDB &db) override;
	std::string Name() override;
	std::string Version() const override;

private:
	unique_ptr<Extension> httpfs_extension;
};

} // namespace duckdb
