#include "astro.hpp"
#include "thrust_control.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 using namespace thrust_control;
 assert(fromRingX(0)==0 && fromRingX(10)==0 && fromRingX(-10)==0);
 assert(std::fabs(fromRingX(212)-1.)<1e-12);
 assert(std::fabs(fromRingX(-212)+1.)<1e-12);
 assert(fromRingX(-106)<0 && fromRingX(106)>0);
 assert(signedAcceleration(0.)==0.);
 for(double t:{.10,.25,.50,.75,1.}){
   assert(signedAcceleration(t)>0.);
   assert(signedAcceleration(-t)==-signedAcceleration(t));
 }
 assert(std::fabs(signedAcceleration(.25)-14.0)<1e-9);
 assert(std::fabs(signedAcceleration(.50)-90.)<1e-9);
 assert(std::fabs(signedAcceleration(1.)-240000.)<1e-9);
 EngineResponse resp;
 assert(std::fabs(resp.advance(1.,.5)-1./4.)<1e-9);
 assert(std::fabs(resp.advance(1.,.5)-1./2.)<1e-9);
 assert(std::fabs(resp.advance(1.,2.)-1.)<1e-9);
 double last=resp.actual;
 for(int i=0;i<7;i++){
   resp.advance(-1.,.5);
   assert(resp.actual<=last+1e-12);last=resp.actual;
   if(i<3) assert(resp.actual>=-1e-12); // crosses zero, never snaps negative
 }
 assert(resp.actual<0.);
 // Releasing does not cut the engine immediately: force eases out over 1.10s.
 const double releasedFrom=resp.actual;
 const double forceAtRelease=resp.acceleration();
 const double tail=resp.advance(0.,.01);
 assert(tail!=0. && std::fabs(tail)<std::fabs(releasedFrom));
 assert(std::fabs(resp.acceleration())<std::fabs(forceAtRelease));
 resp.advance(0.,.54);
 assert(resp.acceleration()!=0.);
 resp.advance(0.,.55);
 assert(resp.actual==0. && resp.acceleration()==0.);
 // A fresh press during coast resumes smoothly without jumping or sign flip.
 resp.advance(.25,.50);
 const double beforeRelease=resp.acceleration();
 resp.advance(0.,.25);
 assert(resp.acceleration()>0. && resp.acceleration()<beforeRelease);
 const double levelBeforeRepress=resp.actual;
 resp.advance(.25,.05);
 assert(resp.actual>levelBeforeRepress);
 resp.clear();
 assert(resp.acceleration()==0.);
 // Force must continue to act on actual physics with the button up.
 astro::Sim coast;coast.reset();coast.ship={astro::AU*100.,0.,0.};
 coast.properV={0.,0.,100000.};coast.yaw=0;coast.pitch=0;
 coast.thrust=true;coast.throttle=.25;
 coast.engine.advance(.25,2.);
 coast.thrust=false;
 coast.engine.advance(0.,.10);
 const double beforeCoast=coast.properV.z;
 coast.step(.10);
 assert(coast.properV.z>beforeCoast);
 coast.engine.advance(0.,RELEASE_FADE_SECONDS);
 assert(coast.thrustAccel()==0.);
 astro::Sim s;s.reset();
 s.ship={astro::AU*100.,0.,0.};s.properV={0.,0.,100000.};s.yaw=0;s.pitch=0;
 s.thrust=true;s.throttle=-.5;s.engine.advance(-.5,3.);
 const double before=s.properV.z;s.step(.01);assert(s.properV.z<before);
 astro::Sim f;f.reset();f.ship={astro::AU*100.,0.,0.};f.properV={0.,0.,100000.};f.yaw=0;f.pitch=0;
 f.thrust=true;f.throttle=.5;f.engine.advance(.5,3.);
 f.step(.01);assert(f.properV.z>before);
 astro::Sim neutral;neutral.reset();neutral.ship={astro::AU*100.,0.,0.};neutral.properV={0.,0.,100000.};neutral.yaw=0;neutral.pitch=0;
 neutral.thrust=true;neutral.throttle=0;neutral.engine.advance(0.,1.);neutral.step(.01);
 assert(std::fabs(neutral.properV.z-before)<1e-6);
 std::cout<<"PASS R16F6: 2x thrust, real-time spool, smooth 1.10s tail, continued physics, zero crossing and retro"<<std::endl;
}
