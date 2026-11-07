#include "astro.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
 using thrust_control::EngineResponse;
 using thrust_control::RELEASE_FADE_SECONDS;
 EngineResponse e;
 assert(thrust_control::signedAcceleration(1.)==240000.);
 assert(thrust_control::signedAcceleration(-1.)==-240000.);
 e.advance(.25,2.);
 const double peak=e.acceleration();
 assert(std::fabs(peak-14.)<1e-9);
 e.advance(0.,.15);
 assert(e.acceleration()>0. && e.acceleration()<peak);
 e.advance(0.,.40);
 assert(e.acceleration()>0.);
 e.advance(0.,RELEASE_FADE_SECONDS-.55);
 assert(e.acceleration()==0. && e.actual==0.);
 // A changing warp factor must not change the engine fade time.
 e.advance(.5,2.);
 e.advance(0.,RELEASE_FADE_SECONDS);
 assert(e.acceleration()==0.);
 std::cout<<"PASS: 2x force and complete 1.10s release fade"<<std::endl;
}
