#ifndef THREADSCAN_CORE_H
#define THREADSCAN_CORE_H
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

#include "config.h"
#include "threadscan_printer.h"
#include "threadscan_searcher.h"
namespace threadscan {

struct ScanParams;
struct ScanResult;

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
std::expected<ScanResult, ScanError> scan(const ScanParams& params);
std::expected<ScanResult, ScanError> scan(const ScanParams& params,
                                          const SortCriteria& criteria);
std::expected<ScanResult, ScanError> scan(const char* path_dir,
                                          const char* word_to_search);
void make_report(const ScanResult& rep);
void make_report(const ScanResult& rep, const ReportOutputOpt& output_opt);
}  // namespace threadscan
#ifdef THREADSCAN_CORE_IMPLEMENTATION
#include "threadscan_core_impl.cpp"
#include "threadscan_searcher_impl.cpp"
#include "threadscan_printer_impl.cpp"
#endif
#endif
