#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

#include "config.h"
#include "threadscan_core.h"
namespace threadscan {
// ---- Structs -----
struct ScanParams {
    std::size_t number_of_files_to_search = 0;  // 0 means seaching in all files
    std::size_t number_of_threads = 0;          // 0 means single thread
    std::string path_dir;  // full path to directory. E.g /path/to/your/dir
    std::string word_to_search;  // word to search
};

struct ScanResult {
    std::uint64_t id;
    std::uint64_t start_time;
    std::uint64_t end_time;
    std::uint32_t thread_id;
    std::string path;
};

struct ScanReport {
    std::uint64_t start_time;
    std::uint64_t end_time;
    std::vector<ScanResult> results;
};

struct ReportOutputOpt {
    ReportOutputType output_type = ReportOutputType::CONSOLE;
    std::string path_dir;  // empty = not set
};

// ---- Internal Helpers -----
namespace {}  // namespace

// ---- Printer Helpers -----
inline std::ostream& operator<<(std::ostream& os, const ScanResult& r) {
    os << "ScanResult {\n"
       << "  id         = " << r.id << "\n"
       << "  thread_id  = " << r.thread_id << "\n"
       << "  start_time = " << r.start_time << "\n"
       << "  end_time   = " << r.end_time << "\n"
       << "  elapsed    = " << (r.end_time - r.start_time) << "\n"
       << "  path       = " << r.path << "\n"
       << "}";
    return os;
}
inline std::ostream& operator<<(std::ostream& os, const ScanReport& rep) {
    os << "ScanReport {\n"
       << "  start_time = " << rep.start_time << "\n"
       << "  end_time   = " << rep.end_time << "\n"
       << "  results    = " << rep.results.size() << " items\n"
       << "}\n";

    for (const auto& r : rep.results) {
        os << r << "\n";
    }

    return os;
}

inline std::ostream& operator<<(std::ostream& os, const ScanParams& p) {
    os << "ScanParams {\n"
       << "  number_of_files_to_search = " << p.number_of_files_to_search
       << "\n"
       << "  number_of_threads        = " << p.number_of_threads << "\n"
       << "  path_dir                 = " << p.path_dir << "\n"
       << "  word_to_search           = " << p.word_to_search << "\n"
       << "}";
    return os;
}

// ---- Implementation -----

std::expected<ScanReport, ScanError> scan(const ScanParams& params) {
    std::cout << "Start scanning...\n";
    std::cout << params << "\n";

    // validate params
    if (params.path_dir.empty()) {
        return std::unexpected(ScanError::PATH_EMPTY);
    }
    std::error_code ec;
    if (!std::filesystem::is_directory(params.path_dir, ec)) {
        return std::unexpected(ScanError::PATH_NOT_DIRECTORY);
    }
    if (params.word_to_search.empty()) {
        return std::unexpected(ScanError::WORD_EMPTY);
    }

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
    std::cout << "Start making report...\n";
    std::cout << rep << "\n";
    switch (opt.output_type) {
    case ReportOutputType::CONSOLE:
        std::cout << "Using console output\n";
        break;
    case ReportOutputType::CSV_FILE:
        if (opt.path_dir.empty()) {
            std::cerr << "CSV_FILE output requires path_dir\n";
            return;
        }
        std::cout << "Using CSV_FILE output\n";
        break;
    }
}
void make_report(const ScanReport& rep) { make_report(rep, ReportOutputOpt{}); }
}  // namespace threadscan
