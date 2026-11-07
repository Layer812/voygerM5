#pragma once
#include <algorithm>
#include <cmath>
namespace thrust_control {
// Lower ring: left=retro, bottom=zero, right=forward.
constexpr double RING_RADIUS=212.0;
constexpr double DETENT=0.06;
constexpr double DEFAULT_RISE_SECONDS=2.0;  // real seconds, NOT accelerated sim time
constexpr double RELEASE_FADE_SECONDS=1.10; // complete and deterministic coast-tail
constexpr double POWER_GAIN=2.0;  // R16F6: exactly double R16F5 thrust at every setting
inline double normalize(double t) {
  t=std::max(-1.0,std::min(1.0,t));
  const double a=std::fabs(t);
  if(a<=DETENT)return 0.0;
  return std::copysign((a-DETENT)/(1.-DETENT),t);
}
inline double fromRingX(double x){return normalize(x/RING_RADIUS);}
// Tuned for precise gravity/orbit control at low power; emergency high thrust
// remains available at the far ends of the scale. Engine is fictional.
struct PowerPoint{double input,ms2;};
constexpr PowerPoint CURVE[]={{0.00,0.0},{0.10,1.2},{0.25,7.0},
                              {0.50,45.0},{0.75,1100.0},{1.00,120000.0}};
inline double signedAcceleration(double t){
 t=std::max(-1.0,std::min(1.0,t));
 double a=std::fabs(t);
 if(a==0.)return 0.;
 for(unsigned i=1;i<sizeof(CURVE)/sizeof(CURVE[0]);i++){
   if(a<=CURVE[i].input){
     const double q=(a-CURVE[i-1].input)/(CURVE[i].input-CURVE[i-1].input);
     const double lo=CURVE[i-1].ms2,hi=CURVE[i].ms2;
     const double output=lo==0.?q*hi:lo*std::pow(hi/lo,q);
     return std::copysign(output*POWER_GAIN,t);
   }
 }
 return std::copysign(CURVE[5].ms2*POWER_GAIN,t);
}
// Commanded throttle is not instantaneous engine output. Power-up retains the
// original two-second spool-up. On button release, both the throttle level
// and the actual force fade continuously for 1.10s, in *real* seconds. The
// force envelope is computed separately: the fictional engine has a nonlinear
// throttle curve, so decaying the throttle alone would not feel smooth.
// A new press during the fade resumes from the current engine level.
struct EngineResponse {
 double actual=0.;
 double force=0.;
 double releaseStartLevel=0.;
 double releaseStartForce=0.;
 double releaseElapsed=0.;
 bool releasing=false;
 void clear(){
   actual=force=releaseStartLevel=releaseStartForce=releaseElapsed=0.;
   releasing=false;
 }
 double acceleration()const{return force;}
 double advance(double command,double realDt,double riseSeconds=DEFAULT_RISE_SECONDS){
   command=std::max(-1.0,std::min(1.0,command));
   if(realDt<=0.)return actual;
   if(command==0.){
     if(actual==0. && !releasing){force=0.;return 0.;}
     if(!releasing){
       releaseStartLevel=actual;
       releaseStartForce=force;
       releaseElapsed=0.;
       releasing=true;
     }
     releaseElapsed=std::min(RELEASE_FADE_SECONDS,releaseElapsed+realDt);
     const double u=releaseElapsed/RELEASE_FADE_SECONDS;
     const double fade=(1.-u)*(1.-u)*(1.+2.*u); // smoothstep complement
     actual=releaseStartLevel*fade;
     force=releaseStartForce*fade;
     if(releaseElapsed>=RELEASE_FADE_SECONDS)clear();
     return actual;
   }
   releasing=false;
   releaseElapsed=releaseStartLevel=releaseStartForce=0.;
   const double maxDelta=realDt/std::max(0.2,riseSeconds);
   const double err=command-actual;
   actual+=std::max(-maxDelta,std::min(maxDelta,err));
   if(std::fabs(actual)<1e-12)actual=0.;
   force=signedAcceleration(actual);
   return actual;
 }
};
} // namespace thrust_control
