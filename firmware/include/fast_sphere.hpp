#pragma once
// R14: portable sphere UV LUT. One-time nonlinear geometry, no trig in pixel loop.
// All coordinates are relative to the visible hemisphere; perspective-free map
// matches the R13 orthographic disk. Entries are 6 bytes (396,294 bytes total).
#include <stdint.h>
#include <stddef.h>
#include <math.h>
namespace fast_sphere {
static constexpr int kHalf = 128;
static constexpr int kGrid = 257;
static constexpr int kCount = kGrid*kGrid;
static constexpr uint16_t kOutside = 65535u;
struct Cell {uint16_t u, v, z;};
static_assert(sizeof(Cell)==6,"sphere UV table must be 6 bytes/entry");
inline int clampI(int n,int a,int b){return n<a?a:(n>b?b:n);}
inline int lookupIndex(int delta,int radius){
  return clampI(128+(delta*128)/radius,0,256);
}
inline int normalQ14(int delta,int radius){
  return clampI(delta*16384/radius,-16384,16384);
}
inline void build(Cell* cells){
  if(!cells) return;
  constexpr double pi=3.14159265358979323846;
  for(int iy=0;iy<kGrid;++iy){
    const double ny=(128.0-iy)/128.0;
    for(int ix=0;ix<kGrid;++ix){
      const double nx=(ix-128.0)/128.0;
      const double t=nx*nx+ny*ny;
      Cell& c=cells[iy*kGrid+ix];
      if(t>1.0){c.u=c.v=0;c.z=kOutside;continue;}
      const double nz=sqrt(t>=1.0?0.0:1.0-t);
      const double u=(atan2(nx,nz)/(2.0*pi)+0.5)*65536.0;
      const double v=(0.5-asin(ny)/pi)*65536.0;
      c.u=uint16_t(clampI(int(u),0,65535));
      c.v=uint16_t(clampI(int(v),0,65535));
      c.z=uint16_t(clampI(int(nz*16384.0+0.5),0,16384));
    }
  }
}
inline bool valid(const Cell& c){return c.z!=kOutside;}
inline int texU(const Cell& c,uint16_t phase,int width){
  return (int)((uint32_t(uint16_t(c.u+phase))*unsigned(width))>>16);
}
inline int texV(const Cell& c,int height){
  return (int)((uint32_t(c.v)*unsigned(height))>>16);
}
inline int diffuseQ8(int idx,int nxQ,int nyQ,int lxQ,int lyQ,int lzQ,int nzQ){
  if(idx==0)return 256;
  int dot=(nxQ*lxQ+nyQ*lyQ-nzQ*lzQ)>>14; // all Q14
  return clampI(44+((243*dot)>>14),18,294);
}
inline uint16_t shade565(uint16_t color,int diffuse,int dr,int dg,int db){
  int r=clampI((((color>>11)&31)*diffuse)>>8,0,31);
  int g=clampI((((color>>5)&63)*diffuse)>>8,0,63);
  int b=clampI(((color&31)*diffuse)>>8,0,31);
  return uint16_t(((r*dr/31)<<11)|((g*dg/63)<<5)|(b*db/31));
}
}
