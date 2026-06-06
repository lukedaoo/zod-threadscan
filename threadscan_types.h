#ifndef THREADSCAN_TYPES_H
#define THREADSCAN_TYPES_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace threadscan {

struct ScanParams {
    std::size_t number_of_files_to_search = 0;
    std::size_t number_of_threads = 0;
    std::size_t number_of_runs = 0;
    std::string path_dir;
    std::string word_to_search;
};

struct FileScanResult {
    size_t occurrences = 0;
    // 0b0000'0000 = no error
    // 0b0000'0001 = open error
    // 0b0000'0010 = io error
    // 0b0000'0100 = word was found
    uint8_t error_flags = 0;
};

struct ScanReport {
    std::uint64_t start_time;
    std::uint64_t end_time;
    std::vector<FileScanResult> results;
};

using SortCriteria =
    std::function<bool(const std::string&, const std::string&)>;


inline std::ostream& operator<<(std::ostream& os, const ScanParams& p) {
    std::string num_files = p.number_of_files_to_search == 0
                                ? "all"
                                : std::to_string(p.number_of_files_to_search);
    std::string num_threads = p.number_of_threads == 0
                                  ? "no threads"
                                  : std::to_string(p.number_of_threads);
    os << "ScanParams {\n"
       << "  number_of_files_to_search = " << num_files << "\n"
       << "  number_of_threads         = " << num_threads << "\n"
       << "  number_of_runs            = " << p.number_of_runs << "\n"
       << "  path_dir                  = " << p.path_dir << "\n"
       << "  word_to_search            = " << p.word_to_search << "\n"
       << "}";
    return os;
}

}  // namespace threadscan

#endif
