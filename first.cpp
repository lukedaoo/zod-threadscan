#include <iostream>
#include <string_view>

#define THREADSCAN_CORE_IMPLEMENTATION
#include "threadscan_core.h"

#define THREADSCAN_INPUT_IMPLEMENTATION
#include "threadscan_input.h"

namespace ts = threadscan;

static std::string_view scan_error_to_string(ts::ScanError err) {
    switch (err) {
    case ts::ScanError::PATH_EMPTY:
        return "path_dir is empty";
    case ts::ScanError::PATH_NOT_DIRECTORY:
        return "path_dir is not a directory";
    case ts::ScanError::PATH_NOT_FOUND:
        return "path_dir is not found";
    case ts::ScanError::PERMISSION_DENIED:
        return "permission denied";
    case ts::ScanError::PATH_ACCESS_ERROR:
        return "path_dir access error";
    case ts::ScanError::WORD_EMPTY:
        return "word_to_search is empty";
    case ts::ScanError::NULL_ARGUMENT:
        return "path_dir or word_to_search is null";
    }
    return "unknown error";
}

int main(int argc, char* argv[]) {
    ts::ScanParams params;
    if (!ts::get_params(argc, argv, params)) {
        return 1;
    }
    auto result = ts::scan(params);
    if (!result) {
        std::cerr << "[scan] error: " << scan_error_to_string(result.error())
                  << "\n";
        return 1;
    }
    ts::ScanReport scan_report = std::move(*result);
    ts::make_report(scan_report, {.output_type = ts::ReportOutputType::CONSOLE,
                                  .path_dir = "tmp/ts"});
    return 0;
}
