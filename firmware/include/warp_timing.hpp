#pragma once
// STAR VOYAGER R16 FIX8: warp FX time follows frames actually presented.
// The visual effect must remain visible under low-FPS planet rendering.
#include <cstdint>
#include <algorithm>
namespace warp_timing {
static const uint32_t TOTAL_MS=2400;
static const uint32_t MIN_FRAME_STEP_MS=35;
static const uint32_t MAX_FRAME_STEP_MS=140;
struct FrameClock {
 uint32_t elapsedMs=0;
 uint32_t lastPresentMs=0;
 uint32_t frames=0;
 void start(){elapsedMs=0;lastPresentMs=0;frames=0;}
 float progress() const {return float(elapsedMs)/float(TOTAL_MS);}
 bool finished() const{return elapsedMs>=TOTAL_MS;}
 void onPresentedFrame(uint32_t now){
  if(finished())return;
  uint32_t delta=frames ? uint32_t(now-lastPresentMs) : MIN_FRAME_STEP_MS;
  delta=std::max(MIN_FRAME_STEP_MS,std::min(MAX_FRAME_STEP_MS,delta));
  elapsedMs=std::min(TOTAL_MS,elapsedMs+delta);
  lastPresentMs=now;
  ++frames;
 }
};
}
