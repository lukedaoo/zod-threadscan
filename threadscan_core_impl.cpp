#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
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
};  // namespace

struct ReportOutputOpt {
    ReportOutputType output_type = ReportOutputType::CONSOLE;
    std::string path_dir;
};

// ---- Implementation -----

void apply_sort(std::vector<std::string>& files, const SortCriteria& criteria) {
    std::ranges::sort(files, criteria);
}

std::expected<ScanReport, ScanError> scan(const ScanParams& params) {
    SortCriteria default_criteria = [](const std::string& a,
                                       const std::string& b) -> bool {
        auto extract = [](const std::string& s) -> int {
            auto stem = std::filesystem::path(s).stem().string();
            auto it = std::ranges::find_if(stem.begin(), stem.end(), ::isdigit);
            if (it == stem.end()) {
                return -1;
            }
            return std::stoi(std::string(it, stem.end()));
        };
        return extract(a) < extract(b);
    };
    return scan(params, default_criteria);
}

std::expected<ScanReport, ScanError> scan(const ScanParams& params,
                                          const SortCriteria& criteria) {
    if (auto validate_res = validate_scan_params(params); !validate_res) {
        return std::unexpected(validate_res.error());
    }

    bool scan_all_files = params.number_of_files_to_search == 0;
    bool is_single_thread = params.number_of_threads == 0;
    std::string_view run_mode =
        is_single_thread ? "single-threaded" : "multi-threaded";

    std::vector<std::string> files_path;

    for (const auto& entry :
         std::filesystem::directory_iterator(params.path_dir)) {
        if (entry.is_regular_file()) {
            files_path.push_back(entry.path().string());
        }
    }

    apply_sort(files_path, criteria);

    if (!scan_all_files &&
        files_path.size() > params.number_of_files_to_search) {
        files_path.resize(params.number_of_files_to_search);
    }
    if (files_path.empty()) {
        return std::unexpected(ScanError::DIR_EMPTY);
    }

    size_t files_count = files_path.size();
    std::cout << "\nSearching for word '" << params.word_to_search << "' in "
              << files_count << " files (" << run_mode << ")...\n";

    ScanReport report;
    report.start_time = now_ms();

    if (is_single_thread) {
        report.results = run_single_threaded(files_path, params.word_to_search);
    } else {
        // std::cout << "Using chunk strategy\n";
        // ChunkStrategy strategy(params.number_of_threads);

        // std::cout << "Using queue strategy\n";
        QueueStrategy strategy(params.number_of_threads);
        report.results =
            run_multi_threaded(files_path, params.word_to_search, strategy);
    }

    report.end_time = now_ms();
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
    double time_taken_in_seconds =
        static_cast<double>(rep.end_time - rep.start_time) / 1000.;
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
