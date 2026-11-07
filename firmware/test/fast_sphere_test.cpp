#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include "fast_sphere.hpp"
int main(){
 using namespace fast_sphere;
 std::unique_ptr<Cell[]> cells(new Cell[kCount]);
 build(cells.get());
 auto at=[&](int x,int y)->const Cell&{return cells[y*kGrid+x];};
 assert(sizeof(Cell)==6 && kCount*sizeof(Cell)==396294);
 assert(valid(at(128,128)) && !valid(at(0,0)));
 assert(std::abs(int(at(128,128).u)-32768)<2);
 assert(std::abs(int(at(128,128).v)-32768)<2);
 assert(at(128,128).z==16384);
 assert(at(128,0).v==0 && at(128,256).v==65535);
 assert(texU(at(128,128),32768,512)==0); // half revolution
 assert(lookupIndex(-220,220)==0 && lookupIndex(220,220)==256);
 assert(normalQ14(-220,220)==-16384 && normalQ14(220,220)==16384);
 assert(diffuseQ8(0,0,0,0,0,0,16384)==256);
 // Compare table-based projection vs R13 trig projection over interior.
 double maxU=0,maxV=0,meanU=0,meanV=0; int samples=0;
 constexpr double pi=3.14159265358979323846;
 for(int y=-116;y<=116;y+=2)for(int x=-116;x<=116;x+=2){
   double nx=x/128.,ny=y/128.; double rr=nx*nx+ny*ny;
   if(rr>0.92)continue;
   auto& c=at(lookupIndex(x,128),lookupIndex(-y,128));
   if(!valid(c))continue;
   double u=atan2(nx,sqrt(1-rr))/(2*pi)+.5;
   double v=.5-asin(ny)/pi;
   double du=std::abs(c.u/65536.0-u);
   double dv=std::abs(c.v/65536.0-v);
   maxU=std::max(maxU,du);maxV=std::max(maxV,dv);meanU+=du;meanV+=dv;++samples;
 }
 assert(maxU<0.00005 && maxV<0.00005); // exact LUT grid test points
 std::cout<<"PASS fast_sphere: "<<samples<<" samples, ";
 std::cout<<"table="<<kCount*sizeof(Cell)<<" bytes, max(U,V)=("<<maxU<<","<<maxV<<")\n";
}
