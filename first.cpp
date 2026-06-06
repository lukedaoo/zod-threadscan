#include <utility>

#define THREADSCAN_CORE_IMPLEMENTATION
#include "threadscan_core.h"

#define THREADSCAN_INPUT_IMPLEMENTATION
#include "threadscan_input.h"

namespace ts = threadscan;

int main(int argc, char* argv[]) {
    ts::ScanParams params;
    if (!ts::get_params(argc, argv, params)) {
        return 1;
    }
    auto result = ts::scan(params);
    if (!result) {
        return 1;
    }
    ts::ScanResult scan_result = std::move(*result);
    ts::make_report(scan_result);
    return 0;
}
