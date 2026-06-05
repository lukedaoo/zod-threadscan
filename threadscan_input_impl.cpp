#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "config.h"
#include "threadscan_types.h"

namespace threadscan {

namespace {

bool get_str_console_input(std::string_view prompt, std::string& output) {
    std::string tmp;
    while (tmp.empty()) {
        std::cout << prompt;
        if (!std::getline(std::cin, tmp)) {
            return false;
        }
    }
    output = std::move(tmp);
    return true;
}

bool get_dir_console_input(std::string_view prompt, std::string& output) {
    std::string tmp;
    do {
        if (!get_str_console_input(prompt, tmp)) {
            return false;
        }
        std::error_code ec;
        if (std::filesystem::is_directory(tmp, ec)) {
            output = std::move(tmp);
            return true;
        }
        if (ec) {
            std::cerr << "cannot access '" << tmp << "': " << ec.message()
                      << "\n";
        } else {
            std::cerr << "not a directory: " << tmp << "\n";
        }
    } while (true);
}

bool get_int_console_input(std::string_view prompt, int64_t& output,
                           std::pair<int64_t, int64_t> range) {
    std::string line;
    int64_t tmp;

    while (true) {
        std::cout << prompt;
        if (!std::getline(std::cin, line)) {
            return false;
        }

        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        if (!(ss >> tmp)) {
            continue;
        }

        if (tmp >= range.first && tmp <= range.second) {
            output = tmp;
            return true;
        }
    }
}

bool parse_arg_to_str(const char* arg_name, const char* arg_raw_value,
                      std::string& output) {
    if (arg_raw_value == nullptr || std::string_view(arg_raw_value).empty()) {
        std::cerr << "argument '" << arg_name << "' requires a value\n";
        return false;
    }
    output = arg_raw_value;
    return true;
}

bool parse_arg_to_int(const char* arg_name, const char* arg_raw_value,
                      int64_t& output) {
    if (arg_raw_value == nullptr || std::string_view(arg_raw_value).empty()) {
        std::cerr << "argument '" << arg_name << "' requires a value\n";
        return false;
    }
    try {
        output = std::stoll(arg_raw_value);
        return true;
    } catch (const std::exception&) {
        std::cerr << "invalid integer for '" << arg_name << "': '"
                  << arg_raw_value << "'\n";
        return false;
    }
}

bool parse_required_string(const char* flag, int& i, int argc, char* argv[],
                           std::string& out) {
    if (i + 1 >= argc) {
        std::cerr << "'" << flag << "' requires a value\n";
        return false;
    }
    return parse_arg_to_str(flag, argv[++i], out);
}

void parse_optional_int64(const char* flag, int& i, int argc, char* argv[],
                          int64_t& out, bool& was_set) {
    if (i + 1 >= argc) {
        return;
    }
    int64_t tmp = out;
    if (!parse_arg_to_int(flag, argv[++i], tmp)) {
        return;
    }
    out = tmp;
    was_set = true;
}

enum ArgBits : uint8_t {
    NONE = 0,
    PATH = 1 << 0,
    FILES = 1 << 1,
    THREADS = 1 << 2,
    WORD = 1 << 3,
    ALL = PATH | FILES | THREADS | WORD
};
}  // namespace

bool get_scan_params_from_console(ScanParams& out) {
    constexpr int64_t MAX_INT64 = std::numeric_limits<int64_t>::max();

    std::string path_dir;
    if (!get_dir_console_input(
            "Enter the path of the folder that contains the files to search: ",
            path_dir)) {
        std::cerr << "invalid path.\n";
        return false;
    }

    int64_t num_files = DEFAULT_NUM_OF_FILES_TO_SEARCH;
    if (!get_int_console_input(
            "Enter the number of files to search (0 = all files): ", num_files,
            {0, MAX_INT64})) {
        std::cerr << "invalid number of files.\n";
        return false;
    }

    std::string word;
    if (!get_str_console_input("Enter the word to search for: ", word)) {
        std::cerr << "invalid word.\n";
        return false;
    }

    int64_t num_threads = DEFAULT_NUM_OF_THREAD_USAGE;
    if (!get_int_console_input(
            "Enter the number of threads to use (0 = no threads): ",
            num_threads, {0, MAX_INT64})) {
        std::cerr << "invalid number of threads.\n";
        return false;
    }

    out = ScanParams{
        .number_of_files_to_search = static_cast<std::size_t>(num_files),
        .number_of_threads = static_cast<std::size_t>(num_threads),
        .path_dir = std::move(path_dir),
        .word_to_search = std::move(word),
    };
    return true;
}

bool get_scan_params_from_command_line(int argc, char* argv[],
                                       ScanParams& out) {
    std::string path_dir;
    std::string word;
    int64_t num_files =
        DEFAULT_NUM_OF_FILES_TO_SEARCH < 0 ? 0 : DEFAULT_NUM_OF_FILES_TO_SEARCH;
    int64_t num_threads =
        DEFAULT_NUM_OF_THREAD_USAGE < 0 ? 0 : DEFAULT_NUM_OF_THREAD_USAGE;
    bool files_set = false;
    bool threads_set = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "--path") {
            if (!parse_required_string("--path", i, argc, argv, path_dir)) {
                return false;
            }
        } else if (arg == "--word") {
            if (!parse_required_string("--word", i, argc, argv, word)) {
                return false;
            }
        } else if (arg == "--num-of-files") {
            parse_optional_int64("--num-of-files", i, argc, argv, num_files,
                                 files_set);
        } else if (arg == "--num-of-threads") {
            parse_optional_int64("--num-of-threads", i, argc, argv, num_threads,
                                 threads_set);
        } else if (arg == "--console" || arg == "--no-console") {
            continue;
        } else {
            std::cerr << "unknown argument: " << arg << "\n";
        }
    }

    if (!files_set) {
        std::cerr << "num-of-files not set, using default: " << num_files
                  << "\n";
    }

    if (!threads_set) {
        std::cerr << "num-of-threads not set, using default: " << num_threads
                  << "\n";
    }

    out = {
        .number_of_files_to_search = static_cast<std::size_t>(num_files),
        .number_of_threads = static_cast<std::size_t>(num_threads),
        .path_dir = std::move(path_dir),
        .word_to_search = std::move(word),
    };
    return true;
}

bool get_params(int argc, char* argv[], ScanParams& out) {
    bool console_input = ENABLE_CONSOLE_INPUT;
    uint8_t mask = 0;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--console") {
            console_input = true;
        } else if (arg == "--no-console") {
            console_input = false;
        } else if (arg == "--path") {
            mask |= ArgBits::PATH;
        } else if (arg == "--num-of-files") {
            mask |= ArgBits::FILES;
        } else if (arg == "--num-of-threads") {
            mask |= ArgBits::THREADS;
        } else if (arg == "--word") {
            mask |= ArgBits::WORD;
        }
    }

    if (console_input && (mask != 0)) {
        std::cerr
            << "console input is active but data arguments were passed\n"
               "  to disable permanently: set ENABLE_CONSOLE_INPUT=false in "
               "config.h\n"
               "  to disable for this run: pass --no-console\n";
        return false;
    }

    if (console_input) {
        return get_scan_params_from_console(out);
    }

    if ((mask & (ArgBits::PATH | ArgBits::WORD)) !=
        (ArgBits::PATH | ArgBits::WORD)) {
        std::cerr << "must specify required arguments: --path, --word\n";
        return false;
    }

    return get_scan_params_from_command_line(argc, argv, out);
}

}  // namespace threadscan
