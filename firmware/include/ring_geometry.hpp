#pragma once
#include <cmath>
#include <algorithm>
#include "thrust_control.hpp"
namespace ring_geometry {
// Angle in screen coordinates (positive y down). 10h=-150deg/210deg, 6h=90deg, 2h=-30deg.
constexpr double kPiRing=3.14159265358979323846;
constexpr double BEGIN=7.0*kPiRing/6.0, END=-kPiRing/6.0;
constexpr double SPAN=4.0*kPiRing/3.0;
inline double angleForThrottle(double t){ return kPiRing/2.0-std::max(-1.0,std::min(1.0,t))*2.0*kPiRing/3.0; }
inline double normAngle(double x,double y) {
 double a=std::atan2(y,x);
 if(a<END-0.24) a+=2*kPiRing;
 return a;
}
// Permit a little extra room beyond 10h/2h for a finger to reach full travel.
inline bool inArc(double x,double y){
 double a=normAngle(x,y);
 return a>=END-0.24 && a<=BEGIN+0.24;
}
// The user's finger need not sit exactly on the illuminated 209px radius.
inline bool onThrottleBand(double x,double y){
 const double radius=std::sqrt(x*x+y*y);
 return radius>=146.0 && radius<=252.0 && inArc(x,y);
}
inline double fromRingXY(double x,double y){
 double a=normAngle(x,y);
 // Outside the upper gap, stick to the NEAREST endpoint; do not snap from
 // full forward straight to full reverse when crossing two o'clock.
 if(a>BEGIN) a=(a<(3.0*kPiRing/2.0))?BEGIN:END;
 double t=thrust_control::normalize((kPiRing/2.0-a)/(2.0*kPiRing/3.0));
 // Avoid demanding pixel-perfect contact with the watch's clipped arc ends.
 if(t>=.92)return 1.0;
 if(t<=-.92)return -1.0;
 return t;
}
}
