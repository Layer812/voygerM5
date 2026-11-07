// STAR VOYAGER R3: motor-strength envelope in milliseconds.
// No delays: called from main loop; impact always overrides warp.
#pragma once
#include <stdint.h>
#include <stddef.h>
namespace haptics {
struct Key { uint16_t until_ms; uint8_t power; };
// Short accelerating pulses, a stronger engagement pulse, then two release ticks.
static constexpr Key warp[] = {
 {180,0},{240,75},{355,0},{410,85},{505,0},{570,100},
 {655,0},{725,115},{805,0},{875,130},{945,0},{1015,145},
 {1090,0},{1190,175},{1280,0},{1370,190},{1500,0},
 {1600,110},{1750,0},{1810,85},{2050,0},{2125,70},{2400,0}
};
// A dedicated 3.6-second pattern for 1.125 ly jumps. Distinct pulses across
// charging, prolonged transit and exit; keep gaps to avoid continuous rumble.
static constexpr Key longWarp[] = {
 {180,0},{245,75},{365,0},{435,85},{555,0},{640,105},
 {755,0},{840,120},{960,0},{1050,135},{1175,0},{1280,150},
 {1410,0},{1530,170},{1680,0},{1810,190},{1940,0},
 {2050,145},{2190,0},{2300,165},{2440,0},{2550,190},
 {2690,0},{2800,155},{2940,0},{3055,175},{3190,0},
 {3300,110},{3400,0},{3490,75},{3600,0}
};
// A hard first shock, a shorter aftershock, then a fading shudder.
static constexpr Key impact[] = {
 {105,230},{160,0},{238,165},{305,0},{348,95},{450,0},
 {520,45},{1400,0}
};
template<size_t N> inline uint8_t sample(const Key (&curve)[N],uint32_t elapsed) {
 for(const auto &key : curve) if(elapsed < key.until_ms) return key.power;
 return 0;
}
inline uint8_t output(uint32_t elapsedWarp,bool warpActive,
                      uint32_t elapsedImpact,bool impactActive,
                      bool longRangeWarp=false) {
 if(impactActive) return sample(impact,elapsedImpact);
 if(warpActive) return longRangeWarp ? sample(longWarp,elapsedWarp) : sample(warp,elapsedWarp);
 return 0;
}
}
