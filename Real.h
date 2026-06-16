#pragma once

// The numeric type for every value a formula evaluates to, and every generated value tier. Everything downstream
// is written in terms of Real, so the precision/storage trade-off is a single knob here:
//   double      - IEEE 754 binary64, ~15-17 significant digits, 8 bytes, SIMD-friendly. The default.
//   long double - x86-64 80-bit extended, ~18-19 digits, 16 bytes, x87 (no SIMD); built-in and hardware-backed.
//   __float128  - IEEE binary128, ~33-34 digits, 16 bytes, but software-emulated and far slower.
// The extra digits let us tell a genuinely-recovered formula apart from a coincidental 15-digit approximation.
using Real = long double;