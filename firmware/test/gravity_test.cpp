#include "astro.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
int main(){
 astro::Sim s; s.reset();
 std::cout<<"start alt="<<s.altitude()/1000<<"km g="<<s.earthGravity()<<"m/s^2 radial="<<s.radialSpeed()<<"m/s\n";
 assert(std::abs(s.altitude()-25000000.)<.01);
 assert(std::abs(s.earthGravity()-0.405)<.01);
 assert(std::abs(s.radialSpeed())<.01);
 double previous=s.altitude(),impactAt=0;
 for(int elapsed=60;elapsed<=12000;elapsed+=60){
  s.step(60);
  if(elapsed%1800==0||s.crashed){
   std::cout<<"freefall time="<<elapsed<<"s alt="<<s.altitude()/1000<<"km radial="<<s.radialSpeed()<<"m/s\n";
  }
  assert(s.altitude()<=previous+50000 || s.crashed);
  previous=s.altitude();
  if(s.crashed){impactAt=elapsed;break;}
 }
 assert(impactAt>6000 && impactAt<11000);
 astro::Sim orbit;orbit.startMode=astro::StartMode::CircularOrbit;orbit.reset();
 double init=orbit.altitude();
 std::cout<<"orbit start radial="<<orbit.radialSpeed()<<"m/s\n";
 for(int elapsed=60;elapsed<=11000;elapsed+=60){orbit.step(60); assert(!orbit.crashed);}
 std::cout<<"orbit alt after 3h="<<orbit.altitude()/1000<<"km delta="<<(orbit.altitude()-init)/1000<<"km\n";
 assert(std::abs(orbit.altitude()-init)<1000000.); // currently first-order ephemeris & lunar model
 std::cout<<"PASS gravity fall to Earth, circular orbital comparison\n";
}
