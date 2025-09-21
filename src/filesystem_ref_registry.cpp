#include "filesystem_ref_registry.hpp"

#include "duckdb/common/exception.hpp"
#include "observability_filesystem.hpp"

namespace duckdb {

/*static*/ ObservabilityFsRefRegistry &ObservabilityFsRefRegistry::Get() {
	static auto *registry = new ObservabilityFsRefRegistry();
	return *registry;
}

void ObservabilityFsRefRegistry::Register(ObservabilityFileSystem *fs) {
	auto* internal_filesystem = fs->GetInternalFileSystem();

	// Validate internal filesystem is not another observability filesystem.
	if (dynamic_cast<ObservabilityFileSystem*>(internal_filesystem) != nullptr) {
		throw InvalidInputException("Cannot wrap an observabibility filesystem %s!", fs->GetName());
	}
 
	// Validate filesystem hasn't been registered before.
	for (const auto& already_registered : observability_filesystems) {
		if (already_registered->GetInternalFileSystem()->GetName() == internal_filesystem->GetName()) {
			throw InvalidInputException("Cannot re-register the same internal filesystem %s!", internal_filesystem->GetName());
		}
	}

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
