#include "fast_sphere.hpp"
#include "thrust_control.hpp"
#include "relativity.hpp"
#include <cassert>
#include <iostream>
static unsigned sw16(unsigned n){return ((n<<8)&65535) | (n>>8);}
int main(){
 using namespace fast_sphere;
 assert(sw16(0xf800)==0x00f8 && sw16(0x001f)==0x1f00);
 assert(shade565(0xf800,256,31,63,31)==0xf800);
 assert(shade565(0x07e0,256,31,63,31)==0x07e0);
 assert(shade565(0x001f,256,31,63,31)==0x001f);
 assert(shade565(0xffff,256,31,63,31)==0xffff);
 using thrust_control::signedAcceleration;
 assert(signedAcceleration(0)==0);
 assert(signedAcceleration(.10)>=1.2);
 assert(signedAcceleration(.25)>=7.);
 assert(signedAcceleration(.50)>=45.);
 assert(signedAcceleration(-.25)<=-7.);
 assert(relativity::factor(0,1)==1.0);
 astro::Sim ship; ship.reset();
 assert(ship.localReferenceBody()==3);
 assert(ship.localSpeed()<0.01);
 const double start=ship.localSpeed();
 // No screen interaction needed: one smooth 5-o'clock (25%) sustained pulse.
 ship.thrust=true;ship.throttle=.25;
 for(int i=0;i<20;i++){
   ship.engine.advance(.25,.05);
   ship.step(1.); // 20x simulation time
 }
 assert(ship.localSpeed()>start+25.);

 std::cout<<"PASS R15 RGB565 identity and enhanced low-thrust curve\n";
}
