#include <fstream>
#include <thread>

#include "threadscan_searcher.h"
#include "threadscan_types.h"

namespace threadscan {

constexpr uint8_t ERR_OPEN = 1 << 0;  // 0000'0001
constexpr uint8_t ERR_IO = 1 << 1;    // 0000'0010
constexpr uint8_t FOUND = 1 << 2;     // 0000'0100

FileScanResult scan_one_file(const std::string& file_path,
                             std::string_view word) {
    FileScanResult result;
    std::ifstream file(file_path, std::ios::in);

    if (!file.good()) {
        result.error_flags |= ERR_OPEN;
        return result;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find(word);
        while (pos != std::string::npos) {
            ++result.occurrences;
            pos = line.find(word, pos + 1);
            result.error_flags |= FOUND;
        }
    }
    if (file.bad()) {
        result.error_flags |= ERR_IO;
    }
    return result;
}

std::vector<FileScanResult> run_single_threaded(
    std::span<const std::string> files, std::string_view word) {
    std::vector<FileScanResult> results;
    results.reserve(files.size());
    for (const auto& path : files) {
        results.push_back(scan_one_file(path, word));
    }
    return results;
}

std::vector<FileScanResult> run_multi_threaded(
    std::span<const std::string> files, std::string_view word,
    ScanStrategy& strategy) {
    std::vector<FileScanResult> out;
    strategy.execute(files, word, out);
    return out;
}

void ChunkStrategy::execute(std::span<const std::string> files,
                            std::string_view word,
                            std::vector<FileScanResult>& out) {
    size_t files_count = files.size();
    size_t n = std::min(num_threads, files_count);
    const size_t base = files_count / n;
    const size_t rem = files_count % n;

    out.resize(files_count);
    std::vector<std::thread> pool;
    pool.reserve(n);

    size_t begin = 0;
    for (size_t t = 0; t < n; ++t) {
        const size_t count = base + (t < rem ? 1 : 0);
        const size_t end = begin + count;
        pool.emplace_back([&, begin, end]() {
            for (size_t i = begin; i < end; ++i) {
                out[i] = scan_one_file(files[i], word);
            }
        });
        begin = end;
    }
    for (auto& th : pool) {
        th.join();
    }
}

void QueueStrategy::execute(std::span<const std::string> files,
                            std::string_view word,
                            std::vector<FileScanResult>& out) {
    size_t files_count = files.size();
    size_t n = std::min(num_threads, files_count);

    out.resize(files_count);
    std::atomic<size_t> idx{0};
    std::vector<std::thread> pool;
    pool.reserve(n);

    for (size_t t = 0; t < n; ++t) {
        pool.emplace_back([&]() {
            while (true) {
                size_t i = idx.fetch_add(1, std::memory_order_relaxed);
                if (i >= files_count) {
                    break;
                }
                out[i] = scan_one_file(files[i], word);
            }
        });
    }
    for (auto& th : pool) {
        th.join();
    }
}

}  // namespace threadscan
