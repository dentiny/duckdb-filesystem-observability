#include "catch/catch.hpp"

#include "duckdb/common/multi_file/multi_file_list.hpp"
#include "duckdb/common/virtual_file_system.hpp"
#include "observability_filesystem.hpp"

using namespace duckdb;

namespace {

class MockFileSystemWithoutExtendedGlob : public FileSystem {
public:
	vector<OpenFileInfo> Glob(const string &path, FileOpener *opener = nullptr) override {
		++glob_invocation;
		return glob_results;
	}

	void SetExtendedGlobResults(vector<OpenFileInfo> results) {
		glob_results = std::move(results);
	}

	uint64_t GetGlobInvocation() const {
		return glob_invocation;
	}
	uint64_t GetGlobExtendedInvocation() const {
		return glob_extended_invocation;
	}

	string GetName() const override {
		return "mock_without_extended_glob";
	}

protected:
	bool SupportsGlobExtended() const override {
		return false;
	}

private:
	uint64_t glob_invocation = 0;
	uint64_t glob_extended_invocation = 0;
	vector<OpenFileInfo> glob_results;
};

class MockFileSystemWithExtendedGlob : public FileSystem {
public:
	vector<OpenFileInfo> Glob(const string &path, FileOpener *opener = nullptr) override {
		++glob_invocation;
		return {OpenFileInfo(path)};
	}

	void SetExtendedGlobResults(vector<OpenFileInfo> results) {
		extended_glob_results = std::move(results);
	}

	uint64_t GetGlobInvocation() const {
		return glob_invocation;
	}
	uint64_t GetGlobExtendedInvocation() const {
		return glob_extended_invocation;
	}

	string GetName() const override {
		return "mock_with_extended_glob";
	}

	bool SupportsGlobExtended() const override {
		return true;
	}

protected:
	unique_ptr<MultiFileList> GlobFilesExtended(const string &path, const FileGlobInput &input,
	                                            optional_ptr<FileOpener> opener) override {
		++glob_extended_invocation;
		return make_uniq<SimpleMultiFileList>(std::move(extended_glob_results));
	}

private:
	uint64_t glob_invocation = 0;
	uint64_t glob_extended_invocation = 0;
	vector<OpenFileInfo> extended_glob_results;
};
} // namespace

TEST_CASE("Test Glob with GlobFilesExtended unsupported", "[glob test]") {
	VirtualFileSystem vfs;
	auto mock_filesystem = make_uniq<MockFileSystemWithoutExtendedGlob>();
	auto *mock_ptr = mock_filesystem.get();

	vector<OpenFileInfo> glob_results;
	glob_results.emplace_back(OpenFileInfo("s3://bucket/snapshots/file1.parquet"));
	glob_results.emplace_back(OpenFileInfo("s3://bucket/snapshots/file2.parquet"));
	mock_filesystem->SetExtendedGlobResults(std::move(glob_results));

	auto observability_filesystem = make_uniq<ObservabilityFileSystem>(std::move(mock_filesystem), vfs);
	auto results = observability_filesystem->Glob("s3://bucket/snapshots/*.parquet");
	REQUIRE(mock_ptr->GetGlobInvocation() == 1);
	REQUIRE(results.size() == 2);
	REQUIRE(results[0].path == "s3://bucket/snapshots/file1.parquet");
	REQUIRE(results[1].path == "s3://bucket/snapshots/file2.parquet");
}

// Regression test: must use GlobFilesExtended when the internal filesystem supports it.
// See: https://github.com/dentiny/duck-read-cache-fs/pull/477
TEST_CASE("Test Glob uses GlobFilesExtended", "[glob test]") {
	VirtualFileSystem vfs;
	auto mock_filesystem = make_uniq<MockFileSystemWithExtendedGlob>();
	auto *mock_ptr = mock_filesystem.get();

	vector<OpenFileInfo> glob_results;
	glob_results.emplace_back(OpenFileInfo("s3://bucket/snapshots/file1.parquet"));
	glob_results.emplace_back(OpenFileInfo("s3://bucket/snapshots/file2.parquet"));
	mock_filesystem->SetExtendedGlobResults(std::move(glob_results));

	auto observability_filesystem = make_uniq<ObservabilityFileSystem>(std::move(mock_filesystem), vfs);
	auto results = observability_filesystem->Glob("s3://bucket/snapshots/*.parquet");
	REQUIRE(mock_ptr->GetGlobInvocation() == 0);
	REQUIRE(mock_ptr->GetGlobExtendedInvocation() == 1);
	REQUIRE(results.size() == 2);
	REQUIRE(results[0].path == "s3://bucket/snapshots/file1.parquet");
	REQUIRE(results[1].path == "s3://bucket/snapshots/file2.parquet");
}
