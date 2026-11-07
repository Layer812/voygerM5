#include "astro.hpp"
#include "voyagers.hpp"
#include "haptics.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 astro::Sim s;s.reset();
 double e=astro::norm(s.b[3].p);assert(e>.98*astro::AU&&e<1.02*astro::AU);
 double m=astro::norm(s.b[4].p-s.b[3].p);assert(m>340e6&&m<420e6);
 double alt=astro::norm(s.ship-s.b[3].p)-s.b[3].r;
 assert(fabs(alt-25e6)<1e-4);assert(s.beta()<.001);assert(s.gamma()>=1);
 auto old=s.ship;s.step(45);assert(!s.crashed);assert(astro::norm(s.ship-old)>1000);
 s.warp();assert(!s.crashed);assert(s.gamma()>=1);
 for(int i=0;i<2;i++){
   auto p=astro::voyagerPosition(i,astro::VOYAGER_EPOCH_JD);
   double dist=astro::norm(p-astro::planet(2,astro::VOYAGER_EPOCH_JD))/astro::AU;
   assert(std::abs(dist-astro::VOYAGER_SPECS[i].earthAU)<1.e-7);
   auto later=astro::voyagerPosition(i,astro::VOYAGER_EPOCH_JD+1);
   double travelled=astro::norm(later-p);
   assert(std::abs(travelled-astro::VOYAGER_SPECS[i].speed*86400.)<300.);
 }
 assert(haptics::output(0,true,0,false)==0);
 assert(haptics::output(200,true,0,false)==75);
 assert(haptics::output(1180,true,0,false)==175);
 assert(haptics::output(2400,true,0,false)==0);
 assert(haptics::output(0,false,0,true)==230);
 assert(haptics::output(0,false,150,true)==0);
 assert(haptics::output(0,false,180,true)==165);
 assert(haptics::output(200,true,20,true)==230); // Impact has priority.
 assert(haptics::output(0,false,1400,true)==0);
 std::cout << "PASS: haptic pulses and impact priority\n";
 std::cout << "PASS: planets, Moon, start altitude, orbit, warp, Voyager 1/2 positions and propagation\n";
}
