#include "fake_filesystem.hpp"

#include "duckdb/common/string.hpp"
#include "duckdb/common/string_util.hpp"
#include "no_destructor.hpp"

namespace duckdb {

namespace {
string BuildFakeFilesystemPrefix() {
#ifdef _WIN32
	const char *base = "C:\\\\Windows\\\\Temp";
	const char sep = '\\';
#else
	const char *base = "/tmp";
	const char sep = '/';
#endif
	string prefix = string(base);
	if (!prefix.empty() && prefix.back() != sep) {
		prefix.push_back(sep);
	}
	prefix += "cache_httpfs_fake_filesystem";
	return prefix;
}

const string &GetFakeFilesystemPrefix() {
	static const NoDestructor<string> kPrefix {BuildFakeFilesystemPrefix()};
	return *kPrefix;
}
} // namespace

ObserveHttpfsFakeFsHandle::ObserveHttpfsFakeFsHandle(string path, unique_ptr<FileHandle> internal_file_handle_p,
                                                     ObserveHttpfsFakeFileSystem &fs)
    : FileHandle(fs, std::move(path), internal_file_handle_p->GetFlags()),
      internal_file_handle(std::move(internal_file_handle_p)) {
}
ObserveHttpfsFakeFileSystem::ObserveHttpfsFakeFileSystem() : local_filesystem(LocalFileSystem::CreateLocal()) {
	local_filesystem->CreateDirectory(GetFakeFilesystemPrefix());
}
bool ObserveHttpfsFakeFileSystem::CanHandleFile(const string &path) {
	return StringUtil::StartsWith(path, GetFakeFilesystemPrefix());
}

unique_ptr<FileHandle> ObserveHttpfsFakeFileSystem::OpenFile(const string &path, FileOpenFlags flags,
                                                             optional_ptr<FileOpener> opener) {
	auto file_handle = local_filesystem->OpenFile(path, flags, opener);
	return make_uniq<ObserveHttpfsFakeFsHandle>(path, std::move(file_handle), *this);
}
void ObserveHttpfsFakeFileSystem::Read(FileHandle &handle, void *buffer, int64_t nr_bytes, idx_t location) {
	auto &local_filesystem_handle = handle.Cast<ObserveHttpfsFakeFsHandle>().internal_file_handle;
	local_filesystem->Read(*local_filesystem_handle, buffer, nr_bytes, location);
}
int64_t ObserveHttpfsFakeFileSystem::Read(FileHandle &handle, void *buffer, int64_t nr_bytes) {
	auto &local_filesystem_handle = handle.Cast<ObserveHttpfsFakeFsHandle>().internal_file_handle;
	return local_filesystem->Read(*local_filesystem_handle, buffer, nr_bytes);
}

void ObserveHttpfsFakeFileSystem::Write(FileHandle &handle, void *buffer, int64_t nr_bytes, idx_t location) {
	auto &local_filesystem_handle = handle.Cast<ObserveHttpfsFakeFsHandle>().internal_file_handle;
	local_filesystem->Write(*local_filesystem_handle, buffer, nr_bytes, location);
}
int64_t ObserveHttpfsFakeFileSystem::Write(FileHandle &handle, void *buffer, int64_t nr_bytes) {
	auto &local_filesystem_handle = handle.Cast<ObserveHttpfsFakeFsHandle>().internal_file_handle;
	return local_filesystem->Write(*local_filesystem_handle, buffer, nr_bytes);
}
int64_t ObserveHttpfsFakeFileSystem::GetFileSize(FileHandle &handle) {
	auto &local_filesystem_handle = handle.Cast<ObserveHttpfsFakeFsHandle>().internal_file_handle;
	return local_filesystem->GetFileSize(*local_filesystem_handle);
}
void ObserveHttpfsFakeFileSystem::FileSync(FileHandle &handle) {
	auto &local_filesystem_handle = handle.Cast<ObserveHttpfsFakeFsHandle>().internal_file_handle;
	local_filesystem->FileSync(*local_filesystem_handle);
}

void ObserveHttpfsFakeFileSystem::Seek(FileHandle &handle, idx_t location) {
	auto &local_filesystem_handle = handle.Cast<ObserveHttpfsFakeFsHandle>().internal_file_handle;
	local_filesystem->Seek(*local_filesystem_handle, location);
}
idx_t ObserveHttpfsFakeFileSystem::SeekPosition(FileHandle &handle) {
	auto &local_filesystem_handle = handle.Cast<ObserveHttpfsFakeFsHandle>().internal_file_handle;
	return local_filesystem->SeekPosition(*local_filesystem_handle);
}
bool ObserveHttpfsFakeFileSystem::Trim(FileHandle &handle, idx_t offset_bytes, idx_t length_bytes) {
	auto &local_filesystem_handle = handle.Cast<ObserveHttpfsFakeFsHandle>().internal_file_handle;
	return local_filesystem->Trim(*local_filesystem_handle, offset_bytes, length_bytes);
}
timestamp_t ObserveHttpfsFakeFileSystem::GetLastModifiedTime(FileHandle &handle) {
	auto &local_filesystem_handle = handle.Cast<ObserveHttpfsFakeFsHandle>().internal_file_handle;
	return local_filesystem->GetLastModifiedTime(*local_filesystem_handle);
}
FileType ObserveHttpfsFakeFileSystem::GetFileType(FileHandle &handle) {
	auto &local_filesystem_handle = handle.Cast<ObserveHttpfsFakeFsHandle>().internal_file_handle;
	return local_filesystem->GetFileType(*local_filesystem_handle);
}
void ObserveHttpfsFakeFileSystem::Truncate(FileHandle &handle, int64_t new_size) {
	auto &local_filesystem_handle = handle.Cast<ObserveHttpfsFakeFsHandle>().internal_file_handle;
	local_filesystem->Truncate(*local_filesystem_handle, new_size);
}
bool ObserveHttpfsFakeFileSystem::OnDiskFile(FileHandle &handle) {
	auto &local_filesystem_handle = handle.Cast<ObserveHttpfsFakeFsHandle>().internal_file_handle;
	return local_filesystem->OnDiskFile(*local_filesystem_handle);
}

} // namespace duckdb
