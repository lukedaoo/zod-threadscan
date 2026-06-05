#ifndef THREADSCAN_CORE_H
#define THREADSCAN_CORE_H
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

#include "config.h"
namespace threadscan {

struct ScanParams;
struct ScanReport;

enum class ReportOutputType : uint8_t {
    CONSOLE,  // = 0, console output
    CSV_FILE  // = 1, csv file output
};

enum class ScanError : uint8_t {
    // Input Errors
    PATH_EMPTY,
    PATH_NOT_DIRECTORY,
    PATH_NOT_FOUND,
    PERMISSION_DENIED,
    PATH_ACCESS_ERROR,
    WORD_EMPTY,
    NULL_ARGUMENT,
    // SCAN
    DIR_EMPTY
};

struct ReportOutputOpt;
std::expected<ScanReport, ScanError> scan(const ScanParams& params);
std::expected<ScanReport, ScanError> scan(const char* path_dir,
                                          const char* word_to_search);
void make_report(const ScanReport& rep);
void make_report(const ScanReport& rep, const ReportOutputOpt& output_opt);
}  // namespace threadscan
#ifdef THREADSCAN_CORE_IMPLEMENTATION
#include "threadscan_core_impl.cpp"
#endif
#endif
