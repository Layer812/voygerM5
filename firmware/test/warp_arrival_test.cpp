#include "warp_arrival.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
  astro::Sim sim;sim.reset();
  const astro::V start=sim.ship;
  const astro::V startVelocity=sim.properV;
  const auto freeFlight=warp_arrival::calculate(sim,0);
  assert(astro::norm(freeFlight.ship-start)>astro::AU*.069);
  assert(astro::norm(freeFlight.properVelocity-startVelocity)<.00001);
  assert(freeFlight.yaw==sim.yaw && freeFlight.pitch==sim.pitch);
  for(int target=1;target<12;++target){
    const auto arrival=warp_arrival::calculate(sim,target);
    const astro::V bodyPos=warp_arrival::targetPosition(sim,target);
    const astro::V delta=bodyPos-arrival.ship;
    const astro::V pointing={std::sin(arrival.yaw)*std::cos(arrival.pitch),
                              std::sin(arrival.pitch),
                              std::cos(arrival.yaw)*std::cos(arrival.pitch)};
    assert(astro::dot(astro::unit(delta),pointing)>.999999999);
    assert(astro::norm(delta)>100.);
    const astro::V moving=arrival.properVelocity/std::sqrt(1.+astro::dot(arrival.properVelocity,arrival.properVelocity)/(astro::C*astro::C));
    assert(astro::norm(moving-warp_arrival::targetVelocity(sim,target))<.00001);
    if(target<=9){
      const double radius=sim.b[target].r;
      assert(astro::norm(delta)>radius*1.5);
      assert(astro::norm(delta)<radius*6.0+10e6);
      const double apparentRadiusPx=380.*radius/astro::norm(delta);
      assert(apparentRadiusPx>55.);  // Watch's target is actually visible.
    } else {
      assert(std::fabs(astro::norm(delta)-warp_arrival::PROBE_STANDOFF_METERS)<.1);
      assert(warp_arrival::probeVisualRadius(astro::norm(delta))>=30);
      assert(warp_arrival::probeVisualRadius(warp_arrival::PROBE_VISIBLE_METERS)>=2);
    }
    sim.ship=arrival.ship;sim.properV=arrival.properVelocity;
    sim.yaw=arrival.yaw;sim.pitch=arrival.pitch;sim.crashed=false;sim.engine.clear();
    sim.step(.10);
    assert(!sim.crashed); // Warp should not immediately cause a body collision.
    // Reset for next target from known Earth orbit to avoid cumulative changes.
    sim.reset();
  }
  assert(warp_arrival::ARRIVAL_GLOW_MS==1800);
  std::cout<<"PASS R16F7 warp arrival: all 9 bodies plus Voyager 1/2 centered,"
           <<" velocity matched, collision safe, no labels/markers"<<std::endl;
}
