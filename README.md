# FormulaFinder

FormulaFinder searches for a short closed-form expression whose value is as close as
possible to a target number you provide. Given a target, it reports, for each expression
"depth", the closest formula it can build from a small set of building blocks — the first
few natural numbers and a handful of well-known constants — combined with the operators
`+ - * / ^`, `log`, and `sqrt`.

It is useful for recognising a decimal as a familiar constant or combination — for example,
spotting that a number "is really" `(6 / 5) ^ (pi / e)` — and, at higher precision, for
telling a genuine closed form apart from a coincidental approximation.

## Example

```
$ ./FormulaFinder 8 RESULT
Enter the target value to approximate: 1.23456
depth 2: high score 0.01544;     x = 5 / 4
depth 3: high score 0.00041261;  x = 6 / (8 - pi)
depth 4: high score 7.67466e-07; x = (6 / 5) ^ (pi / e)
depth 5: high score 1.83719e-07; x = (pi + 5) ^ (3 / pi) / 6
depth 6: high score 0;           x = (3 ^ 6 + 4) / 5 ^ 5 + 1
depth 7: high score 0;           x = (3 ^ 6 + 4) / 5 ^ 5 + (2 - 1)
depth 8: high score 0;           x = 3 / (5 ^ 5 / 6) * 6 + 1 / 5 * 6
```

`high score` is the absolute difference between the formula's value and the target (smaller
is better; `0` means it matched to full precision). Each depth prints the best formula of
that complexity, so you can pick the simplest one that is accurate enough.

## Building

FormulaFinder is header-only apart from `main.cpp`, requires a **C++23** compiler, and has
no external dependencies.

With CMake (needs CMake ≥ 3.23):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Or directly:

```sh
g++ -O3 -DNDEBUG -std=c++23 main.cpp -o FormulaFinder
```

On GCC 11, use `-std=c++2b` in place of `-std=c++23`. The project builds and runs on GCC 11;
GCC 13+ is only needed for the optional `std::float128_t` precision (see [Precision](#precision)).

## Usage

```
FormulaFinder [max_depth] [log_level]
```

- `max_depth` — maximum expression depth to search (integer ≥ 2, at most 10, default 8).
- `log_level` — `SILENT | RESULT | INFO | DEBUG` (default `INFO`).
- `-h` / `--help` — print usage and exit.

The **target value is entered interactively** after startup (so it can be typed or piped in).
Both positional arguments are optional and keep their defaults if omitted.

Log levels are cumulative:

| Level    | Output                                                                    |
| -------- | ------------------------------------------------------------------------- |
| `SILENT` | nothing (e.g. for benchmarking)                                           |
| `RESULT` | the best formula per depth                                                |
| `INFO`   | + per-depth timing, generation/sort/prune progress and sizes, diagnostics |
| `DEBUG`  | + the per-operation merge timing breakdown                                |

## How it works

**Atoms and constants.** The depth-1 building blocks are the first few natural numbers and a
handful of well-known mathematical constants (such as `pi` and `e`). A few further constants
are seeded at depth 2 rather than depth 1, so they stay available but cost as much as a
two-deep subtree. The exact set lives in `Generator.h` and is easy to extend.

**Operators.** Binary: `+`, `-`, `*`, `/`, `^` (power), and `log_b(a)` — with both operand
orders considered where they differ. Unary: `sqrt`, which costs one depth.

**Depth.** Depth measures expression size: a depth-`d` formula combines a depth-`k` and a
depth-`(d−k)` subtree with a binary operator, or takes the `sqrt` of a depth-`(d−1)` subtree.
Roughly, depth equals the number of leaves.

**Search.** For each depth, FormulaFinder builds the sorted set of every value reachable at
that depth (the value "tiers"), then finds the closest reachable value to the target with a
two-pointer "closest match" sweep over the sorted tiers: within each region where an operator
is monotone in both operands, the closest value is found in linear time. The winning value is
then reconstructed into a formula tree and rendered as minimally-parenthesised infix text.

## Configuration

A few compile-time constants control the search:

- `GENERATION_CAP` (`main.cpp`, default `5`) — value tiers are materialised up to this depth.
  Deeper searches reuse the deepest tier, so the largest searchable depth is `2 * GENERATION_CAP`
  (= 10); `max_depth` is capped there. Raising the cap quickly becomes memory-bound (see below).
- `Real` (`Real.h`, default `long double`) — the numeric type for all values; see below.

## Precision

All values use the `Real` type alias, set to `long double` (~18–19 significant digits on
x86-64). The constants come from `std::numbers`, so they are correctly rounded at whatever
precision `Real` has. This lets the search distinguish a genuinely-recovered formula from a
coincidental ~15-digit match.

`Real` is the single knob for the precision/speed/memory trade-off:

- `double` — ~15–17 digits, 8 bytes, fastest.
- `long double` — ~18–19 digits, 16 bytes (the default).
- `std::float128_t` — ~33–34 digits (needs C++23 `<stdfloat>`, i.e. GCC 13+).

Switching is a one-line change in `Real.h`; the constants and `std::` math follow automatically.

Note that results are only as precise as the target you enter, which is parsed at `Real`'s
precision — a 15-digit value cannot match to more than ~15 digits.

## Limitations

- **Memory.** The depth-5 tier holds tens of millions of values; generation peak is the main
  ceiling. Going past depth 5 in generation would need a sub-chunked / out-of-core redesign.
- **Depth.** Searchable depth is capped at `2 * GENERATION_CAP` (10) — beyond that, no formula
  can be built from the available tiers.
- **Runtime** grows with depth and is higher at `long double` precision than at `double`.

## Project layout

| File              | Responsibility                                        |
| ----------------- | ----------------------------------------------------- |
| `main.cpp`        | CLI parsing, interactive prompt, the depth loop       |
| `Real.h`          | the `Real` numeric type alias                         |
| `Operators.h`     | operator vocabulary (evaluation, symbols, precedence) |
| `Formula.h`       | formula tree and the infix renderer                   |
| `MergeFinder.h`   | the two-pointer closest-match search engine           |
| `FormulaFinder.h` | orchestration: search, tree reconstruction, printing  |
| `Generator.h`     | generation of the per-depth value tiers               |
| `Utils.h`         | binary search and diagnostics                         |
| `Log.h`           | log levels                                            |
| `SimpleClock.h`   | timing helper                                         |
