#ifndef THREADSCAN_PRINTER_H
#define THREADSCAN_PRINTER_H
#include <iosfwd>

#include "threadscan_types.h"

namespace threadscan {
void print_console(const ScanResult& report);
void print_csv(const ScanResult& report, std::ostream& os);
}  // namespace threadscan

#ifdef THREADSCAN_PRINTER_IMPLEMENTATION
#include "threadscan_printer_impl.cpp"
#endif
#endif
