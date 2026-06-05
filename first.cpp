#include <iostream>
#include <string_view>

#define THREADSCAN_CORE_IMPLEMENTATION
#include "threadscan_core.h"

#define THREADSCAN_INPUT_IMPLEMENTATION
#include "threadscan_input.h"

static std::string_view scan_error_to_string(threadscan::ScanError err) {
    switch (err) {
    case threadscan::ScanError::PATH_EMPTY:
        return "path_dir is empty";
    case threadscan::ScanError::PATH_NOT_DIRECTORY:
        return "path_dir is not a directory";
    case threadscan::ScanError::PATH_NOT_FOUND:
        return "path_dir is not found";
    case threadscan::ScanError::PERMISSION_DENIED:
        return "permission denied";
    case threadscan::ScanError::PATH_ACCESS_ERROR:
        return "path_dir access error";
    case threadscan::ScanError::WORD_EMPTY:
        return "word_to_search is empty";
    case threadscan::ScanError::NULL_ARGUMENT:
        return "path_dir or word_to_search is null";
    }
    return "unknown error";
}

int main(int argc, char* argv[]) {
    threadscan::ScanParams params;
    if (!threadscan::get_params(argc, argv, params)) {
        return 1;
    }
    auto result = threadscan::scan(params);
    if (!result) {
        std::cerr << "scan failed: " << scan_error_to_string(result.error())
                  << "\n";
        return 1;
    }
    threadscan::ScanReport scan_report = std::move(*result);
    threadscan::make_report(
        scan_report, {.output_type = threadscan::ReportOutputType::CONSOLE,
                      .path_dir = "tmp/threadscan"});
    return 0;
}
