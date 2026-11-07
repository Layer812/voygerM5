#include <cassert>
#include "button_latch.hpp"
int main(){
 button_latch::PressGate g;
 assert(g.accept(true,true,100)); // GPIO latch
 assert(!g.accept(true,true,111)); // M5Unified duplicate edge delayed
 assert(!g.accept(false,true,122));
 assert(!g.accept(true,false,135)); // release bounce
 assert(!g.accept(false,false,165));
 assert(!g.accept(true,false,195)); // M5 delayed duplicate
 assert(!g.accept(false,false,211));
 assert(!g.accept(false,false,242)); // re-armed, no automatic event
 assert(g.accept(true,true,270)); // new deliberate press
 assert(!g.accept(true,true,282));
 // Common fast double click stays suppressed when no stable release yet.
 assert(!g.accept(false,false,295));
 assert(!g.accept(true,true,305));
}
