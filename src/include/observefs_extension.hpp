#pragma once

#include "duckdb.hpp"

namespace duckdb {

class ObservefsExtension : public Extension {
public:
	void Load(ExtensionLoader &loader) override;
	std::string Name() override;
	std::string Version() const override;
	~ObservefsExtension() override = default;

private:
	unique_ptr<Extension> httpfs_extension;
};

} // namespace duckdb
