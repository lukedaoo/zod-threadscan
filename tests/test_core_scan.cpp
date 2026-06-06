#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <numeric>
#include <sstream>
#include <string>

#include "../threadscan_core.h"
#include "../threadscan_types.h"

namespace ts = threadscan;

struct SuppressOut {
    std::ostringstream buf;
    std::streambuf* orig = std::cout.rdbuf(buf.rdbuf());
    ~SuppressOut() { std::cout.rdbuf(orig); }
};

static size_t total_occurrences(const ts::ScanResult& rep) {
    return std::accumulate(rep.final_results.begin(), rep.final_results.end(),
                           size_t{0},
                           [](size_t s, const ts::FileScanResult& r) {
                               return s + r.occurrences;
                           });
}

class ScanBehaviorTest : public ::testing::Test {
  protected:
    std::filesystem::path tmpdir;
    std::string tmpdir_str;

    void SetUp() override {
        tmpdir = std::filesystem::temp_directory_path() / "ts_scan_behavior";
        std::filesystem::create_directories(tmpdir);
        tmpdir_str = tmpdir.string();
    }
    void TearDown() override { std::filesystem::remove_all(tmpdir); }

    void write_file(const std::string& name, const std::string& content) {
        std::ofstream f{tmpdir / name};
        f << content;
    }
};

TEST_F(ScanBehaviorTest, SingleThreaded_CountsOccurrences) {
    write_file("a.txt", "hello world\nhello again\n");
    write_file("b.txt", "no match here\n");

    SuppressOut suppress;
    ts::ScanParams p{.number_of_threads = 0,
                     .path_dir = tmpdir_str,
                     .word_to_search = "hello"};
    auto result = ts::scan(p);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(total_occurrences(*result), 2U);
}

class MultiThreadCountTest : public ScanBehaviorTest,
                             public ::testing::WithParamInterface<size_t> {};

TEST_P(MultiThreadCountTest, MatchesSingleThreadCount) {
    for (int i = 1; i <= 20; ++i) {
        write_file("f" + std::to_string(i) + ".txt",
                   "quasi quasi\nnot a match\nquasi\n");
    }

    SuppressOut suppress;
    auto run = [&](size_t threads) -> size_t {
        ts::ScanParams p{.number_of_threads = threads,
                         .path_dir = tmpdir_str,
                         .word_to_search = "quasi"};
        auto r = ts::scan(p);
        EXPECT_TRUE(r.has_value());
        return r.has_value() ? total_occurrences(*r) : 0U;
    };

    size_t single = run(0);
    size_t multi = run(GetParam());
    EXPECT_EQ(single, 60U);
    EXPECT_EQ(multi, 60U);
}

INSTANTIATE_TEST_SUITE_P(ThreadCounts, MultiThreadCountTest,
                         ::testing::Values(2, 5, 10, 15, 20));

TEST_F(ScanBehaviorTest, WordNotPresent_ZeroOccurrences) {
    write_file("a.txt", "the quick brown fox\n");

    SuppressOut suppress;
    ts::ScanParams p{.number_of_threads = 0,
                     .path_dir = tmpdir_str,
                     .word_to_search = "architecto"};
    auto result = ts::scan(p);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(total_occurrences(*result), 0U);
}

TEST_F(ScanBehaviorTest, MultipleOccurrencesOnOneLine) {
    write_file("a.txt", "aaa aaa aaa\n");

    SuppressOut suppress;
    ts::ScanParams p{.number_of_threads = 0,
                     .path_dir = tmpdir_str,
                     .word_to_search = "aaa"};
    auto result = ts::scan(p);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(total_occurrences(*result), 3U);
}
