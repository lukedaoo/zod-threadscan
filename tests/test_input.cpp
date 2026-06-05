#define THREADSCAN_INPUT_IMPLEMENTATION
#include <gtest/gtest.h>

#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include "../threadscan_input.h"
#include "../threadscan_types.h"

namespace ts = threadscan;
struct CinInject {
    std::istringstream ss;
    std::streambuf* old;
    explicit CinInject(std::string input)
        : ss(std::move(input)), old(std::cin.rdbuf(ss.rdbuf())) {}
    ~CinInject() {
        std::cin.rdbuf(old);
        std::cin.clear();
    }
};

struct CerrCapture {
    std::ostringstream ss;
    std::streambuf* old;
    CerrCapture() : old(std::cerr.rdbuf(ss.rdbuf())) {}
    ~CerrCapture() { std::cerr.rdbuf(old); }
    std::string str() const { return ss.str(); }
};

struct CoutSuppress {
    std::ostringstream ss;
    std::streambuf* old;
    CoutSuppress() : old(std::cout.rdbuf(ss.rdbuf())) {}
    ~CoutSuppress() { std::cout.rdbuf(old); }
};

struct Argv {
    std::vector<char*> v;
    explicit Argv(std::initializer_list<const char*> args) {
        for (const char* s : args) v.push_back(const_cast<char*>(s));
        v.push_back(nullptr);
    }
    int argc() const { return static_cast<int>(v.size()) - 1; }  // NOLINT
    char** argv() { return v.data(); }
};

class WithTmpDir : public ::testing::Test {
  protected:
    std::filesystem::path tmpdir;
    std::string tmpdir_str;

    void SetUp() override {
        tmpdir = std::filesystem::temp_directory_path() / "ts_test";
        std::filesystem::create_directories(tmpdir);
        tmpdir_str = tmpdir.string();
    }
    void TearDown() override { std::filesystem::remove_all(tmpdir); }
};

class ConsoleInputTest : public WithTmpDir {};
class CommandLineTest : public WithTmpDir {};
class GetParamsTest : public WithTmpDir {};

TEST_F(ConsoleInputTest, ValidInputs_Success) {
    CinInject cin(tmpdir_str + "\n5\nfoo\n2\n");
    CoutSuppress cout_sup;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_console(p));
    EXPECT_EQ(p.path_dir, tmpdir_str);
    EXPECT_EQ(p.number_of_files_to_search, 5U);
    EXPECT_EQ(p.word_to_search, "foo");
    EXPECT_EQ(p.number_of_threads, 2U);
}

TEST_F(ConsoleInputTest, ZeroFilesAndThreads_Success) {
    CinInject cin(tmpdir_str + "\n0\nfoo\n0\n");
    CoutSuppress cout_sup;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_console(p));
    EXPECT_EQ(p.number_of_files_to_search, 0U);
    EXPECT_EQ(p.number_of_threads, 0U);
}

TEST_F(ConsoleInputTest, InvalidDirThenValid_Retries) {
    CinInject cin("/nonexistent_path_xyz\n" + tmpdir_str + "\n1\nfoo\n1\n");
    CoutSuppress cout_sup;
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_console(p));
    EXPECT_EQ(p.path_dir, tmpdir_str);
}

TEST_F(ConsoleInputTest, EmptyWord_Retries) {
    CinInject cin(tmpdir_str + "\n1\n\nfoo\n1\n");
    CoutSuppress cout_sup;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_console(p));
    EXPECT_EQ(p.word_to_search, "foo");
}

TEST_F(ConsoleInputTest, EofDuringDir_ReturnsFalse) {
    CinInject cin("");
    CoutSuppress cout_sup;
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_FALSE(ts::get_scan_params_from_console(p));
}

TEST_F(ConsoleInputTest, EofDuringFiles_ReturnsFalse) {
    CinInject cin(tmpdir_str + "\n");
    CoutSuppress cout_sup;
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_FALSE(ts::get_scan_params_from_console(p));
}

TEST_F(ConsoleInputTest, NonIntFiles_Retries) {
    CinInject cin(tmpdir_str + "\nabc\n5\nfoo\n1\n");
    CoutSuppress cout_sup;
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_console(p));
    EXPECT_EQ(p.number_of_files_to_search, 5u);
}

TEST_F(CommandLineTest, AllArgs_Success) {
    Argv a{"prog",           "--path", tmpdir_str.c_str(), "--word", "foo",
           "--num-of-files", "10",     "--num-of-threads", "4"};
    CoutSuppress cout_sup;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_command_line(a.argc(), a.argv(), p));
    EXPECT_EQ(p.path_dir, tmpdir_str);
    EXPECT_EQ(p.word_to_search, "foo");
    EXPECT_EQ(p.number_of_files_to_search, 10U);
    EXPECT_EQ(p.number_of_threads, 4U);
}

TEST_F(CommandLineTest, OnlyProgramName_Success) {
    Argv a{"prog"};
    CoutSuppress cout_sup;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_command_line(a.argc(), a.argv(), p));
    EXPECT_TRUE(p.path_dir.empty());
    EXPECT_TRUE(p.word_to_search.empty());
}

TEST_F(CommandLineTest, MissingValueForPath_ReturnsFalse) {
    Argv a{"prog", "--path"};
    CoutSuppress cout_sup;
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_FALSE(ts::get_scan_params_from_command_line(a.argc(), a.argv(), p));
    EXPECT_NE(cerr.str().find("requires a value"), std::string::npos);
}

TEST_F(CommandLineTest, MissingValueForWord_ReturnsFalse) {
    Argv a{"prog", "--word"};
    CoutSuppress cout_sup;
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_FALSE(ts::get_scan_params_from_command_line(a.argc(), a.argv(), p));
    EXPECT_NE(cerr.str().find("requires a value"), std::string::npos);
}

TEST_F(CommandLineTest, InvalidIntForFiles_UsesDefault) {
    Argv a{"prog", "--num-of-files", "abc"};
    CoutSuppress cout_sup;
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_command_line(a.argc(), a.argv(), p));
    EXPECT_EQ(p.number_of_files_to_search, DEFAULT_NUM_OF_FILES_TO_SEARCH);
    EXPECT_NE(cerr.str().find("invalid integer for '--num-of-files'"), std::string::npos);
}

TEST_F(CommandLineTest, InvalidIntForThreads_UsesDefault) {
    Argv a{"prog", "--num-of-threads", "xyz"};
    CoutSuppress cout_sup;
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_command_line(a.argc(), a.argv(), p));
    EXPECT_EQ(p.number_of_threads, DEFAULT_NUM_OF_THREAD_USAGE);
    EXPECT_NE(cerr.str().find("invalid integer for '--num-of-threads'"), std::string::npos);
}

TEST_F(CommandLineTest, FilesNotSet_UsesDefaultAndLogs) {
    Argv a{"prog", "--path", tmpdir_str.c_str(), "--word", "foo",
           "--num-of-threads", "4"};
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_command_line(a.argc(), a.argv(), p));
    EXPECT_EQ(p.number_of_files_to_search, DEFAULT_NUM_OF_FILES_TO_SEARCH);
    EXPECT_EQ(p.number_of_threads, 4U);
    EXPECT_NE(cerr.str().find("num-of-files not set"), std::string::npos);
}

TEST_F(CommandLineTest, ThreadsNotSet_UsesDefaultAndLogs) {
    Argv a{"prog", "--path", tmpdir_str.c_str(), "--word", "foo",
           "--num-of-files", "5"};
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_command_line(a.argc(), a.argv(), p));
    EXPECT_EQ(p.number_of_threads, DEFAULT_NUM_OF_THREAD_USAGE);
    EXPECT_EQ(p.number_of_files_to_search, 5U);
    EXPECT_NE(cerr.str().find("num-of-threads not set"), std::string::npos);
}

TEST_F(CommandLineTest, NeitherOptionalSet_BothUseDefaults) {
    Argv a{"prog", "--path", tmpdir_str.c_str(), "--word", "foo"};
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_command_line(a.argc(), a.argv(), p));
    EXPECT_EQ(p.number_of_files_to_search, DEFAULT_NUM_OF_FILES_TO_SEARCH);
    EXPECT_EQ(p.number_of_threads, DEFAULT_NUM_OF_THREAD_USAGE);
    EXPECT_NE(cerr.str().find("num-of-files not set"), std::string::npos);
    EXPECT_NE(cerr.str().find("num-of-threads not set"), std::string::npos);
}

TEST_F(CommandLineTest, DuplicateFlag_LastWins) {
    Argv a{"prog",   "--path", "/tmp",           "--path", tmpdir_str.c_str(),
           "--word", "w",      "--num-of-files", "0",      "--num-of-threads",
           "0"};
    CoutSuppress cout_sup;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_command_line(a.argc(), a.argv(), p));
    EXPECT_EQ(p.path_dir, tmpdir_str);
}

TEST_F(CommandLineTest, UnknownFlag_LogsAndContinues) {
    Argv a{"prog", "--foo",          "--path", tmpdir_str.c_str(), "--word",
           "w",    "--num-of-files", "0",      "--num-of-threads", "0"};
    CoutSuppress cout_sup;
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_command_line(a.argc(), a.argv(), p));
    EXPECT_NE(cerr.str().find("unknown argument"), std::string::npos);
}

TEST_F(CommandLineTest, ModeFlags_Skipped) {
    Argv a{
        "prog",   "--console", "--no-console",   "--path", tmpdir_str.c_str(),
        "--word", "w",         "--num-of-files", "0",      "--num-of-threads",
        "0"};
    CoutSuppress cout_sup;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_scan_params_from_command_line(a.argc(), a.argv(), p));
    EXPECT_EQ(p.path_dir, tmpdir_str);
}

TEST_F(CommandLineTest, EmptyWordValue_ReturnsFalse) {
    Argv a{"prog", "--word", ""};
    CoutSuppress cout_sup;
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_FALSE(ts::get_scan_params_from_command_line(a.argc(), a.argv(), p));
    EXPECT_NE(cerr.str().find("requires a value"), std::string::npos);
}

TEST_F(GetParamsTest, NoConsole_AllArgs_Success) {
    Argv a{"prog", "--no-console",   "--path", tmpdir_str.c_str(), "--word",
           "foo",  "--num-of-files", "1",      "--num-of-threads", "1"};
    CoutSuppress cout_sup;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_params(a.argc(), a.argv(), p));
    EXPECT_EQ(p.path_dir, tmpdir_str);
    EXPECT_EQ(p.word_to_search, "foo");
}

TEST_F(GetParamsTest, NoConsole_MissingPath_ReturnsFalse) {
    Argv a{"prog", "--no-console", "--word", "foo"};
    CoutSuppress cout_sup;
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_FALSE(ts::get_params(a.argc(), a.argv(), p));
    EXPECT_NE(cerr.str().find("must specify required"), std::string::npos);
}

TEST_F(GetParamsTest, NoConsole_OnlyPathAndWord_Success) {
    Argv a{"prog", "--no-console", "--path", tmpdir_str.c_str(), "--word", "foo"};
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_params(a.argc(), a.argv(), p));
    EXPECT_EQ(p.path_dir, tmpdir_str);
    EXPECT_EQ(p.word_to_search, "foo");
    EXPECT_EQ(p.number_of_files_to_search, DEFAULT_NUM_OF_FILES_TO_SEARCH);
    EXPECT_EQ(p.number_of_threads, DEFAULT_NUM_OF_THREAD_USAGE);
}

TEST_F(GetParamsTest, NoConsole_NoDataArgs_ReturnsFalse) {
    Argv a{"prog", "--no-console"};
    CoutSuppress cout_sup;
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_FALSE(ts::get_params(a.argc(), a.argv(), p));
}

TEST_F(GetParamsTest, Console_ValidStdin_Success) {
    Argv a{"prog", "--console"};
    CinInject cin(tmpdir_str + "\n1\nfoo\n1\n");
    CoutSuppress cout_sup;
    ts::ScanParams p;
    ASSERT_TRUE(ts::get_params(a.argc(), a.argv(), p));
    EXPECT_EQ(p.path_dir, tmpdir_str);
    EXPECT_EQ(p.word_to_search, "foo");
}

TEST_F(GetParamsTest, Console_PlusDataArgs_ReturnsFalse) {
    Argv a{"prog", "--console", "--path", "/tmp"};
    CoutSuppress cout_sup;
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_FALSE(ts::get_params(a.argc(), a.argv(), p));
    EXPECT_NE(cerr.str().find("console input is active"), std::string::npos);
}

TEST_F(GetParamsTest, Console_ErrorMentions_NoConsole) {
    Argv a{"prog", "--console", "--word", "foo"};
    CoutSuppress cout_sup;
    CerrCapture cerr;
    ts::ScanParams p;
    ASSERT_FALSE(ts::get_params(a.argc(), a.argv(), p));
    EXPECT_NE(cerr.str().find("--no-console"), std::string::npos);
}
