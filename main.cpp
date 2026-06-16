#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include "FormulaFinder.h"
#include "Utils.h"
#include "Generator.h"
#include "Real.h"
#include "Log.h"

namespace {

constexpr size_t DEFAULT_MAX_DEPTH = 8;
constexpr LogLevel DEFAULT_LOG_LEVEL = LogLevel::INFO;
// Value tiers are materialized only up to this depth; deeper searches reuse the deepest tier (see findAndPrint's
// jump logic). Generating past this is the current memory ceiling, so it stays an internal constant for now rather
// than a CLI knob.
//
// Depth 6 is the wall: combining tier 5 (~44.5M) with the 10 atoms alone yields ~3.5 billion values (~26 GB) that
// must be materialized contiguously before they can be sorted and pruned, with ~46 GB of raw output in total and a
// final deduped tier estimated at ~14-24 GB. reserve / in-place pruning do not help -- that raw chunk is the floor.
// Reaching depth 6 would need sub-chunked, incrementally pruned generation (to bound the transient) or an
// out-of-core redesign (to fit the final tier). See git history for the full breakdown.
constexpr size_t GENERATION_CAP = 5;

void print_usage(const char* program) {
    std::cerr << "Usage: " << program << " [max_depth] [log_level]\n"
              << "  max_depth   maximum expression depth to search (integer >= 2, default "
              << DEFAULT_MAX_DEPTH << ")\n"
              << "  log_level   SILENT | RESULT | INFO | DEBUG (default INFO)\n"
              << "The target value is entered interactively after startup.\n";
}

// Parses the optional positional args into max_depth / level. Returns false (after printing usage) on bad input
// or when help is requested, so main can exit.
bool parse_args(int argc, char** argv, size_t& max_depth, LogLevel& level) {
    for (int i = 1; i < argc; i++) {
        const std::string_view arg = argv[i];
        if (arg == "-h" || arg == "--help") { return false; }
    }
    if (argc > 1) {
        try {
            const int depth = std::stoi(argv[1]);
            if (depth < 2) { throw std::invalid_argument("depth too small"); }
            max_depth = static_cast<size_t>(depth);
        } catch (const std::exception&) {
            std::cerr << "Invalid max_depth: '" << argv[1] << "'\n\n";
            return false;
        }
    }
    if (argc > 2) {
        if (const auto parsed = Log::parse_level(argv[2])) {
            level = *parsed;
        } else {
            std::cerr << "Invalid log_level: '" << argv[2] << "'\n\n";
            return false;
        }
    }
    return argc <= 3; // extra positional args are a mistake worth flagging.
}

// Reads a finite target value from stdin, re-prompting on garbage. Returns false on EOF (e.g. a closed pipe).
bool prompt_target(Real& to_find) {
    std::cout << "Enter the target value to approximate: " << std::flush;
    while (!(std::cin >> to_find) || !std::isfinite(to_find)) {
        if (std::cin.eof()) { return false; }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Not a finite number, please try again: " << std::flush;
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    size_t max_depth = DEFAULT_MAX_DEPTH;
    LogLevel level = DEFAULT_LOG_LEVEL;
    if (!parse_args(argc, argv, max_depth, level)) {
        print_usage(argv[0]);
        return 1;
    }
    Log::set_level(level);

    Real to_find = 0;
    if (!prompt_target(to_find)) {
        std::cerr << "No target value provided." << std::endl;
        return 1;
    }

    auto values_per_depth = Generator::initialize_values();
    FormulaFinder finder(Generator::atoms(), Generator::constants());

    for (size_t depth = 2; depth <= max_depth; depth++) {
        finder.findAndPrint<true>(depth, values_per_depth, to_find);
        // Materialize the next tier only while under the cap; deeper searches reuse the deepest tier via the jump.
        if (depth <= GENERATION_CAP) {
            Generator::generate_values(values_per_depth, depth);
            Utils::count_matching_negative_numbers(values_per_depth[depth]);
        }
    }
    return 0;
}
