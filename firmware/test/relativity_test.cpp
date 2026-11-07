#include "relativity.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 using namespace relativity; using astro::V;
 for(double b:{0.,.01,.1,.5,.8,.99,.9999}){
   double dFront=factor(b,1.),dRear=factor(b,-1.);
   assert(std::abs(dFront*dRear-1.)<1.e-7);
   assert(std::abs(dFront-std::sqrt((1.+b)/(1.-b)))<1.e-7);
   assert(std::isfinite(dRear)&&std::isfinite(dFront));
   auto front=aberrate({0,0,1},{0,0,1},b),side=aberrate({1,0,0},{0,0,1},b);
   assert(std::abs(astro::norm(front)-1.)<1.e-9);
   assert(std::abs(astro::norm(side)-1.)<1.e-9);
   assert(side.z>=b-1.e-9);
 }
 auto one=sample(1.),red=sample(.5),blue=sample(2.);
 auto R=[](Sample s){return int((s.rgb565>>11)&31);};
 auto B=[](Sample s){return int(s.rgb565&31);};
 assert(R(one)>28&&B(one)>28);
 assert(R(red)>B(red));
 assert(B(blue)>R(blue));
 Palette p;p.update(.8);assert(p.lookup(1.).rgb565!=p.lookup(-1.).rgb565);
 std::cout<<"PASS: relativistic Doppler, aberration, visible-spectrum LUT\n";
}
