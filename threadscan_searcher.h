#ifndef THREADSCAN_SEARCHER_H
#define THREADSCAN_SEARCHER_H

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "threadscan_types.h"

namespace threadscan {

struct ScanStrategy {
    virtual void execute(std::span<const std::string> files,
                         std::string_view word,
                         std::vector<FileScanResult>& out) = 0;
    virtual ~ScanStrategy() = default;
};

// Divides files into equal-sized chunks upfront; each thread owns a fixed
// slice.
struct ChunkStrategy : ScanStrategy {
    size_t num_threads;
    explicit ChunkStrategy(size_t n) : num_threads(n) {}
    void execute(std::span<const std::string> files, std::string_view word,
                 std::vector<FileScanResult>& out) override;
};

// Threads pull work items from a shared atomic counter; no pre-partitioning.
struct QueueStrategy : ScanStrategy {
    size_t num_threads;
    explicit QueueStrategy(size_t n) : num_threads(n) {}
    void execute(std::span<const std::string> files, std::string_view word,
                 std::vector<FileScanResult>& out) override;
};
std::vector<FileScanResult> run_single_threaded(
    std::span<const std::string> files, std::string_view word);

std::vector<FileScanResult> run_multi_threaded(
    std::span<const std::string> files, std::string_view word,
    ScanStrategy& strategy);

}  // namespace threadscan
#endif
