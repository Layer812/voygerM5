#include <cassert>
#include <cmath>
#include "ring_geometry.hpp"
int main(){
 auto a=[](double t){double x=212*cos(ring_geometry::angleForThrottle(t)),y=212*sin(ring_geometry::angleForThrottle(t));return ring_geometry::fromRingXY(x,y);};
 assert(a(-1)<-.98&&a(1)>.98&&fabs(a(0))<1e-12);
 assert(a(-.5)<-.42&&a(.5)>.42);
 assert(ring_geometry::fromRingXY(212*cos(-.70),212*sin(-.70))>.98); // just above 2h stays forward
 assert(ring_geometry::fromRingXY(212*cos(3.85),212*sin(3.85))<-.98); // just above 10h stays reverse
 assert(!ring_geometry::inArc(0,-212)); // top is NOT a throttle touch
 assert(ring_geometry::inArc(0,212));
 assert(ring_geometry::angleForThrottle(-1)>ring_geometry::angleForThrottle(0));
 // All intermediate positions, including full reverse and full forward.
 for(int i=0;i<=100;i++){
   const double t=-1.0+2.0*i/100.0;
   const double theta=ring_geometry::angleForThrottle(t);
   const double got=ring_geometry::fromRingXY(177*cos(theta),177*sin(theta));
   assert(fabs(got-t)<.081);
   assert(ring_geometry::onThrottleBand(177*cos(theta),177*sin(theta)));
 }
 assert(!ring_geometry::onThrottleBand(120,0));
 assert(!ring_geometry::onThrottleBand(0,-205));
 assert(ring_geometry::onThrottleBand(185*cos(3.66),185*sin(3.66)));
 assert(ring_geometry::onThrottleBand(185*cos(-.61),185*sin(-.61)));
}
