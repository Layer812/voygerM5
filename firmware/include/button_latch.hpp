#pragma once
#include <cstdint>
namespace button_latch {
// Active-low physical button state and a latched press event. Long display
// frames cannot discard an event until the game logic consumes it.
class Key {
 bool raw_=false,stable_=false,press_=false;
 uint32_t lastChange_=0;
 public:
 void begin(bool down,uint32_t now){raw_=stable_=down;press_=false;lastChange_=now;}
 void update(bool down,uint32_t now){
   if(down!=raw_){raw_=down;lastChange_=now;}
   if(stable_!=raw_ && uint32_t(now-lastChange_)>=12){
      stable_=raw_;
      if(stable_)press_=true;
   }
 }
 bool held() const {return stable_;}
 bool takePress(){bool p=press_;press_=false;return p;}
};
}

// Merge duplicate press edges reported by M5Unified and the independent
// GPIO sampler. Both sample the same physical switch at different times.
// Without a one-press gate, a single short tap can toggle the warp menu twice.
namespace button_latch {
class PressGate {
 bool armed_=true;
 uint32_t lastAccepted_=0, releasedSince_=0;
 public:
 void reset() {armed_=true;lastAccepted_=0;releasedSince_=0;}
 bool accept(bool edge, bool held, uint32_t now) {
   if(!held && releasedSince_==0) releasedSince_=now ? now : 1;
   // Evaluate re-arm BEFORE clearing releasedSince_ on the next new press.
   // This preserves a valid 30ms release even if the button is pressed again
   // on the first polling iteration after the 150ms duplicate-edge guard.
   if(!armed_ && releasedSince_ && uint32_t(now-releasedSince_)>=30 &&
      uint32_t(now-lastAccepted_)>=150) armed_=true;
   if(held) releasedSince_=0;
   if(edge && armed_) {
     armed_=false;lastAccepted_=now;releasedSince_=0;
     return true;
   }
   return false;
 }
};
}
