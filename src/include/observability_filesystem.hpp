// Base class for cache filesystem, including in-memory cache and on-disk cache.

#pragma once

#include "duckdb/common/file_system.hpp"
#include "duckdb/common/open_file_info.hpp"
#include "duckdb/common/shared_ptr.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/common/unique_ptr.hpp"
#include "metrics_collector.hpp"

#include <functional>
#include <mutex>
#include <tuple>

namespace duckdb {

// Forward declaration.
class ObservabilityFileSystem;

class ObservabilityFileSystemHandle : public FileHandle {
public:
	ObservabilityFileSystemHandle(unique_ptr<FileHandle> internal_file_handle_p, ObservabilityFileSystem &fs);
	~ObservabilityFileSystemHandle() = default;

	void Close() override {
	}

	unique_ptr<FileHandle> internal_file_handle;
};

class ObservabilityFileSystem : public FileSystem {
public:
	explicit ObservabilityFileSystem(unique_ptr<FileSystem> internal_filesystem_p)
	    : internal_filesystem(std::move(internal_filesystem_p)) {
	}
	~ObservabilityFileSystem() override {
	}

	// Get the internal filesystem for observabolity filesystem.
	FileSystem *GetInternalFileSystem() const {
		return internal_filesystem.get();
	}

	// Clear observability data.
	void ClearObservabilityData();
	// Get human-readable metrics stats.
	// If no stats collected, which means no interested IO operations for current filesystem.
	string GetHumanReadableStats();

	// Doesn't update file offset (which acts as `PRead` semantics).
	void Read(FileHandle &handle, void *buffer, int64_t nr_bytes, idx_t location) override;
	// Does update file offset (which acts as `Read` semantics).
	int64_t Read(FileHandle &handle, void *buffer, int64_t nr_bytes) override;
	unique_ptr<FileHandle> OpenFile(const string &path, FileOpenFlags flags,
	                                optional_ptr<FileOpener> opener = nullptr) override;
	string GetName() const override;
	// Get file size.
	int64_t GetFileSize(FileHandle &handle) override;
	// Get last modification timestamp.
	timestamp_t GetLastModifiedTime(FileHandle &handle) override;
	unique_ptr<FileHandle> OpenCompressedFile(QueryContext context, unique_ptr<FileHandle> handle, bool write) override;
	void Write(FileHandle &handle, void *buffer, int64_t nr_bytes, idx_t location) override;
	int64_t Write(FileHandle &handle, void *buffer, int64_t nr_bytes) override;
	void Truncate(FileHandle &handle, int64_t new_size) override;
	bool DirectoryExists(const string &directory, optional_ptr<FileOpener> opener = nullptr) override;
	void CreateDirectory(const string &directory, optional_ptr<FileOpener> opener = nullptr) override;
	void RemoveDirectory(const string &directory, optional_ptr<FileOpener> opener = nullptr) override;
	bool ListFiles(const string &directory, const std::function<void(const string &, bool)> &callback,
	               FileOpener *opener = nullptr) override;
	void MoveFile(const string &source, const string &target, optional_ptr<FileOpener> opener = nullptr) override;
	bool FileExists(const string &filename, optional_ptr<FileOpener> opener = nullptr) override;
	void RemoveFile(const string &filename, optional_ptr<FileOpener> opener = nullptr) override;
	vector<OpenFileInfo> Glob(const string &path, FileOpener *opener = nullptr) override;
	void Seek(FileHandle &handle, idx_t location) override;
	void Reset(FileHandle &handle) override;
	idx_t SeekPosition(FileHandle &handle) override;
	FileType GetFileType(FileHandle &handle) override;
	void FileSync(FileHandle &handle) override;
	bool OnDiskFile(FileHandle &handle) override;
	bool Trim(FileHandle &handle, idx_t offset_bytes, idx_t length_bytes) override;
	bool IsManuallySet() override {
		return true;
	}

	// =============================================
	// Delegate into internal filesystem instance.
	// =============================================
	//
	bool IsPipe(const string &filename, optional_ptr<FileOpener> opener = nullptr) override {
		return internal_filesystem->IsPipe(filename, opener);
	}
	string GetHomeDirectory() override {
		return internal_filesystem->GetHomeDirectory();
	}
	string ExpandPath(const string &path) override {
		return internal_filesystem->ExpandPath(path);
	}
	string PathSeparator(const string &path) override {
		return internal_filesystem->PathSeparator(path);
	}
	void RegisterSubSystem(unique_ptr<FileSystem> sub_fs) override {
		internal_filesystem->RegisterSubSystem(std::move(sub_fs));
	}
	void RegisterSubSystem(FileCompressionType compression_type, unique_ptr<FileSystem> fs) override {
		internal_filesystem->RegisterSubSystem(compression_type, std::move(fs));
	}
	void UnregisterSubSystem(const string &name) override {
		internal_filesystem->UnregisterSubSystem(name);
	}
	vector<string> ListSubSystems() override {
		return internal_filesystem->ListSubSystems();
	}
	bool CanHandleFile(const string &fpath) override {
		return internal_filesystem->CanHandleFile(fpath);
	}
	bool CanSeek() override {
		return internal_filesystem->CanSeek();
	}
	void SetDisabledFileSystems(const vector<string> &names) override {
		internal_filesystem->SetDisabledFileSystems(names);
	}

protected:
	// Because extended version of open and list are all "protected" visibility, which cannot access with the
	// [`internal_filesystem`] variable, so we explicitly disable the extended version and fallback to normal one.
	bool SupportsOpenFileExtended() const override {
		return false;
	}
	bool SupportsListFilesExtended() const override {
		return false;
	}

private:
	// Used to access remote files.
	unique_ptr<FileSystem> internal_filesystem;
	// Overall histogram.
	MetricsCollector metrics_collector;
};

} // namespace duckdb
