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
    return duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

FileScanResult scan_one_file(const std::string& file_path,
                             std::string_view word) {
    constexpr uint8_t ERR_OPEN = 1 << 0;  // 0000'0001
    constexpr uint8_t ERR_IO = 1 << 1;    // 0000'0010
    constexpr uint8_t FOUND = 1 << 2;     // 0000'0100

    FileScanResult result;
    std::ifstream file(file_path, std::ios::in);

    if (!file.good()) {
        result.error_flags |= ERR_OPEN;
        return result;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find(word);
        while (pos != std::string::npos) {
            ++result.occurrences;
            pos = line.find(word, pos + 1);
            result.error_flags |= FOUND;
        }
    }
    if (file.bad()) {
        result.error_flags |= ERR_IO;
    }
    return result;
}

};  // namespace

struct ReportOutputOpt {
    ReportOutputType output_type = ReportOutputType::CONSOLE;
    std::string path_dir;
};

// ---- Implementation -----
std::expected<ScanReport, ScanError> scan(const ScanParams& params) {
    if (auto validate_res = validate_scan_params(params); !validate_res) {
        return std::unexpected(validate_res.error());
    }

    bool scan_all_files = params.number_of_files_to_search == 0;
    bool is_single_thread = params.number_of_threads == 0;
    std::string_view run_mode =
        is_single_thread ? "single-threaded" : "multi-threaded";

    // get files
    std::vector<std::string> files_path;
    files_path.reserve(params.number_of_files_to_search);

    for (const auto& entry :
         std::filesystem::directory_iterator(params.path_dir)) {
        if (entry.is_regular_file()) {
            files_path.push_back(entry.path().string());
        }
    }
    std::ranges::sort(files_path, [](const std::string& a,
                                     const std::string& b) {
        auto extract = [](const std::string& s) -> int {
            auto stem = std::filesystem::path(s).stem().string();
            auto it = std::ranges::find_if(stem.begin(), stem.end(), ::isdigit);
            if (it == stem.end()) {
                return -1;
            }
            return std::stoi(std::string(it, stem.end()));
        };
        return extract(a) < extract(b);
    });
    if (!scan_all_files &&
        files_path.size() > params.number_of_files_to_search) {
        files_path.resize(params.number_of_files_to_search);
    }
    if (files_path.size() == 0) {
        return std::unexpected(ScanError::DIR_EMPTY);
    }
    size_t files_count = files_path.size();
    std::cout << "Searching for word '" << params.word_to_search << "' in "
              << files_count << " files (" << run_mode << ")...\n";

    if (is_single_thread) {
        ScanReport report;
        report.results.reserve(scan_all_files
                                   ? files_path.size()
                                   : params.number_of_files_to_search);
        report.start_time = now_ms();
        for (const auto& path : files_path) {
            FileScanResult res = scan_one_file(path, params.word_to_search);
            report.results.push_back(res);
        }
        report.end_time = now_ms();
        return report;
    }

    ScanReport report;
    size_t num_threads = std::min(params.number_of_threads, files_count);

    const size_t base = files_count / num_threads;
    const size_t reminder = files_count % num_threads;

    std::vector<std::thread> pool;
    std::vector<FileScanResult> thread_results(num_threads);
    pool.reserve(num_threads);

    size_t begin = 0;
    report.start_time = now_ms();
    for (size_t t = 0; t < num_threads; ++t) {
        const size_t count = base + (t < reminder ? 1 : 0);
        const size_t end = begin + count;

        pool.emplace_back([&, t, begin, end]() {
            FileScanResult local;

            for (size_t i = begin; i < end; ++i) {
                FileScanResult r =
                    scan_one_file(files_path[i], params.word_to_search);

                local.occurrences += r.occurrences;
                local.error_flags |= r.error_flags;
            }

            thread_results[t] = local;
        });

        begin = end;
    }

    for (auto& th : pool) {
        th.join();
    }
    report.end_time = now_ms();
    report.results = std::move(thread_results);
    return report;
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
    size_t total_occurrences = 0;
    size_t total_files_with_word = 0;
    for (const auto& r : rep.results) {
        total_occurrences += r.occurrences;
        bool found_in_file = (r.error_flags & (1 << 2)) != 0;
        if (found_in_file) {
            total_files_with_word++;
        }
    }
    double time_taken_in_seconds = (rep.end_time - rep.start_time) / 1000.;
    switch (opt.output_type) {
    case ReportOutputType::CONSOLE:
        std::cout << "Finished searching " << rep.results.size() << " files:\n";
        std::cout << "Number of occurrences: " << total_occurrences << " in "
                  << total_files_with_word << " files\n";
        std::cout << "Time taken: " << time_taken_in_seconds << " seconds\n";
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
