#include "observability_filesystem.hpp"

#include "duckdb/common/string_util.hpp"

namespace duckdb {

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
	internal_filesystem->Read(handle, buffer, nr_bytes, location);
}
int64_t ObservabilityFileSystem::Read(FileHandle &handle, void *buffer, int64_t nr_bytes) {
	return internal_filesystem->Read(handle, buffer, nr_bytes);
}
unique_ptr<FileHandle> ObservabilityFileSystem::OpenFile(const string &path, FileOpenFlags flags,
                                                         optional_ptr<FileOpener> opener) {
	const auto oper_id = metrics_collector.GenerateOperId();
	metrics_collector.RecordOperationStart(OperationLatencyHistogram::IoOperation::kOpen, oper_id);
	auto file_handle = internal_filesystem->OpenFile(path, flags, opener);
	metrics_collector.RecordOperationEnd(OperationLatencyHistogram::IoOperation::kOpen, oper_id);
	return file_handle;
}
int64_t ObservabilityFileSystem::GetFileSize(FileHandle &handle) {
	return internal_filesystem->GetFileSize(handle);
}
time_t ObservabilityFileSystem::GetLastModifiedTime(FileHandle &handle) {
	return internal_filesystem->GetLastModifiedTime(handle);
}
unique_ptr<FileHandle> ObservabilityFileSystem::OpenCompressedFile(unique_ptr<FileHandle> handle, bool write) {
	return internal_filesystem->OpenCompressedFile(std::move(handle), write);
}
void ObservabilityFileSystem::Write(FileHandle &handle, void *buffer, int64_t nr_bytes, idx_t location) {
	internal_filesystem->Write(handle, buffer, nr_bytes, location);
}
int64_t ObservabilityFileSystem::Write(FileHandle &handle, void *buffer, int64_t nr_bytes) {
	return internal_filesystem->Write(handle, buffer, nr_bytes);
}
void ObservabilityFileSystem::Truncate(FileHandle &handle, int64_t new_size) {
	internal_filesystem->Truncate(handle, new_size);
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
	return internal_filesystem->Glob(path, opener);
}
void ObservabilityFileSystem::Seek(FileHandle &handle, idx_t location) {
	internal_filesystem->Seek(handle, location);
}

} // namespace duckdb
