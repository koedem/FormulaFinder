#include <cmath>
#include "MergeFinder.h"
#include "Utils.h"
#include "Generator.h"
#include "Log.h"

int main() {
    Log::set_level(LogLevel::INFO); // SILENT | RESULT | INFO | DEBUG
    //double_t to_find = 9.8757731789244900612;
    double_t to_find = 29.6026283655491230532864493; // (π^e)×3−(2^π)×7+24
    auto values_per_depth = Generator::initialize_values();
    MergeFinder finder(Generator::atoms(), Generator::constants());

    for (size_t depth = 2; depth <= 5; depth++) {
        finder.findAndPrint<true>(depth, values_per_depth, to_find);

        Generator::generate_values(values_per_depth, depth);
        Utils::count_matching_negative_numbers(values_per_depth[depth]);
    }

    finder.findAndPrint<true>(7, values_per_depth, to_find);
    finder.findAndPrint<true>(8, values_per_depth, to_find);
    return 0;
}
