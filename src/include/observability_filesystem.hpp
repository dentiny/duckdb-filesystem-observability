// Base class for cache filesystem, including in-memory cache and on-disk cache.

#pragma once

#include "duckdb/common/file_system.hpp"
#include "duckdb/common/open_file_info.hpp"
#include "duckdb/common/shared_ptr.hpp"
#include "duckdb/common/unique_ptr.hpp"

#include <functional>
#include <mutex>
#include <tuple>

namespace duckdb {

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
	// Doesn't update file offset (which acts as `PRead` semantics).
	void Read(FileHandle &handle, void *buffer, int64_t nr_bytes, idx_t location) override;
	// Does update file offset (which acts as `Read` semantics).
	int64_t Read(FileHandle &handle, void *buffer, int64_t nr_bytes) override;
	unique_ptr<FileHandle> OpenFile(const string &path, FileOpenFlags flags, optional_ptr<FileOpener> opener = nullptr);
	std::string GetName() const override;
	// Get file size.
	int64_t GetFileSize(FileHandle &handle);
	// Get last modification timestamp.
	time_t GetLastModifiedTime(FileHandle &handle) override;
	unique_ptr<FileHandle> OpenCompressedFile(unique_ptr<FileHandle> handle, bool write) override;
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

    // =============================================
    // Delegate into internal filesystem instance.
    // =============================================
    //
	bool Trim(FileHandle &handle, idx_t offset_bytes, idx_t length_bytes) override {
		return internal_filesystem->Trim(handle, offset_bytes, length_bytes);
	}
	FileType GetFileType(FileHandle &handle) override {
		return internal_filesystem->GetFileType(handle);
	}
	bool IsPipe(const string &filename, optional_ptr<FileOpener> opener = nullptr) override {
		return internal_filesystem->IsPipe(filename, opener);
	}
	void FileSync(FileHandle &handle) override {
		internal_filesystem->FileSync(handle);
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
	void Reset(FileHandle &handle) override {
		internal_filesystem->Reset(handle);
	}
	idx_t SeekPosition(FileHandle &handle) override {
		return internal_filesystem->SeekPosition(handle);
	}
	bool IsManuallySet() override {
        return internal_filesystem->IsManuallySet();
    }
	bool CanSeek() override {
		return internal_filesystem->CanSeek();
	}
	bool OnDiskFile(FileHandle &handle) override {
		return internal_filesystem->OnDiskFile(handle);
	}
	void SetDisabledFileSystems(const vector<string> &names) override {
		internal_filesystem->SetDisabledFileSystems(names);
	}

private:
	// Used to access remote files.
	unique_ptr<FileSystem> internal_filesystem;
};

} // namespace duckdb
