#ifndef THREADSCAN_TYPES_H
#define THREADSCAN_TYPES_H

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace threadscan {

struct ScanParams {
    std::size_t number_of_files_to_search = 0;
    std::size_t number_of_threads = 0;
    std::string path_dir;
    std::string word_to_search;
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
    std::string num_files = p.number_of_files_to_search == 0
                                ? "all"
                                : std::to_string(p.number_of_files_to_search);
    std::string num_threads = p.number_of_threads == 0
                                  ? "no threads"
                                  : std::to_string(p.number_of_threads);
    os << "ScanParams {\n"
       << "  number_of_files_to_search = " << num_files << "\n"
       << "  number_of_threads        = " << num_threads << "\n"
       << "  path_dir                 = " << p.path_dir << "\n"
       << "  word_to_search           = " << p.word_to_search << "\n"
       << "}";
    return os;
}

}  // namespace threadscan

#endif
