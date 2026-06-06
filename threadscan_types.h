#ifndef THREADSCAN_TYPES_H
#define THREADSCAN_TYPES_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "config.h"
namespace threadscan {

enum class MulThreadStrategy : uint8_t { CHUNK, QUEUE };

inline MulThreadStrategy str_to_mul_thread_strategy(const char* s) {
    if (s != nullptr && std::string_view(s) == "queue") {
        return MulThreadStrategy::QUEUE;
    }
    return MulThreadStrategy::CHUNK;
}

struct ScanParams {
    std::size_t number_of_files_to_search = 0;
    std::size_t number_of_threads = 0;
    std::size_t number_of_runs = 0;
    std::string path_dir;
    std::string word_to_search;
    MulThreadStrategy mul_thread_strategy =
        str_to_mul_thread_strategy(DEFAULT_MUL_THREAD_STRATEGY);
};

struct FileScanResult {
    size_t occurrences = 0;
    // 0b0000'0000 = no error
    // 0b0000'0001 = open error
    // 0b0000'0010 = io error
    // 0b0000'0100 = word was found
    uint8_t error_flags = 0;
};

struct RunTiming {
    std::uint64_t start_ms = 0;
    std::uint64_t end_ms = 0;
    std::size_t total_occurrences = 0;
    [[nodiscard]] std::uint64_t elapsed_ms() const { return end_ms - start_ms; }
};

struct ScanResult {
    // metadata — populated by scan()
    std::string path_dir;
    std::string word_to_search;
    size_t files_intended = 0;
    size_t files_scanned = 0;
    size_t number_of_threads = 0;
    bool is_single_threaded = true;
    // run data
    std::vector<RunTiming> run_timings;
    std::vector<FileScanResult> final_results;
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
    std::string strategy =
        p.mul_thread_strategy == MulThreadStrategy::CHUNK ? "chunk" : "queue";
    os << "ScanParams {\n"
       << "  number_of_files_to_search = " << num_files << "\n"
       << "  number_of_threads         = " << num_threads << "\n"
       << "  number_of_runs            = " << p.number_of_runs << "\n"
       << "  mul_thread_strategy       = " << strategy << "\n"
       << "  path_dir                  = " << p.path_dir << "\n"
       << "  word_to_search            = " << p.word_to_search << "\n"
       << "}";
    return os;
}

}  // namespace threadscan

#endif
