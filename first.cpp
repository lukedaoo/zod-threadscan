#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#define THREADSCAN_CORE_IMPLEMENTATION
#include "config.h"
#include "threadscan_core.h"

// static bool parse_console_input(int argc, char* argv[]) {
//     bool console_input = ENABLE_CONSOLE_INPUT;
//     for (int i = 1; i < argc; ++i) {
//         const std::string arg = argv[i];
//         if (arg == "--console") {
//             console_input = true;
//         } else if (arg == "--no-console") {
//             console_input = false;
//         }
//     }
//     return console_input;
// }

bool get_str_console_input(std::string_view prompt, std::string& output) {
    std::string output_tmp;
    while (output_tmp.empty()) {
        std::cout << prompt;
        if (!std::getline(std::cin, output_tmp)) {
            return false;  // EOF or error
        }
    }
    output = std::move(output_tmp);
    return true;
}

// Prompt for a path, re-prompting until it names an existing directory.
bool get_dir_console_input(std::string_view prompt, std::string& output) {
    std::string tmp;
    while (true) {
        if (!get_str_console_input(prompt, tmp)) {
            return false;  // EOF or error — propagate
        }
        std::error_code ec;
        if (std::filesystem::is_directory(tmp, ec)) {
            output = std::move(tmp);
            return true;
        }
        std::cerr << "not a directory: " << tmp << "\n";
    }
}

std::string_view scan_error_to_string(threadscan::ScanError err) {
    switch (err) {
    case threadscan::ScanError::PATH_EMPTY:
        return "path_dir is empty";
    case threadscan::ScanError::PATH_NOT_DIRECTORY:
        return "path_dir is not a directory";
    case threadscan::ScanError::WORD_EMPTY:
        return "word_to_search is empty";
    case threadscan::ScanError::NULL_ARGUMENT:
        return "path_dir or word_to_search is null";
    }
    return "unknown error";
}

bool get_int_console_input(std::string_view prompt, int64_t& output,
                           std::pair<int64_t, int64_t> range) {
    int64_t output_tmp;
    bool valid = false;
    while (!valid) {
        std::cout << prompt;
        if (!(std::cin >> output_tmp)) {
            if (std::cin.eof()) {
                return false;  // EOF
            }
            std::cin.clear();  // bad input: clear failbit, drop line, re-prompt
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        valid = output_tmp >= range.first && output_tmp <= range.second;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    output = output_tmp;
    return true;
}

int main(int argc, char* argv[]) {
    bool console_input = ENABLE_CONSOLE_INPUT;
    // for (int i = 1; i < argc; ++i) {
    //     const std::string arg = argv[i];
    //     if (arg == "--console") {
    //         console_input = true;
    //     } else if (arg == "--no-console") {
    //         console_input = false;
    //     }
    // }

    // Defaults; overridden by stdin prompts when console input enabled.
    threadscan::ScanParams params{
        .number_of_files_to_search = DEFAULT_NUM_OF_FILES_TO_SEARCH,
        .number_of_threads = DEFAULT_NUM_OF_THREAD_USAGE,
        .path_dir = DEFAULT_SCAN_DIR,
        .word_to_search = "abc",
    };

    if (console_input) {
        constexpr int64_t MAX_VALUE_INT64 = std::numeric_limits<int64_t>::max();
        int64_t n = 0;

        if (!get_dir_console_input(
                "Enter the path of the folder that contains the files to "
                "search: ",
                params.path_dir)) {
            std::cerr << "Invalid path.\n";
            return 1;
        }

        if (!get_int_console_input(
                "Enter the number of files to search (or 0 to search all "
                "files in the directory): ",
                n, {0, MAX_VALUE_INT64})) {
            std::cerr << "Invalid number of files to search.\n";
            return 1;
        }

        params.number_of_files_to_search = static_cast<std::size_t>(n);

        if (!get_str_console_input("Enter the word to search for: ",
                                   params.word_to_search)) {
            std::cerr << "Invalid word to search.\n";
            return 1;
        }
        if (!get_int_console_input(
                "Enter the number of threads to use (or 0 for no threads): ", n,
                {0, MAX_VALUE_INT64})) {
            std::cerr << "Invalid number of threads.\n";
            return 1;
        }
        params.number_of_threads = static_cast<std::size_t>(n);
    }

    auto result = threadscan::scan(params);
    if (!result) {
        std::cerr << "scan failed: " << scan_error_to_string(result.error())
                  << "\n";
        return 1;
    }
    threadscan::ScanReport scan_report = std::move(*result);
    threadscan::make_report(
        scan_report, {.output_type = threadscan::ReportOutputType::CONSOLE});
    return 0;
}
