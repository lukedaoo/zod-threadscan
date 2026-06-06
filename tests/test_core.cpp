#define THREADSCAN_CORE_IMPLEMENTATION
#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

#include "../threadscan_core.h"
#include "../threadscan_types.h"

namespace ts = threadscan;
using SE = ts::ScanError;

struct SuppressOut {
    std::ostringstream buf;
    std::streambuf* orig = std::cout.rdbuf(buf.rdbuf());
    ~SuppressOut() { std::cout.rdbuf(orig); }
};

struct ScanParamsCase {
    std::string label;
    bool use_tmp_dir;
    std::string path_override;
    std::string word;
    std::size_t num_files = 0;
    std::size_t num_threads = 0;
    bool expect_ok;
    std::optional<SE> expect_err;
};

class ScanParamsTest : public ::testing::TestWithParam<ScanParamsCase> {
  protected:
    static std::filesystem::path s_tmpdir;
    static std::string s_tmpdir_str;

    static void SetUpTestSuite() {
        s_tmpdir = std::filesystem::temp_directory_path() / "ts_core_test";
        std::filesystem::create_directories(s_tmpdir);
        s_tmpdir_str = s_tmpdir.string();
        std::ofstream{s_tmpdir / "dummy.txt"};
    }
    static void TearDownTestSuite() { std::filesystem::remove_all(s_tmpdir); }
};
std::filesystem::path ScanParamsTest::s_tmpdir;
std::string ScanParamsTest::s_tmpdir_str;

TEST_P(ScanParamsTest, ValidateInput) {
    const auto& c = GetParam();
    std::string path = c.use_tmp_dir ? s_tmpdir_str : c.path_override;
    ts::ScanParams p{
        .number_of_files_to_search = c.num_files,
        .number_of_threads = c.num_threads,
        .path_dir = path,
        .word_to_search = c.word,
    };
    SuppressOut suppress;
    auto result = ts::scan(p);
    if (c.expect_ok) {
        EXPECT_TRUE(result.has_value()) << "label: " << c.label;
    } else {
        ASSERT_FALSE(result.has_value()) << "label: " << c.label;
        EXPECT_EQ(result.error(), *c.expect_err) << "label: " << c.label;
    }
}

static const std::size_t kMaxSz = std::numeric_limits<std::size_t>::max();

INSTANTIATE_TEST_SUITE_P(
    ScanParamsValidation, ScanParamsTest,
    ::testing::Values(
        // ----- valid -----
        ScanParamsCase{"ValidDir_BasicWord", true, "", "foo", 0, 0, true,
                       std::nullopt},
        ScanParamsCase{"ValidDir_ZeroCounts", true, "", "word", 0, 0, true,
                       std::nullopt},
        ScanParamsCase{"ValidDir_MaxCounts", true, "", "x", kMaxSz, kMaxSz,
                       true, std::nullopt},
        ScanParamsCase{"ValidDir_WhitespaceWord", true, "", " ", 0, 0, true,
                       std::nullopt},
        ScanParamsCase{"ValidDir_SpecialCharWord", true, "", "a/b!#@", 0, 0,
                       true, std::nullopt},
        ScanParamsCase{"ValidDir_LongWord", true, "", std::string(4096, 'z'), 0,
                       0, true, std::nullopt},
        ScanParamsCase{"ValidDir_NumericWord", true, "", "12345", 0, 0, true,
                       std::nullopt},
        // ----- PATH_EMPTY -----
        ScanParamsCase{"EmptyPath", false, "", "foo", 0, 0, false,
                       SE::PATH_EMPTY},
        // ----- PATH_NOT_FOUND -----
        ScanParamsCase{"NonexistentPath", false, "/noexist_xyz", "foo", 0, 0,
                       false, SE::PATH_NOT_FOUND},
        ScanParamsCase{"WhitespacePath", false, "   ", "foo", 0, 0, false,
                       SE::PATH_NOT_FOUND},
        // ----- WORD_EMPTY -----
        ScanParamsCase{"EmptyWord", true, "", "", 0, 0, false, SE::WORD_EMPTY},
        ScanParamsCase{"EmptyWord_WithCounts", true, "", "", 5, 2, false,
                       SE::WORD_EMPTY}),
    [](const ::testing::TestParamInfo<ScanParamsCase>& info) {
        return info.param.label;
    });

class ScanTest : public ::testing::Test {
  protected:
    std::filesystem::path tmpdir;
    std::string tmpdir_str;
    std::filesystem::path tmpfile;
    std::string tmpfile_str;

    void SetUp() override {
        tmpdir = std::filesystem::temp_directory_path() / "ts_core_single";
        std::filesystem::create_directories(tmpdir);
        tmpdir_str = tmpdir.string();
        tmpfile = tmpdir / "sample.txt";
        std::ofstream f{tmpfile};
        tmpfile_str = tmpfile.string();
    }
    void TearDown() override { std::filesystem::remove_all(tmpdir); }
};

TEST_F(ScanTest, PathIsFile_ReturnsPathNotDirectory) {
    SuppressOut suppress;
    ts::ScanParams p{.path_dir = tmpfile_str, .word_to_search = "foo"};
    auto result = ts::scan(p);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SE::PATH_NOT_DIRECTORY);
}

TEST_F(ScanTest, DefaultConstruction_ReturnsPathEmpty) {
    SuppressOut suppress;
    ts::ScanParams p;
    auto result = ts::scan(p);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SE::PATH_EMPTY);
}

TEST_F(ScanTest, ValidDir_EmptyWord_ReturnsWordEmpty) {
    SuppressOut suppress;
    ts::ScanParams p{.path_dir = tmpdir_str, .word_to_search = ""};
    auto result = ts::scan(p);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SE::WORD_EMPTY);
}

TEST_F(ScanTest, PathCheckedBeforeWord) {
    // empty path -> PATH_EMPTY even when word is also empty
    SuppressOut suppress;
    ts::ScanParams p{.path_dir = "", .word_to_search = ""};
    auto result = ts::scan(p);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SE::PATH_EMPTY);
}

TEST(ScanCStrTest, NullPathDir_ReturnsNullArgument) {
    auto result = ts::scan(nullptr, "foo");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SE::NULL_ARGUMENT);
}

TEST(ScanCStrTest, NullWord_ReturnsNullArgument) {
    auto result = ts::scan("/tmp", nullptr);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SE::NULL_ARGUMENT);
}

TEST(ScanCStrTest, BothNull_ReturnsNullArgument) {
    auto result = ts::scan(nullptr, nullptr);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SE::NULL_ARGUMENT);
}
