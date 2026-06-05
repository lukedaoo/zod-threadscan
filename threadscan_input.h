#ifndef THREADSCAN_INPUT_H
#define THREADSCAN_INPUT_H

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "config.h"

namespace threadscan {

struct ScanParams;

bool get_scan_params_from_console(ScanParams& out);
bool get_scan_params_from_command_line(int argc, char* argv[], ScanParams& out);
bool get_params(int argc, char* argv[], ScanParams& out);
}  // namespace threadscan

#ifdef THREADSCAN_INPUT_IMPLEMENTATION
#include "threadscan_input_impl.cpp"
#endif

#endif
