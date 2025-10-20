#include "observability_filesystem.hpp"

#include "duckdb/common/string_util.hpp"
#include "duckdb/main/client_context.hpp"
#include "external_file_cache_stats_recorder.hpp"

namespace duckdb {

ObservabilityFileSystemHandle::ObservabilityFileSystemHandle(unique_ptr<FileHandle> internal_file_handle_p,
                                                             ObservabilityFileSystem &fs)
    : FileHandle(fs, internal_file_handle_p->GetPath(), internal_file_handle_p->GetFlags()),
      internal_file_handle(std::move(internal_file_handle_p)) {
}

std::string ObservabilityFileSystem::GetName() const {
	const auto compount_name = StringUtil::Format("observability-%s", internal_filesystem->GetName());
	return compount_name;
}

void ObservabilityFileSystem::ClearObservabilityData() {
	metrics_collector.Reset();
}
std::string ObservabilityFileSystem::GetHumanReadableStats() {
	return metrics_collector.GetHumanReadableStats();
}

void ObservabilityFileSystem::Read(FileHandle &handle, void *buffer, int64_t nr_bytes, idx_t location) {
	GetExternalFileCacheStatsRecorder().AccessRead(handle.GetPath(), location, nr_bytes);
	const auto latency_guard = metrics_collector.RecordOperationStart(IoOperation::kRead, handle.GetPath(), nr_bytes);
	auto &observability_file_handle = handle.Cast<ObservabilityFileSystemHandle>();
	internal_filesystem->Read(*observability_file_handle.internal_file_handle, buffer, nr_bytes, location);
}
int64_t ObservabilityFileSystem::Read(FileHandle &handle, void *buffer, int64_t nr_bytes) {
	GetExternalFileCacheStatsRecorder().AccessRead(handle.GetPath(), handle.SeekPosition(), nr_bytes);
	const auto latency_guard = metrics_collector.RecordOperationStart(IoOperation::kRead, handle.GetPath(), nr_bytes);
	auto &observability_file_handle = handle.Cast<ObservabilityFileSystemHandle>();
	return internal_filesystem->Read(*observability_file_handle.internal_file_handle, buffer, nr_bytes);
}
unique_ptr<FileHandle> ObservabilityFileSystem::OpenFile(const string &path, FileOpenFlags flags,
                                                         optional_ptr<FileOpener> opener) {
	const auto latency_guard = metrics_collector.RecordOperationStart(IoOperation::kOpen, path);
	auto file_handle = internal_filesystem->OpenFile(path, flags, opener);
	return make_uniq<ObservabilityFileSystemHandle>(std::move(file_handle), *this);
}
int64_t ObservabilityFileSystem::GetFileSize(FileHandle &handle) {
	const auto latency_guard = metrics_collector.RecordOperationStart(IoOperation::kOpen, handle.GetPath());
	auto &observability_file_handle = handle.Cast<ObservabilityFileSystemHandle>();
	return internal_filesystem->GetFileSize(*observability_file_handle.internal_file_handle);
}
timestamp_t ObservabilityFileSystem::GetLastModifiedTime(FileHandle &handle) {
	auto &observability_file_handle = handle.Cast<ObservabilityFileSystemHandle>();
	return internal_filesystem->GetLastModifiedTime(*observability_file_handle.internal_file_handle);
}
unique_ptr<FileHandle> ObservabilityFileSystem::OpenCompressedFile(QueryContext context, unique_ptr<FileHandle> handle,
                                                                   bool write) {
	return internal_filesystem->OpenCompressedFile(std::move(context), std::move(handle), write);
}
void ObservabilityFileSystem::Write(FileHandle &handle, void *buffer, int64_t nr_bytes, idx_t location) {
	auto &observability_file_handle = handle.Cast<ObservabilityFileSystemHandle>();
	internal_filesystem->Write(*observability_file_handle.internal_file_handle, buffer, nr_bytes, location);
}
int64_t ObservabilityFileSystem::Write(FileHandle &handle, void *buffer, int64_t nr_bytes) {
	auto &observability_file_handle = handle.Cast<ObservabilityFileSystemHandle>();
	return internal_filesystem->Write(*observability_file_handle.internal_file_handle, buffer, nr_bytes);
}
void ObservabilityFileSystem::Truncate(FileHandle &handle, int64_t new_size) {
	auto &observability_file_handle = handle.Cast<ObservabilityFileSystemHandle>();
	internal_filesystem->Truncate(*observability_file_handle.internal_file_handle, new_size);
}
bool ObservabilityFileSystem::DirectoryExists(const string &directory, optional_ptr<FileOpener> opener) {
	return internal_filesystem->DirectoryExists(directory, opener);
}
void ObservabilityFileSystem::CreateDirectory(const string &directory, optional_ptr<FileOpener> opener) {
	internal_filesystem->CreateDirectory(directory, opener);
}
void ObservabilityFileSystem::RemoveDirectory(const string &directory, optional_ptr<FileOpener> opener) {
	internal_filesystem->RemoveDirectory(directory, opener);
}
bool ObservabilityFileSystem::ListFiles(const string &directory,
                                        const std::function<void(const string &, bool)> &callback, FileOpener *opener) {
	const auto latency_guard = metrics_collector.RecordOperationStart(IoOperation::kList, directory);
	return internal_filesystem->ListFiles(directory, callback, opener);
}
void ObservabilityFileSystem::MoveFile(const string &source, const string &target, optional_ptr<FileOpener> opener) {
	internal_filesystem->MoveFile(source, target, opener);
}
bool ObservabilityFileSystem::FileExists(const string &filename, optional_ptr<FileOpener> opener) {
	return internal_filesystem->FileExists(filename, opener);
}
void ObservabilityFileSystem::RemoveFile(const string &filename, optional_ptr<FileOpener> opener) {
	internal_filesystem->RemoveFile(filename, opener);
}
vector<OpenFileInfo> ObservabilityFileSystem::Glob(const string &path, FileOpener *opener) {
	const auto latency_guard = metrics_collector.RecordOperationStart(IoOperation::kGlob, path);
	return internal_filesystem->Glob(path, opener);
}
void ObservabilityFileSystem::Seek(FileHandle &handle, idx_t location) {
	auto &observability_file_handle = handle.Cast<ObservabilityFileSystemHandle>();
	internal_filesystem->Seek(*observability_file_handle.internal_file_handle, location);
}
void ObservabilityFileSystem::Reset(FileHandle &handle) {
	auto &observability_file_handle = handle.Cast<ObservabilityFileSystemHandle>();
	internal_filesystem->Reset(*observability_file_handle.internal_file_handle);
}
idx_t ObservabilityFileSystem::SeekPosition(FileHandle &handle) {
	auto &observability_file_handle = handle.Cast<ObservabilityFileSystemHandle>();
	return internal_filesystem->SeekPosition(*observability_file_handle.internal_file_handle);
}
void ObservabilityFileSystem::FileSync(FileHandle &handle) {
	auto &observability_file_handle = handle.Cast<ObservabilityFileSystemHandle>();
	internal_filesystem->FileSync(*observability_file_handle.internal_file_handle);
}
bool ObservabilityFileSystem::OnDiskFile(FileHandle &handle) {
	auto &observability_file_handle = handle.Cast<ObservabilityFileSystemHandle>();
	return internal_filesystem->OnDiskFile(*observability_file_handle.internal_file_handle);
}
bool ObservabilityFileSystem::Trim(FileHandle &handle, idx_t offset_bytes, idx_t length_bytes) {
	auto &observability_file_handle = handle.Cast<ObservabilityFileSystemHandle>();
	return internal_filesystem->Trim(*observability_file_handle.internal_file_handle, offset_bytes, length_bytes);
}
FileType ObservabilityFileSystem::GetFileType(FileHandle &handle) {
	auto &observability_file_handle = handle.Cast<ObservabilityFileSystemHandle>();
	return internal_filesystem->GetFileType(*observability_file_handle.internal_file_handle);
}

} // namespace duckdb
