#pragma once

#include "duckdb.hpp"
#include "duckdb/common/string.hpp"

namespace duckdb {

class ObservefsExtension : public Extension {
public:
	void Load(ExtensionLoader &loader) override;
	string Name() override;
	string Version() const override;
	~ObservefsExtension() override = default;

private:
	unique_ptr<Extension> httpfs_extension;
};

} // namespace duckdb
