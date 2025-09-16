#include "filesystem_ref_registry.hpp"

#include "observability_filesystem.hpp"

namespace duckdb {

/*static*/ ObservabilityFsRefRegistry &ObservabilityFsRefRegistry::Get() {
	static auto *registry = new ObservabilityFsRefRegistry();
	return *registry;
}

void ObservabilityFsRefRegistry::Register(ObservabilityFileSystem *fs) {
    // TODO(hjiang): Add assertion that filesystem cannot be repeatedly registered.
	observability_filesystems.emplace_back(fs);
}

void ObservabilityFsRefRegistry::Reset() {
	observability_filesystems.clear();
}

const vector<ObservabilityFileSystem *> &ObservabilityFsRefRegistry::GetAllObservabilityFs() const {
	return observability_filesystems;
}

} // namespace duckdb
