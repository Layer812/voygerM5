#include "warp_timing.hpp"
#include <cassert>
#include <cstdio>
int main(){
 warp_timing::FrameClock c;
 c.start();
 assert(!c.finished() && c.progress()==0.0f);
 // A very slow renderer must still display at least 17 transit frames.
 for(int i=0;i<17;i++) {c.onPresentedFrame(1000u+3000u*i);assert(!c.finished());}
 for(int i=17;i<30;i++) c.onPresentedFrame(1000u+3000u*i);
 assert(c.finished() && c.progress()==1.0f);
 c.start();
 for(int i=0;i<100;i++) c.onPresentedFrame(2000u+35u*i);
 assert(c.finished());
 std::puts("PASS warp frame pacing: high/low FPS + reset");
}
