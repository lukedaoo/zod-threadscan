#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>

#define THREADSCAN_CORE_IMPLEMENTATION
#include "threadscan_core.h"

namespace ts = threadscan;

struct ExpCase {
    size_t files;
    size_t threads;
    size_t runs;
};

static constexpr ExpCase EXP1[] = {
    {.files = 125, .threads = 0, .runs = 5},
    {.files = 250, .threads = 0, .runs = 5},
    {.files = 500, .threads = 0, .runs = 5},
    {.files = 1000, .threads = 0, .runs = 5},
};

static constexpr ExpCase EXP2[] = {
    {.files = 125, .threads = 2, .runs = 3},
    {.files = 125, .threads = 5, .runs = 3},
    {.files = 125, .threads = 10, .runs = 3},
    {.files = 125, .threads = 15, .runs = 3},
    {.files = 125, .threads = 20, .runs = 3},
    {.files = 250, .threads = 2, .runs = 3},
    {.files = 250, .threads = 5, .runs = 3},
    {.files = 250, .threads = 10, .runs = 3},
    {.files = 250, .threads = 15, .runs = 3},
    {.files = 250, .threads = 20, .runs = 3},
    {.files = 500, .threads = 2, .runs = 3},
    {.files = 500, .threads = 5, .runs = 3},
    {.files = 500, .threads = 10, .runs = 3},
    {.files = 500, .threads = 15, .runs = 3},
    {.files = 500, .threads = 20, .runs = 3},
    {.files = 1000, .threads = 2, .runs = 3},
    {.files = 1000, .threads = 5, .runs = 3},
    {.files = 1000, .threads = 10, .runs = 3},
    {.files = 1000, .threads = 15, .runs = 3},
    {.files = 1000, .threads = 20, .runs = 3},
};

static void run_experiment(const ExpCase* cases, size_t count,
                           const std::string& path, const std::string& word,
                           const std::string& csv_dir) {
    for (size_t i = 0; i < count; ++i) {
        const auto& c = cases[i];
        std::cout << "\n--- [files=" << c.files << " threads=" << c.threads
                  << "] ---\n";
        ts::ScanParams params{
            .number_of_files_to_search = c.files,
            .number_of_threads = c.threads,
            .number_of_runs = c.runs,
            .path_dir = path,
            .word_to_search = word,
        };
        auto result = ts::scan(params);
        if (!result) {
            std::cerr << "[exp] scan failed for files=" << c.files
                      << " threads=" << c.threads << "\n";
            continue;
        }
        ts::make_report(*result);
        ts::make_report(*result, {.output_type = ts::ReportOutputType::CSV_FILE,
                                  .path_dir = csv_dir});
    }
}

int main(int argc, char* argv[]) {
    std::string path;
    std::string word = "quasi";
    int experiment = 0;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--path" && i + 1 < argc) {
            path = argv[++i];
        } else if (arg == "--word" && i + 1 < argc) {
            word = argv[++i];
        } else if (arg == "--experiment-1") {
            experiment = 1;
        } else if (arg == "--experiment-2") {
            experiment = 2;
        }
    }

    if (path.empty()) {
        std::cerr << "[exp] error: --path required\n";
        return 1;
    }
    if (experiment == 0) {
        std::cerr << "[exp] error: --experiment-1 or --experiment-2 required\n";
        return 1;
    }

    std::cout << "=== Experiment " << experiment << " | word: " << word
              << " | path: " << path << " ===\n";

    if (experiment == 1) {
        run_experiment(EXP1, std::size(EXP1), path, word, "data/experiment-1");
    } else {
        run_experiment(EXP2, std::size(EXP2), path, word, "data/experiment-2");
    }
    return 0;
}
