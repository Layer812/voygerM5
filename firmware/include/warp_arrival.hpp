#pragma once
// STAR VOYAGER R16 FIX7 - camera-ready, velocity-matched warp arrival.
// No M5/Arduino dependency: tested on host using the actual simulator.
#include "astro.hpp"
#include "voyagers.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace warp_arrival {
static const double PROBE_STANDOFF_METERS=10000.;
static const uint32_t ARRIVAL_GLOW_MS=1800;
static const double PROBE_VISIBLE_METERS=30000000.;
struct Arrival {
 astro::V ship,properVelocity;
 double yaw=0.,pitch=0.;
 int destination=0;
};
inline astro::V targetPosition(const astro::Sim& sim,int id){
 return (id>=10)?astro::voyagerPosition(id-10,sim.jd):sim.b[id].p;
}
inline astro::V targetVelocity(const astro::Sim& sim,int id){
 if(id>=10){
   const astro::V p=astro::voyagerPosition(id-10,astro::VOYAGER_EPOCH_JD);
   return astro::unit(p)*astro::VOYAGER_SPECS[id-10].speed;
 }
 return sim.referenceVelocity(id);
}
inline astro::V coordinateToProperVelocity(astro::V v){
 const double beta2=astro::dot(v,v)/(astro::C*astro::C);
 return beta2<1.?v/std::sqrt(1.-beta2):astro::V{};
}
inline int probeVisualRadius(double d){
 // Artistic close-range visibility; not a HUD target or an angular-scale model.
 return int(std::max(2.,std::min(39.,400000./std::max(1.,d))));
}
inline Arrival calculate(const astro::Sim& sim,int destination){
 Arrival a;
 a.destination=destination;
 a.ship=sim.ship+sim.fwd()*(astro::AU*.07);
 a.properVelocity=sim.properV;
 a.yaw=sim.yaw;a.pitch=sim.pitch;
 if(destination>0 && destination<12){
   const astro::V obj=targetPosition(sim,destination);
   astro::V direction=astro::unit(obj-sim.ship);
   if(astro::norm(direction)<.5)direction=sim.fwd();
   // Large bodies fit entirely on a 466px display; probes are much nearer.
   const double standOff=destination>=10 ? PROBE_STANDOFF_METERS :
        std::max(sim.b[destination].r*2.15,sim.b[destination].r+3000000.);
   a.ship=obj-direction*standOff;
   a.properVelocity=coordinateToProperVelocity(targetVelocity(sim,destination));
 }
 // Retain existing collision-avoidance constraints, including Earth atmosphere.
 for(int i=0;i<10;++i){
   const astro::Body &b=sim.b[i];
   const astro::V rel=a.ship-b.p;
   const double d=astro::norm(rel);
   const double safe=b.r*1.5+(i==3?500000.:5000000.);
   if(d<safe){
     astro::V outward=astro::unit(rel);
     if(astro::norm(outward)<.5)outward=sim.fwd();
     a.ship=b.p+outward*safe;
   }
 }
 if(destination>0){
   const astro::V aim=astro::unit(targetPosition(sim,destination)-a.ship);
   a.yaw=atan2(aim.x,aim.z);
   a.pitch=asin(std::max(-1.,std::min(1.,aim.y)));
 }
 return a;
}
}
