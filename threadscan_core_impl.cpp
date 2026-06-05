#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#include "config.h"
#include "threadscan_core.h"
#include "threadscan_types.h"

namespace threadscan {

namespace {

std::expected<void, ScanError> validate_scan_params(const ScanParams& params) {
    if (params.path_dir.empty()) {
        return std::unexpected(ScanError::PATH_EMPTY);
    }
    std::error_code ec;
    auto status = std::filesystem::status(params.path_dir, ec);
    if (ec) {
        if (ec == std::make_error_code(std::errc::permission_denied)) {
            return std::unexpected(ScanError::PERMISSION_DENIED);
        }
        if (ec == std::make_error_code(std::errc::no_such_file_or_directory)) {
            return std::unexpected(ScanError::PATH_NOT_FOUND);
        }
        return std::unexpected(ScanError::PATH_ACCESS_ERROR);
    }
    if (!std::filesystem::is_directory(status)) {
        return std::unexpected(ScanError::PATH_NOT_DIRECTORY);
    }
    if (params.word_to_search.empty()) {
        return std::unexpected(ScanError::WORD_EMPTY);
    }
    return {};
}

uint64_t now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch())
        .count();
}
};  // namespace

struct ReportOutputOpt {
    ReportOutputType output_type = ReportOutputType::CONSOLE;
    std::string path_dir;
};

// ---- Implementation -----

std::expected<ScanReport, ScanError> scan(const ScanParams& params) {
    // validate params
    std::cout << "\n";
    if (DEBUG) {
        std::cout << "[scan] starting...\n";
        std::cout << "[scan] scan: validating params...\n";
    }
    if (auto validate_res = validate_scan_params(params); !validate_res) {
        return std::unexpected(validate_res.error());
    }

    bool scan_all_files = params.number_of_files_to_search == 0;
    std::string_view search_mode = scan_all_files ? "all files" : "first N";
    bool is_single_thread = params.number_of_threads == 0;
    std::string_view run_mode =
        is_single_thread ? "single-threaded" : "multi-threaded";

    if (DEBUG) {
        std::cout << "[scan] scan: search_mode = " << search_mode
                  << ", run_mode = " << run_mode << "\n";
    }

    // get files
    size_t files_count = 0;
    std::vector<std::string> files_path;

    for (const auto& entry :
         std::filesystem::directory_iterator(params.path_dir)) {
        if (entry.is_regular_file()) {
            if (scan_all_files ||
                files_count < params.number_of_files_to_search) {
                files_path.push_back(entry.path().string());
                files_count++;
            }
        }
    }

    if (files_count == 0) {
        return std::unexpected(ScanError::DIR_EMPTY);
    }

    if (DEBUG) {
        std::cout << "[scan] scan: found " << files_path.size() << " files in "
                  << params.path_dir << " directory\n";
    }

    std::cout << "Searching for word '" << params.word_to_search << "' in "
              << files_count << " files (" << run_mode << ")...\n";

    size_t files_with_word_count = 0;
    size_t word_found_count = 0;
    uint64_t start_time = now_ms();
    if (is_single_thread) {
        for (const auto& path : files_path) {
            std::ifstream current_file(path, std::ios::in);

            if (!current_file.good()) {
                std::cerr << "[scan] error: cannot open file: " << path << "\n";
                continue;
            }

            std::string line;
            size_t line_number = 0;
            bool found_in_this_file = false;

            while (std::getline(current_file, line)) {
                ++line_number;

                size_t pos = line.find(params.word_to_search);
                while (pos != std::string::npos) {
                    ++word_found_count;

                    if (!found_in_this_file) {
                        found_in_this_file = true;
                        ++files_with_word_count;
                    }

                    if (DEBUG) {
                        std::cout << "[scan] found '" << params.word_to_search
                                  << "' in file=" << path
                                  << " line=" << line_number
                                  << " col=" << pos + 1 << "\n";
                    }

                    pos = line.find(params.word_to_search,
                                    pos + params.word_to_search.size());
                }
            }

            if (current_file.bad()) {
                std::cerr << "[scan] error: I/O failure while reading file: "
                          << path << "\n";
                continue;
            }

            if (!found_in_this_file && DEBUG) {
                std::cout << "[scan] not found in file: " << path << "\n";
            }
        }
    } else {
        // multi-threaded
        size_t num_threads = params.number_of_threads;
        num_threads = std::min(num_threads, files_count);

        const size_t base = files_count / num_threads;
        const size_t rem = files_count % num_threads;

        struct ThreadResult {
            size_t occurrences = 0;
            size_t files_with_word = 0;
            bool open_error = false;
            bool io_error = false;
        };

        std::vector<std::thread> pool;
        std::vector<ThreadResult> results(num_threads);
        pool.reserve(num_threads);

        size_t begin = 0;
        std::string word = params.word_to_search;

        for (size_t t = 0; t < num_threads; ++t) {
            const size_t count = base + (t < rem ? 1 : 0);
            const size_t end = begin + count;

            pool.emplace_back([&, t, begin, end]() {
                ThreadResult local;

                for (size_t i = begin; i < end; ++i) {
                    std::ifstream current_file(files_path[i], std::ios::in);
                    if (!current_file.good()) {
                        local.open_error = true;
                        continue;
                    }

                    std::string line;
                    bool found_in_file = false;

                    while (std::getline(current_file, line)) {
                        size_t pos = line.find(word);
                        while (pos != std::string::npos) {
                            ++local.occurrences;
                            found_in_file = true;
                            pos = line.find(word, pos + word.size());
                        }
                    }

                    if (current_file.bad()) {
                        local.io_error = true;
                    }

                    if (found_in_file) {
                        ++local.files_with_word;
                    }
                }

                results[t] = local;  // write back to shared array
            });

            begin = end;
        }

        for (auto& th : pool) {
            th.join();
        }

        for (const auto& r : results) {
            word_found_count += r.occurrences;
            files_with_word_count += r.files_with_word;

            if (r.open_error) {
                std::cerr << "[scan] warning: some files could not be opened\n";
            }
            if (r.io_error) {
                std::cerr << "[scan] warning: some files had I/O errors\n";
            }
        }
    }

    uint64_t end_time = now_ms();
    double time_taken_in_seconds = (end_time - start_time) / 1000.;
    std::cout << "Finished searching " << files_count << " files:\n";
    std::cout << "Number of occurrences: " << word_found_count << "\n";
    std::cout << "Time taken: " << time_taken_in_seconds << " seconds\n";

    if (DEBUG) {
        std::cout << "[scan] summary: '" << params.word_to_search
                  << "' found in " << files_with_word_count << " files"
                  << " (" << word_found_count << " occurrences)\n";
    }
    // @TODO: scan

    return ScanReport{};
}

std::expected<ScanReport, ScanError> scan(const char* path_dir,
                                          const char* word_to_search) {
    if (path_dir == nullptr || word_to_search == nullptr) {
        return std::unexpected(ScanError::NULL_ARGUMENT);
    }
    const ScanParams params{
        .number_of_files_to_search = DEFAULT_NUM_OF_FILES_TO_SEARCH,
        .number_of_threads = DEFAULT_NUM_OF_THREAD_USAGE,
        .path_dir = path_dir,
        .word_to_search = word_to_search,
    };
    return scan(params);
}

void make_report(const ScanReport& rep, const ReportOutputOpt& opt) {
    if (DEBUG) {
        std::cout << "[scan] report: generating...\n";
        std::cout << rep << "\n";
    }
    switch (opt.output_type) {
    case ReportOutputType::CONSOLE:
        if (DEBUG) {
            std::cout << "[scan] output: using console\n";
        }
        break;
    case ReportOutputType::CSV_FILE:
        if (opt.path_dir.empty() && DEBUG) {
            std::cerr << "[scan] missing-value: csv output requires path_dir\n";
            return;
        }
        if (DEBUG) {
            std::cout << "[scan] output: using csv file\n";
        }
        break;
    }
}
void make_report(const ScanReport& rep) { make_report(rep, ReportOutputOpt{}); }
}  // namespace threadscan
