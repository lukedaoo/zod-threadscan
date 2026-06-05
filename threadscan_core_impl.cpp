#include <expected>
#include <filesystem>
#include <iostream>
#include <system_error>

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
};  // namespace

struct ReportOutputOpt {
    ReportOutputType output_type = ReportOutputType::CONSOLE;
    std::string path_dir;
};

// ---- Implementation -----

std::expected<ScanReport, ScanError> scan(const ScanParams& params) {
    std::cout << "Start scanning...\n";
    std::cout << params << "\n";

    // validate params
    if (auto validate_res = validate_scan_params(params); !validate_res) {
        return std::unexpected(validate_res.error());
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
