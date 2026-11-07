#pragma once
#include "fast_sphere.hpp"
// The 257x257 pre-generated UV table lives in flash; a PSRAM mirror is used
// when available. The firmware never computes thousands of trig values at boot.
extern const fast_sphere::Cell gSphereLookup[fast_sphere::kCount];
