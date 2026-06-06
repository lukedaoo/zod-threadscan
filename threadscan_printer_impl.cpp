#include <iomanip>
#include <iostream>
#include <ostream>
#include <string>

#include "config.h"
#include "threadscan_printer.h"
#include "threadscan_types.h"

namespace threadscan {

void print_console(const ScanResult& report) {
    if (report.run_timings.empty()) {
        return;
    }

    const size_t total_occ = report.run_timings.back().total_occurrences;
    double total_sec = 0.0;
    for (const auto& t : report.run_timings) {
        total_sec += static_cast<double>(t.elapsed_ms()) / 1000.0;
    }
    const double avg_sec =
        total_sec / static_cast<double>(report.run_timings.size());

    std::cout << "Finished searching " << report.files_scanned << " files:\n";
    std::cout << "Number of occurrences found: " << total_occ << "\n";
    std::cout << "Time taken: " << std::fixed << std::setprecision(7) << avg_sec
              << " seconds\n";

    if (DEBUG && report.run_timings.size() > 1) {
        std::cout << "\n";
        for (size_t i = 0; i < report.run_timings.size(); ++i) {
            double sec =
                static_cast<double>(report.run_timings[i].elapsed_ms()) /
                1000.0;
            std::cout << "  run " << (i + 1) << ": " << std::fixed
                      << std::setprecision(7) << sec << " s | occurrences: "
                      << report.run_timings[i].total_occurrences << "\n";
        }
        std::cout << "  average: " << std::fixed << std::setprecision(7)
                  << avg_sec << " s\n";
    }
}

void print_csv(const ScanResult& report, std::ostream& os) {
    const std::string mode =
        report.is_single_threaded ? "single-threaded" : "multi-threaded";
    os << "directory,word,files_intended,files_scanned,mode\n";
    os << report.path_dir << "," << report.word_to_search << ","
       << report.files_intended << "," << report.files_scanned << "," << mode
       << "\n\n";
    os << "run,time_sec,occurrences\n";

    double total_sec = 0.0;
    for (size_t i = 0; i < report.run_timings.size(); ++i) {
        double sec =
            static_cast<double>(report.run_timings[i].elapsed_ms()) / 1000.0;
        total_sec += sec;
        os << (i + 1) << "," << std::fixed << std::setprecision(7) << sec << ","
           << report.run_timings[i].total_occurrences << "\n";
    }
    if (!report.run_timings.empty()) {
        double avg = total_sec / static_cast<double>(report.run_timings.size());
        os << "average," << std::fixed << std::setprecision(7) << avg << ",\n";
    }
}

}  // namespace threadscan
