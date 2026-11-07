#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "thrust_control.hpp"
namespace astro {
constexpr double kPi=3.1415926535897932384626;
constexpr double DEG=kPi/180., AU=149597870700., C=299792458.;
constexpr double LIGHT_YEAR=C*365.25*86400.;
constexpr double LONG_WARP_STEP=1.125*LIGHT_YEAR; // four jumps cover 4.5 ly
struct V { double x=0,y=0,z=0; V()=default; V(double X,double Y,double Z):x(X),y(Y),z(Z){};
V operator+(V b)const{return{x+b.x,y+b.y,z+b.z};}V operator-(V b)const{return{x-b.x,y-b.y,z-b.z};}
V operator*(double s)const{return{x*s,y*s,z*s};}V operator/(double s)const{return{x/s,y/s,z/s};}
V& operator+=(V b){x+=b.x;y+=b.y;z+=b.z;return *this;}};
inline double dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline double norm(V a){return sqrt(dot(a,a));}
inline V unit(V a){double s=norm(a);return s>0?a/s:V{};}
struct El {double val[6],rate[6];};
// JPL "Approximate Positions of the Planets" Table 1, J2000 ecliptic, 1800-2050.
static constexpr El orbit[8]={
{{.38709927,.20563593,7.00497902,252.25032350,77.45779628,48.33076593},{.00000037,.00001906,-.00594749,149472.67411175,.16047689,-.12534081}},
{{.72333566,.00677672,3.39467605,181.97909950,131.60246718,76.67984255},{.00000390,-.00004107,-.00078890,58517.81538729,.00268329,-.27769418}},
{{1.00000261,.01671123,-.00001531,100.46457166,102.93768193,0},{.00000562,-.00004392,-.01294668,35999.37244981,.32327364,0}},
{{1.52371034,.09339410,1.84969142,-4.55343205,-23.94362959,49.55953891},{.00001847,.00007882,-.00813131,19140.30268499,.44441088,-.29257343}},
{{5.20288700,.04838624,1.30439695,34.39644051,14.72847983,100.47390909},{-.00011607,-.00013253,-.00183714,3034.74612775,.21252668,.20469106}},
{{9.53667594,.05386179,2.48599187,49.95424423,92.59887831,113.66242448},{-.00125060,-.00050991,.00193609,1222.49362201,-.41897216,-.28867794}},
{{19.18916464,.04725744,.77263783,313.23810451,170.95427630,74.01692503},{-.00196176,-.00004397,.00242939,428.48202785,.40805281,.04240589}},
{{30.06992276,.00859048,1.77004347,-55.12002969,44.96476227,131.78422574},{.00026291,.00005105,.00035372,218.45945325,-.32241464,-.00508664}}
};
inline V planet(int n,double jd){
 double T=(jd-2451545.)/36525.,q[6];for(int i=0;i<6;i++)q[i]=orbit[n].val[i]+orbit[n].rate[i]*T;
 double a=q[0],ecc=q[1],I=q[2]*DEG,N=q[5]*DEG,w=(q[4]-q[5])*DEG;
 double M=remainder((q[3]-q[4])*DEG,2.*kPi),E=M;
 for(int j=0;j<6;j++)E-=(E-ecc*sin(E)-M)/(1.-ecc*cos(E));
 double xo=a*(cos(E)-ecc),yo=a*sqrt(1.-ecc*ecc)*sin(E);
 double cw=cos(w),sw=sin(w),cN=cos(N),sN=sin(N),ci=cos(I),si=sin(I);
 return V{(cw*cN-sw*sN*ci)*xo+(-sw*cN-cw*sN*ci)*yo,
          (cw*sN+sw*cN*ci)*xo+(-sw*sN+cw*cN*ci)*yo,
          sw*si*xo+cw*si*yo}*AU;
}
inline V moon(double jd,V earth){
 // Artistic gameplay approximation; lunar coordinates NOT from Horizons/JPL.
 double days=jd-2451545.,M=(134.963+13.064993*days)*DEG,E=M;
 for(int i=0;i<5;i++)E-=(E-.0549*sin(E)-M)/(1.-.0549*cos(E));
 double r=384400000.*(1.-.0549*cos(E)),th=E+.12,node=(125.08-.0529539*days)*DEG;
 V q{r*cos(th),r*sin(th)*cos(5.145*DEG),r*sin(th)*sin(5.145*DEG)};
 return earth+V{q.x*cos(node)-q.y*sin(node),q.x*sin(node)+q.y*cos(node),q.z};
}
struct Body {const char* name;double GM,r; V p;};
enum class StartMode {FreeFall, CircularOrbit};
struct Sim {
 static constexpr double JD_START=2461349.5; // 2026-11-05 00:00 UTC; checked via epoch conversion in host tests.
 Body b[10]={
  {"SUN",1.32712440018e20,696340000.,{}},{"MERCURY",2.2032e13,2439700.,{}},
  {"VENUS",3.24859e14,6051800.,{}},{"EARTH",3.986004418e14,6371000.,{}},
  {"MOON",4.9048695e12,1737400.,{}},{"MARS",4.282837e13,3389500.,{}},
  {"JUPITER",1.26686534e17,69911000.,{}},{"SATURN",3.7931187e16,58232000.,{}},
  {"URANUS",5.793939e15,25362000.,{}},{"NEPTUNE",6.836529e15,24622000.,{}}
 };
 Body alienWorld{"UNLISTED",3.986004418e14,6371000.,{}}; // hidden story world; not a nav destination
 double jd=JD_START;V ship{},properV{};double yaw=0,pitch=0,throttle=0.; bool thrust=false,crashed=false;
 int crashBody=-1; double crashRelativeSpeed=0.;
 thrust_control::EngineResponse engine; // real-time power buildup
 StartMode startMode=StartMode::FreeFall;
 void update(){b[0].p={};for(int i=0;i<8;i++)b[i<3?i+1:i+2].p=planet(i,jd);b[4].p=moon(jd,b[3].p);}
 V fwd()const{return{sin(yaw)*cos(pitch),sin(pitch),cos(yaw)*cos(pitch)};}
 V vel()const{return properV/sqrt(1.+dot(properV,properV)/(C*C));}
 double beta()const{return norm(vel())/C;}
 double gamma()const{return sqrt(1.+dot(properV,properV)/(C*C));}
 V earthVelocity(double atJD) const {
   return (planet(2,atJD+1./86400.)-planet(2,atJD-1./86400.))*.5;
 }
 // Reference frame for flight HUD speed: use the body with the strongest
 // local gravitational pull, not the arbitrary heliocentric 30 km/s baseline.
 int localReferenceBody()const{
   int best=0; double maxG=-1.;
   for(int i=0;i<10;++i){
     const V d=ship-b[i].p;
     const double r2=std::max(dot(d,d),b[i].r*b[i].r);
     const double strength=b[i].GM/r2;
     if(strength>maxG){maxG=strength;best=i;}
   }
   return best;
 }
 V referenceVelocity(int i)const{
   if(i==0)return {};
   const double sec=1./86400.;
   if(i==3)return earthVelocity(jd);
   if(i==4){
     return (moon(jd+sec,planet(2,jd+sec))-
             moon(jd-sec,planet(2,jd-sec)))*.5;
   }
   const int ephemeris=(i<4)?i-1:i-2;
   return (planet(ephemeris,jd+sec)-planet(ephemeris,jd-sec))*.5;
 }
 double localSpeed()const{
   return norm(vel()-referenceVelocity(localReferenceBody()));
 }

 void reset(){jd=JD_START;update();V e=unit(b[3].p);double r=b[3].r+25000000.;
   ship=b[3].p-e*r;
   // Relative velocity is exactly zero in FreeFall (Earth's heliocentric
   // velocity is inherited, rather than incorrectly starting at Sun rest).
   V earthV=earthVelocity(jd);
   V tangent=unit(V{-e.y,e.x,0});
   properV=earthV;
   if(startMode==StartMode::CircularOrbit)properV+=tangent*sqrt(b[3].GM/r);
   yaw=atan2(e.x,e.z);pitch=asin(e.y);crashed=false;thrust=false;throttle=0.;engine.clear();
   crashBody=-1;crashRelativeSpeed=0.;
   // Exactly four 1.125 ly reverse-direction jumps place the craft 30 Mm from the planet.
   // No orientation correction: a one-degree heading error misses by about 0.079 ly.
   alienWorld.p=ship-e*(4.*LONG_WARP_STEP+30000000.);
 }
 double altitude(int body=3)const{return norm(ship-b[body].p)-b[body].r;}
 double radialSpeed(int body=3)const{
   V r=ship-b[body].p;
   V reference=(body==3)?earthVelocity(jd):V{};
   return dot(vel()-reference,unit(r)); // negative indicates inbound
 }
 double earthGravity()const{
   double r=norm(ship-b[3].p);return b[3].GM/(r*r);
 }
 V gravity()const{V a;for(const auto& x:b){V diff=x.p-ship;double r=std::max(norm(diff),x.r*.9);a+=diff*(x.GM/(r*r*r));}
   // The unlisted planet only affects the flight when the ship reaches its vicinity.
   const V d=alienWorld.p-ship; if(dot(d,d)<1.e22){double r=std::max(norm(d),alienWorld.r*.9);a+=d*(alienWorld.GM/(r*r*r));}
   return a;}
 double thrustAccel()const{return engine.acceleration();}
 void step(double dt){
   if(crashed||dt<=0)return;
   double minAlt=1e99;for(auto& x:b)minAlt=std::min(minAlt,(norm(ship-x.p)-x.r)/x.r);
   double maxStep=minAlt<1.? .5 : minAlt<5.?2.:minAlt<30.?20.:120.;
   int count=std::min(120,std::max(1,(int)ceil(dt/maxStep)));double h=dt/count;
   // Kick-drift-kick (velocity Verlet / leapfrog): greatly reduces orbit
   // energy drift compared with the original forward-Euler integration.
   for(int j=0;j<count;j++){
      V a=gravity();if(engine.acceleration()!=0.)a+=fwd()*thrustAccel();
      properV+=a*(h*.5);
      const V beforeMove=ship;
      ship+=vel()*h;
      jd+=h/86400.;update();
      V aNext=gravity();if(engine.acceleration()!=0.)aNext+=fwd()*thrustAccel();
      properV+=aNext*(h*.5);
      for(int k=0;k<10;k++){
        const double safeRadius=b[k].r+(k==3?100000.:0.);
        bool hit=norm(ship-b[k].p)<safeRadius;
        // Swept segment prevents 0.85c flights from tunneling through Earth.
        if(!hit && k==3){
          const V travel=ship-beforeMove, from=beforeMove-b[k].p;
          const double len2=dot(travel,travel);
          if(len2>0.){
            const double t=-dot(from,travel)/len2;
            if(t>0. && t<1.){
              const V near=from+travel*t;
              hit=dot(near,near)<safeRadius*safeRadius;
            }
          }
        }
        if(hit){
         crashed=true;crashBody=k;
         crashRelativeSpeed=norm(vel()-referenceVelocity(k));
         thrust=false;engine.clear();return;
        }
      }
   }
 }
 void warp(){ship+=fwd()*(AU*.07);update();for(const auto& x:b){double safe=x.r*2.+50000000.;V q=ship-x.p;if(norm(q)<safe)ship=x.p+unit(q)*safe;}}
};
} // namespace astro
