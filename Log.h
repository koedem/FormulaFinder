#pragma once

#include <iostream>

// Verbosity levels, ordered: each enables everything below it.
//   SILENT - no output at all (benchmarking, or using the finder as a library).
//   RESULT - only the solved formula per depth.
//   INFO   - + per-depth total time, generation/sort/prune progress and sizes, jumps, diagnostics.
//   DEBUG  - + the per-operation merge timing breakdown.
enum class LogLevel { SILENT = 0, RESULT = 1, INFO = 2, DEBUG = 3 };

class Log {
public:
    static void set_level(LogLevel l) { current = l; }
    static LogLevel level() { return current; }
    static bool enabled(LogLevel l) { return static_cast<int>(current) >= static_cast<int>(l); }

private:
    inline static LogLevel current = LogLevel::INFO;
};

// Streams to std::cout only when the current level is at least `lvl`. The right-hand side of the `<<` chain
// (e.g. a clock.end() call) is not evaluated when the level is too low, so disabled logging stays cheap.
#define LOG_AT(lvl) if (!Log::enabled(lvl)) {} else std::cout