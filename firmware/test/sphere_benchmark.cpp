// Quick host-only comparison; not a StopWatch FPS claim.
#include "fast_sphere.hpp"
#include <chrono>
#include <memory>
#include <cstdio>
#include <cmath>
int main(){
 using namespace fast_sphere;
 std::unique_ptr<Cell[]> lut(new Cell[kCount]);build(lut.get());
 constexpr int R=200,W=512,H=256;
 volatile unsigned long long sum=0;
 using Clock=std::chrono::steady_clock;
 auto now=[](){return Clock::now();};
 auto slow=[&](){
  unsigned long long result=0;
  for(int y=-R;y<=R;++y){double ny=-double(y)/R;
   for(int x=-R;x<=R;++x){double nx=double(x)/R, rr=nx*nx+ny*ny;if(rr>1.)continue;
    double nz=sqrt(1.-rr),u=atan2(nx,nz)/(2*3.141592653589793)+.5+.117;
    u-=floor(u);
    int vx=(int)(u*W),vy=(int)((.5-asin(ny)/3.141592653589793)*H);
    result+=vx+vy;
   }
  }
  return result;
 };
 auto fast=[&](){
  unsigned long long result=0;
  int xi[401];for(int x=-R;x<=R;++x)xi[x+R]=lookupIndex(x,R);
  for(int y=-R;y<=R;++y){
   const Cell* row=&lut[lookupIndex(y,R)*kGrid];
   for(int x=-R;x<=R;++x){const Cell& c=row[xi[x+R]];if(!valid(c))continue;
    int ux=texU(c,uint16_t(.117*65536),W),vy=texV(c,H);result+=ux+vy;
   }
  }
  return result;
 };
 sum+=slow();sum+=fast(); // warmup
 auto t0=now();for(int i=0;i<3;++i)sum+=slow();auto t1=now();
 for(int i=0;i<3;++i)sum+=fast();auto t2=now();
 double oldms=std::chrono::duration<double,std::milli>(t1-t0).count()/3.;
 double newms=std::chrono::duration<double,std::milli>(t2-t1).count()/3.;
 printf("HOST ONLY UV projection 401x401: old %.3f ms, LUT %.3f ms, ratio %.1fx (sum=%llu)\n",oldms,newms,oldms/newms,sum);
}
