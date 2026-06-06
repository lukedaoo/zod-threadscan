#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
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

std::expected<std::vector<std::string>, ScanError> prepare_file_paths(
    const ScanParams& params, const SortCriteria& criteria) {
    std::vector<std::string> files;
    for (const auto& entry :
         std::filesystem::directory_iterator(params.path_dir)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().string());
        }
    }
    std::ranges::sort(files, criteria);
    bool scan_all = params.number_of_files_to_search == 0;
    if (!scan_all && files.size() > params.number_of_files_to_search) {
        files.resize(params.number_of_files_to_search);
    }
    if (files.empty()) {
        return std::unexpected(ScanError::DIR_EMPTY);
    }
    return files;
}

std::unique_ptr<ScanStrategy> create_strategy(const ScanParams& params) {
    if (params.number_of_threads == 0) {
        return nullptr;
    }
    if (params.mul_thread_strategy == MulThreadStrategy::CHUNK) {
        return std::make_unique<ChunkStrategy>(params.number_of_threads);
    }
    return std::make_unique<QueueStrategy>(params.number_of_threads);
}

ScanResult execute_runs(std::span<const std::string> files,
                        std::string_view word, size_t runs,
                        ScanStrategy* strategy) {
    ScanResult report;
    size_t ref_occurrences = 0;
    for (size_t run = 0; run < runs; ++run) {
        RunTiming t;
        t.start_ms = now_ms();
        auto current_results = (strategy == nullptr)
                                   ? run_single_threaded(files, word)
                                   : run_multi_threaded(files, word, *strategy);
        t.end_ms = now_ms();

        for (const auto& r : current_results) {
            t.total_occurrences += r.occurrences;
        }

        if (run == 0) {
            ref_occurrences = t.total_occurrences;
        } else if (t.total_occurrences != ref_occurrences) {
            std::cerr << "[report] error: total_occurrences mismatch at run"
                      << run << "!\n";
        }

        if (run == runs - 1) {
            report.final_results = std::move(current_results);
        }

        report.run_timings.push_back(t);
    }
    return report;
}

std::filesystem::path resolve_csv_path(const std::string& path_dir,
                                       const ScanResult& rep) {
    std::filesystem::path p(path_dir);
    if (!p.extension().empty()) {
        std::filesystem::create_directories(p.parent_path());
        return p;
    }
    std::filesystem::create_directories(p);
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    char ts[16];
    std::strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", &tm);
    std::string threads_part =
        rep.is_single_threaded ? "single"
                               : std::to_string(rep.number_of_threads) + "t";
    std::string filename = std::string(ts) + "_" + rep.word_to_search + "_" +
                           std::to_string(rep.files_scanned) + "files_" +
                           threads_part + ".csv";
    return p / filename;
}

}  // namespace

struct ReportOutputOpt {
    ReportOutputType output_type = ReportOutputType::CONSOLE;
    std::string path_dir;
};

// ---- Public API -----

std::expected<ScanResult, ScanError> scan(const ScanParams& params) {
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

std::expected<ScanResult, ScanError> scan(const ScanParams& params,
                                          const SortCriteria& criteria) {
    if (auto v = validate_scan_params(params); !v) {
        return std::unexpected(v.error());
    }

    auto files_result = prepare_file_paths(params, criteria);
    if (!files_result) {
        return std::unexpected(files_result.error());
    }
    const auto& files = *files_result;

    auto strategy = create_strategy(params);
    std::string run_mode = (params.number_of_threads == 0)
                               ? "single-threaded"
                               : "multi-threaded with " +
                                     std::to_string(params.number_of_threads) +
                                     " threads";
    std::cout << "Searching for word '" << params.word_to_search << "' in "
              << files.size() << " files (" << run_mode << ")...\n";

    size_t runs = std::max(params.number_of_runs, size_t{1});
    auto report =
        execute_runs(files, params.word_to_search, runs, strategy.get());
    report.path_dir = params.path_dir;
    report.word_to_search = params.word_to_search;
    report.files_intended = files.size();
    report.files_scanned = report.final_results.size();
    report.number_of_threads = params.number_of_threads;
    report.is_single_threaded = params.number_of_threads == 0;
    return report;
}

std::expected<ScanResult, ScanError> scan(const char* path_dir,
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

void make_report(const ScanResult& rep, const ReportOutputOpt& opt) {
    if (opt.output_type == ReportOutputType::CSV_FILE) {
        if (opt.path_dir.empty()) {
            print_csv(rep, std::cout);
        } else {
            auto filepath = resolve_csv_path(opt.path_dir, rep);
            std::ofstream f(filepath);
            if (!f.is_open()) {
                std::cerr << "[report] error: cannot open csv file: "
                          << filepath << "\n";
                return;
            }
            print_csv(rep, f);
            std::cout << "[report] csv saved: " << filepath << "\n";
        }
    } else {
        print_console(rep);
    }
}
void make_report(const ScanResult& rep) {
    if (DEFAULT_PRINTER_TYPE != nullptr) {
        if (std::string_view(DEFAULT_PRINTER_TYPE) == "console") {
            make_report(rep, {.output_type = ReportOutputType::CONSOLE});
        } else if (std::string_view(DEFAULT_PRINTER_TYPE) == "csv") {
            make_report(rep, {.output_type = ReportOutputType::CSV_FILE});
        }
        return;
    }
    print_console(rep);
}

}  // namespace threadscan
