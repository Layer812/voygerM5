// STAR VOYAGER R16F8 / M5Stack StopWatch ESP32-S3 / M5 0.3.0, OTA0 boot layout
// JPL-approximate planet positions, Newtonian gravity + relativistic speed cap,
// 6-axis IMU relative attitude, edge-touch throttle, texture mapped bodies,
// atmospheric entry reset, and fictional warp jump.
// NASA Earth/Moon maps optional via GET_REAL_TEXTURES.cmd; placeholders otherwise.
#include <M5Unified.h>
#include <M5GFX.h>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include "astro.hpp"
#include "voyagers.hpp"
#include "warp_arrival.hpp"
#include "warp_timing.hpp"
#include "stars.hpp"
#include "relativity.hpp"
#include "planet_textures.hpp"
#include "haptics.hpp"
#include "ring_geometry.hpp"
#include "button_latch.hpp"
#include "hardware_buttons.hpp"
#include "tutorial_assets.hpp"
#include "fast_sphere.hpp"
#include "fast_sphere_data.hpp"
#include <esp_heap_caps.h>
using astro::V;

static astro::Sim sim;
static M5Canvas screen(&M5.Display);
static constexpr int S=466,CTR=233;
static uint32_t prevUs=0, lastDraw=0, impactMs=0, warpMs=0;
static bool warpApplied=false;
static bool longRangeWarpActive=false;
static warp_timing::FrameClock warpFrameClock;
static uint8_t lastMotorPower=255;
static void tickHaptics(uint32_t now) {
 uint8_t power=haptics::output(warpMs?warpFrameClock.elapsedMs:uint32_t(now-warpMs),warpMs!=0,
                               uint32_t(now-impactMs),impactMs!=0,longRangeWarpActive);
 if(power!=lastMotorPower){M5.Power.setVibration(power);lastMotorPower=power;}
}
static V warpEnd{}, warpArrivalVelocity{};
static double warpArrivalYaw=0., warpArrivalPitch=0.;
static int warpArrivalTarget=0;
static uint32_t arrivalGlowMs=0, arrivalFocusUntil=0;
static int navIndex=0; // 0=FREE, 1..9=planet/Moon, 10/11=Voyager 1/2
static constexpr int NAV_COUNT=12;
static bool warpMenu=false, hudVisible=true, rightBlockedUntilRelease=false;
// Two accepted presses within 450ms start an untargeted 1.125 light-year jump.
static bool leftTapPending=false;
static uint32_t lastLeftTapMs=0;
static constexpr double SHIP_MASS_KG=65000.;
// StopWatch physical key A (yellow, GPIO2) and B (blue, GPIO1) are active LOW.
// Poll raw GPIO alongside M5Unified to retain brief presses during long frames.
static button_latch::Key leftKey, rightKey;
static button_latch::PressGate leftPressGate,rightPressGate;
// Input task on core 0 samples physical keys every 5ms even during expensive
// planet rasterization. M5.update stays exclusively on the Arduino loop core.
// If task creation fails, fall back to the known-good VinylM5 BtnA/BtnB API.
static bool leftM5Edge=false,rightM5Edge=false;
static void sampleButtons(bool refreshM5=false) {
  // M5.update() runs once per Arduino loop. Extra calls during slow rendering
  // refresh the latched button states only; the core-0 raw GPIO task keeps taps.
  if(refreshM5)M5.update();
  const uint32_t now=millis();
  leftKey.update(M5.BtnA.isPressed(),now);
  rightKey.update(M5.BtnB.isPressed(),now);
  // Capture M5Unified's own debounced transitions as well. In particular,
  // brief button presses should not be hidden by a long frame.
  if(M5.BtnA.wasPressed())leftM5Edge=true;
  if(M5.BtnB.wasPressed())rightM5Edge=true;
}
static bool leftHeld(){return leftKey.held()||hardware_buttons::leftDown();}
static bool rightHeld(){return rightKey.held()||hardware_buttons::rightDown();}
static bool leftPressed(){
 // M5Unified edge and GPIO sampler edge may arrive in DIFFERENT frames.
 // One physical press must produce only one warp-menu toggle.
 const bool edge=leftKey.takePress();
 const bool raw=hardware_buttons::takeLeftPress();
 const bool m5=leftM5Edge;
 leftM5Edge=false;
 return leftPressGate.accept(edge || raw || m5,leftHeld(),millis());
}
static bool rightPressed(){
 const bool edge=rightKey.takePress();
 const bool raw=hardware_buttons::takeRightPress();
 const bool m5=rightM5Edge;
 rightM5Edge=false;
 return rightPressGate.accept(edge || raw || m5,rightHeld(),millis());
}
static uint32_t neutralCueUntil=0;

// Touch operation is primary in R9. BMI270 yaw integration was removed here
// so resting the watch cannot unexpectedly override deliberate swipes.
static bool warpActive(){return warpMs!=0;}
static V targetPosition(int n){if(n>=10)return astro::voyagerPosition(n-10,sim.jd);return sim.b[n].p;}
static const char* navName(int n){if(n==0)return "FREE FLIGHT";if(n>=10)return astro::VOYAGER_SPECS[n-10].name;return sim.b[n].name;}
static void selectTarget(int n){navIndex=(n+NAV_COUNT)%NAV_COUNT;if(!navIndex)return;
 // Selecting a warp destination never changes ship attitude.
}
static void beginWarp(bool longJump=false){
 if(warpActive()||sim.crashed)return;
 // Calculate a camera-ready arrival point, not just a coordinate jump.
 // Keep the destination's velocity so a planet/probe does not immediately
 // fly out of view after warp. Warp menu FREE FLIGHT keeps original motion.
 const auto a=warp_arrival::calculate(sim,longJump?0:navIndex);
 // A long jump always follows the raw nose direction. No steering correction.
 warpEnd=longJump ? sim.ship+sim.fwd()*astro::LONG_WARP_STEP : a.ship;
 warpArrivalVelocity=a.properVelocity;
 warpArrivalYaw=a.yaw;warpArrivalPitch=a.pitch;
 warpArrivalTarget=longJump?0:navIndex;
 warpApplied=false;arrivalGlowMs=0;arrivalFocusUntil=0;
 longRangeWarpActive=longJump;
 warpFrameClock.start(longJump?warp_timing::LONG_TOTAL_MS:warp_timing::TOTAL_MS);
 warpMs=millis();sim.thrust=false;sim.engine.clear();
 // Motor is driven by the non-blocking vibration envelope in tickHaptics().
}
static void tickWarp(uint32_t now){
 if(!warpActive())return;
 const float t=warpFrameClock.progress();
 if(t>=.53f&&!warpApplied){
   sim.ship=warpEnd;
   if(warpArrivalTarget){
     sim.properV=warpArrivalVelocity;
     sim.yaw=warpArrivalYaw;sim.pitch=warpArrivalPitch;
   }
   warpApplied=true;
 }
 if(t>=1){
   warpMs=0;warpApplied=false;longRangeWarpActive=false;
   arrivalGlowMs=now;
   // Only briefly track the object while the warp-exit glow fades.
   arrivalFocusUntil=warpArrivalTarget?now+warp_arrival::ARRIVAL_GLOW_MS:0;
 }
}
static void maintainArrivalFocus(uint32_t now){
 if(!arrivalFocusUntil || int32_t(now-arrivalFocusUntil)>=0){
   arrivalFocusUntil=0;return;
 }
 if(warpArrivalTarget==0)return;
 const V aim=targetPosition(warpArrivalTarget)-sim.ship;
 if(astro::norm(aim)>1.){
   sim.yaw=atan2(aim.x,aim.z);
   sim.pitch=asin(std::max(-1.,std::min(1.,astro::unit(aim).y)));
 }
}

static bool ringTouch=false,touchLast=false,gestureMoved=false,longDone=false;
static uint32_t touchBeginMs=0;
static float touchStartX=0,touchStartY=0,lastX=0,lastY=0;
static float menuGestureX=0,menuGestureY=0;
static int timeScale=20;
static bool tutorialOpen=true, tutorialPainted=false;
static int tutorialPage=0;
static bool tutorialTouchPrev=false;
static float tutorialStartX=0, tutorialStartY=0,tutorialLastX=0,tutorialLastY=0;
static uint32_t flightTitleStart=0;
static constexpr uint32_t TITLE_FADE_MS=2800;
static void drawTutorialPage();
// One display owner: M5.begin(cfg) only. Never invoke M5.Display.init().
// BOARD_ID=30: M5StopWatch fixed-board startup, avoiding unrelated board detection.
// A successful boot splash must not depend on PSRAM sprite allocation.
// Verify the actual RGB565 layout of M5Canvas rather than assuming its raw
// PSRAM format. R14 bypassed the renderer and could swap R/B or byte order.
static bool haveSprite=false, displayReady=false;
enum class RawLayout : uint8_t { API_ONLY, RGB565, BYTESWAP, BGR565, BGR_BYTESWAP };
static RawLayout rawLayout=RawLayout::API_ONLY;
static uint16_t swap16(uint16_t c){return uint16_t((c<<8)|(c>>8));}
static uint16_t swapRB(uint16_t c){return uint16_t(((c&31u)<<11)|(c&0x07E0u)|((c>>11)&31u));}
static uint16_t framebufferColor(uint16_t c){
 switch(rawLayout){
   case RawLayout::RGB565: return c;
   case RawLayout::BYTESWAP: return swap16(c);
   case RawLayout::BGR565: return swapRB(c);
   case RawLayout::BGR_BYTESWAP: return swap16(swapRB(c));
   default: return c;
 }
}
static bool framebufferRawOK(){return rawLayout!=RawLayout::API_ONLY;}
static void detectSpritePixelFormat(){
 rawLayout=RawLayout::API_ONLY;
 if(!haveSprite)return;
 uint16_t* pixels=static_cast<uint16_t*>(screen.getBuffer());
 if(!pixels)return;
 // Red + green probes reveal both component order and byte order.
 screen.drawPixel(0,0,uint16_t(0xF800));
 screen.drawPixel(1,0,uint16_t(0x07E0));
 const uint16_t red=pixels[0],green=pixels[1];
 if(red==0xF800u&&green==0x07E0u)rawLayout=RawLayout::RGB565;
 else if(red==0x00F8u&&green==0xE007u)rawLayout=RawLayout::BYTESWAP;
 else if(red==0x001Fu&&green==0x07E0u)rawLayout=RawLayout::BGR565;
 else if(red==0x1F00u&&green==0xE007u)rawLayout=RawLayout::BGR_BYTESWAP;
 Serial.printf("R15 RGB565 probe red=%04X green=%04X layout=%d\n",red,green,int(rawLayout));
}
static int physicalW=466,physicalH=466;
static uint32_t bootSplashUntil=0;
static lgfx::LGFXBase* drawTarget=&M5.Display;
static lgfx::LGFXBase& canvas() { return *drawTarget; }
static void bootSplash(){
  // Independent R13 FIX3 first-frame sequence: draw directly before PSRAM work.
  // M5Unified owns the AMOLED, PMIC and touch setup. Do not touch their GPIOs.
  if (!displayReady) return;
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.fillRect(0,0,physicalW,92,0x2AE5u);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::Font4);
  M5.Display.setTextSize(1.0f);
  M5.Display.setTextColor(TFT_WHITE,0x2AE5u);
  M5.Display.drawString("voygerM5",physicalW/2,66);
}

static void drawTutorialPage(){
 if(!displayReady)return;
 // Paint directly to LCD once per page, independently of the large flight sprite.
 M5.Display.drawJpg(kTutorialPages[tutorialPage],kTutorialPageSizes[tutorialPage],
                    (physicalW-S)/2,(physicalH-S)/2);
 tutorialPainted=true;
}
static void startFlight(){
 tutorialOpen=false;flightTitleStart=millis();
 leftTapPending=false;
 (void)hardware_buttons::takeLeftDoublePress(); // discard tutorial button sequences
 prevUs=micros();lastDraw=0;touchLast=false;rightBlockedUntilRelease=true;
 // Commit the first flight pixels directly before the expensive planet/space
 // renderer runs.  A slow first full-screen sprite cannot leave a black LCD.
 M5.Display.fillScreen(0x0000);
 M5.Display.setTextDatum(middle_center);
 M5.Display.setFont(&fonts::FreeMonoBold12pt7b);
 M5.Display.setTextColor(0x8e7e);
 M5.Display.drawString("voygerM5",physicalW/2,physicalH/2);
 Serial.println("STAR_VOYAGER_R13 FIRST_FLIGHT_FRAME_DIRECT / FLIGHT READY");
}
static uint32_t starRandState=26;
static uint32_t nextRand(){starRandState=1664525u*starRandState+1013904223u;return starRandState;}
static float rand01(){return float((nextRand()>>8)&0xffffff)/16777216.f;}
struct StarPoint {float x,y,z,mag;bool bright;};
static StarPoint starList[1050];static int starCount=0;
static void prepareStars(){
  for(auto s:BRIGHT_STARS){double ra=s.ra*astro::DEG,dc=s.dec*astro::DEG;
    double x=cos(dc)*cos(ra),y=cos(dc)*sin(ra),z=sin(dc),ob=23.43928*astro::DEG;
    starList[starCount++]={float(x),float(cos(ob)*y+sin(ob)*z),float(-sin(ob)*y+cos(ob)*z),s.mag,true};}
  while(starCount<(int)(sizeof(starList)/sizeof(starList[0]))){float z=2*rand01()-1,a=rand01()*2*astro::kPi,r=sqrtf(1-z*z);
    starList[starCount++]={r*cosf(a),r*sinf(a),z,3.6f+rand01()*3.0f,false};}
}
static uint16_t rgb(uint8_t r,uint8_t g,uint8_t b){return (uint16_t(r>>3)<<11)|(uint16_t(g>>2)<<5)|(b>>3);}
static uint16_t tint(uint16_t c,float k){k=std::max(0.f,std::min(1.6f,k));
  int r=std::min(31,int(((c>>11)&31)*k)),g=std::min(63,int(((c>>5)&63)*k)),b=std::min(31,int((c&31)*k));return (r<<11)|(g<<5)|b;}
struct View {V f,r,u;};
static View view(){double sy=sin(sim.yaw),cy=cos(sim.yaw),sp=sin(sim.pitch),cp=cos(sim.pitch);
 return {{sy*cp,sp,cy*cp},{cy,0,-sy},{-sy*sp,cp,-cy*sp}};}
// Touch + button sampling must also run during slow full-screen planet painting:
// without it a finger can traverse half the ring between two input reads.
static void controls(double dt);
static relativity::Palette dopplerPalette; // 129-color precomputed visible-band table
static void drawStars(View cam){
 V vel=sim.vel(),veldir=astro::unit(vel);
 const double beta=sim.beta();
 dopplerPalette.update(beta);
 // The ESP32-S3 has fast single-precision FP but slow software double math.
 // At ordinary velocities, 1050 direction transforms use float instead of
 // multiple 64-bit dot products per star. Exact aberration retained >=0.004c.
 if(beta<=.004){
   const float fx=cam.f.x,fy=cam.f.y,fz=cam.f.z;
   const float rx=cam.r.x,ry=cam.r.y,rz=cam.r.z;
   const float ux=cam.u.x,uy=cam.u.y,uz=cam.u.z;
   const float vx=veldir.x,vy=veldir.y,vz=veldir.z;
   for(int i=0;i<starCount;i++){
     if((i&127)==0){sampleButtons();delay(0);}
     const StarPoint &s=starList[i];
     const float z=s.x*fx+s.y*fy+s.z*fz;
     if(z<=.15f)continue;
     const float invZ=380.f/z;
     const int x=int(CTR+(s.x*rx+s.y*ry+s.z*rz)*invZ);
     const int y=int(CTR-(s.x*ux+s.y*uy+s.z*uz)*invZ);
     const int dx=x-CTR,dy=y-CTR;
     if(dx*dx+dy*dy>204*204)continue;
     const float mu=s.x*vx+s.y*vy+s.z*vz;
     const auto& sample=dopplerPalette.lookup(mu);
     if(sample.visible<4)continue;
     const float magLight=std::max(.13f,1.f-(s.mag+1.f)*.12f);
     const uint16_t col=tint(sample.rgb565,magLight);
     canvas().drawPixel(x,y,col);
     if(s.bright&&s.mag<1.8f)canvas().fillCircle(x,y,s.mag<.4f?2:1,col);
   }
 }else{
   for(int i=0;i<starCount;i++){
     if((i&63)==0){sampleButtons();delay(0);}
     const StarPoint &s=starList[i];
     V p{s.x,s.y,s.z};
     const double mu=astro::dot(p,veldir);
     p=relativity::aberrate(p,veldir,beta);
     const double z=astro::dot(p,cam.f);if(z<=.15)continue;
     const int x=int(CTR+380*astro::dot(p,cam.r)/z);
     const int y=int(CTR-380*astro::dot(p,cam.u)/z);
     const int dx=x-CTR,dy=y-CTR;if(dx*dx+dy*dy>204*204)continue;
     const auto& sample=dopplerPalette.lookup(mu);if(sample.visible<4)continue;
     const uint16_t col=tint(sample.rgb565,std::max(.13f,1.f-(s.mag+1.f)*.12f));
     canvas().drawPixel(x,y,col);
     if(s.bright&&s.mag<1.8f)canvas().fillCircle(x,y,s.mag<.4f?2:1,col);
   }
 }
}
struct Projected {int idx,x,y,r;double dist;};
// One-time nonlinear UV lookup replaces per-pixel sqrt/atan2/asin.
// Size: 396294 bytes of PSRAM, allocated after the display boot splash.
static const fast_sphere::Cell* sphereLut=gSphereLookup;
static void buildSphereLut(){
 // Flash table is usable directly even if PSRAM allocation fails.
 // PSRAM mirror avoids flash cache misses when shading an entire planet.
 auto* mirror=(fast_sphere::Cell*)heap_caps_malloc(sizeof(fast_sphere::Cell)*fast_sphere::kCount,
                                                 MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
 if(mirror){
   memcpy(mirror,gSphereLookup,sizeof(fast_sphere::Cell)*fast_sphere::kCount);
   sphereLut=mirror;
   Serial.println("R14 precomputed sphere UV LUT copied to PSRAM");
 }else Serial.println("R14 sphere LUT using immutable flash table");
}
// 256 unit-circle directions for the Earth atmospheric limb.
// Computed once during setup; only integer rotations in the render loop.
struct AtmospherePoint {int16_t x,y;};
static AtmospherePoint atmosphereCircle[256];
static void prepareAtmosphereCircle(){
  for(int i=0;i<256;++i){
    const double a=(i*(2.*astro::kPi))/256.;
    atmosphereCircle[i]={int16_t(16384.*cos(a)),int16_t(16384.*sin(a))};
  }
}
static uint16_t alienRecolor(uint16_t src){
 // Reuse Earth's texture, with a distinct blue-green palette.
 const int r=((src>>11)&31)*2/5;
 const int g=std::min(63,((src>>5)&63)*6/5+5);
 const int b=std::min(31,(src&31)*4/5+3);
 return uint16_t((r<<11)|(g<<5)|b);
}
static void bodyDisk(const Projected& p,View cam){
 const bool alien=p.idx==10;
 const int texIdx=alien?3:p.idx;
 const auto &b=alien?sim.alienWorld:sim.b[p.idx];
 int x0=p.x,y0=p.y,R=std::min(220,p.r);
 if(R<2){canvas().drawPixel(x0,y0,alien?rgb(48,210,201):(p.idx==0?0xffa0:0xffff));return;}
 // A nearby fictional star illuminates the alien world, not our distant Sun.
 V sunlight=alien?astro::unit(V{-0.6,0.3,0.7}):(p.idx?astro::unit(sim.b[0].p-b.p):V{0,0,1});
 double lx=astro::dot(sunlight,cam.r),ly=astro::dot(sunlight,cam.u),lz=astro::dot(sunlight,cam.f);
 const int W=PLANET_TW[texIdx],H=PLANET_TH[texIdx];
 const double spin=fmod(sim.jd*.002,1.0);
 const uint16_t spinQ=uint16_t(unsigned(spin*65536.)&65535u);
 V vel=sim.vel();double mu=astro::dot(astro::unit(b.p-sim.ship),astro::unit(vel));
 const auto& shifted=dopplerPalette.lookup(mu);
 // Near everyday solar-system speeds the planets must retain actual NASA colors.
 // Stellar Doppler tint is reserved for relativistic flight; do not apply a
 // ~6500K stellar spectrum to a textured planet at 30 km/s.
 const bool relativistic=sim.beta()>.01;
 const int red=alien?31:(relativistic?((shifted.rgb565>>11)&31):31);
 const int green=alien?63:(relativistic?((shifted.rgb565>>5)&63):63);
 const int blue=alien?31:(relativistic?(shifted.rgb565&31):31);
 if(sphereLut){
   // Convert the 3D sunlight/camera rotation into one fixed-point light vector
   // per body. No matrix operations, divisions or trig in the inner pixel loop.
   const int lqx=int(lx*16384.),lqy=int(ly*16384.),lqz=int(lz*16384.);
   int16_t xs[466],xn[466];
   const int xmin=std::max(0,x0-R),xmax=std::min(S-1,x0+R);
   for(int sx=xmin;sx<=xmax;++sx){
     xs[sx]=int16_t(fast_sphere::lookupIndex(sx-x0,R));
     xn[sx]=int16_t(fast_sphere::normalQ14(sx-x0,R));
   }
   // RGB565 16-bit framebuffer in PSRAM; direct contiguous writes remove
   // one LGFX drawPixel() virtual/interface call per *planet pixel*.
   uint16_t* fb=(haveSprite && framebufferRawOK())?reinterpret_cast<uint16_t*>(screen.getBuffer()):nullptr;
   for(int sy=std::max(0,y0-R);sy<std::min(S,y0+R+1);++sy){
     if((sy&15)==0){sampleButtons();delay(0);}
     const int dy=sy-y0;
     const int circleSq=R*R-dy*dy;
     if(circleSq<0)continue;
     const int diskDx=int(sqrtf(float(circleSq)));
     const int cameraDy=sy-CTR,camSq=204*204-cameraDy*cameraDy;
     if(camSq<0)continue;
     const int viewDx=int(sqrtf(float(camSq)));
     const int xbegin=std::max(xmin,std::max(x0-diskDx,CTR-viewDx));
     const int xend=std::min(xmax,std::min(x0+diskDx,CTR+viewDx));
     if(xbegin>xend)continue;
     const int iy=fast_sphere::lookupIndex(dy,R);
     const int nyQ=fast_sphere::normalQ14(-dy,R);
     const fast_sphere::Cell* row=&sphereLut[iy*fast_sphere::kGrid];
     for(int sx=xbegin;sx<=xend;++sx){
       const fast_sphere::Cell& c=row[xs[sx]];
       if(!fast_sphere::valid(c))continue;
       const int u=fast_sphere::texU(c,spinQ,W),v=fast_sphere::texV(c,H);
       uint16_t src=pgm_read_word(PLANET_TEX[texIdx]+v*W+u);
       if(alien)src=alienRecolor(src);
       const int shade=fast_sphere::diffuseQ8(texIdx,xn[sx],nyQ,lqx,lqy,lqz,c.z);
       const uint16_t color=fast_sphere::shade565(src,shade,red,green,blue);
       if(fb)fb[sy*S+sx]=framebufferColor(color);
       else canvas().drawPixel(sx,sy,color);
     }
   }
 }else{
   // Low-memory fallback retains the older renderer to guarantee display.
   for(int sy=std::max(0,y0-R);sy<std::min(S,y0+R+1);++sy){
    if((sy&15)==0){sampleButtons();delay(0);}
    const double ny=double(y0-sy)/R;
    for(int sx=std::max(0,x0-R);sx<std::min(S,x0+R+1);++sx){
     const int ddx=sx-CTR,ddy=sy-CTR;if(ddx*ddx+ddy*ddy>204*204)continue;
     const double nx=double(sx-x0)/R,sr=nx*nx+ny*ny;if(sr>1)continue;
     const double nz=sqrt(std::max(0.,1.-sr)),lon=atan2(nx,nz);
     double uv=lon/(2*astro::kPi)+.5+spin;uv-=floor(uv);
     int u=(int)(uv*W),v=(int)((.5-asin(ny)/astro::kPi)*H);
     u=std::max(0,std::min(W-1,u));v=std::max(0,std::min(H-1,v));
     uint16_t c=pgm_read_word(PLANET_TEX[texIdx]+v*W+u);
     if(alien)c=alienRecolor(c);
     float diffuse=texIdx==0?1.f:float(std::max(.07,std::min(1.15,.17+.95*(nx*lx+ny*ly-nz*lz))));
     uint16_t lit=tint(c,diffuse);
     canvas().drawPixel(sx,sy,(uint16_t)((((lit>>11)&31)*red/31)<<11 | (((lit>>5)&63)*green/63)<<5 | ((lit&31)*blue/31)));
    }
   }
 }
 if(p.idx==7&&R>5){
  uint16_t col=rgb(204,186,138);for(int a=0;a<360;a+=2){float t=a*astro::DEG;
   int x=x0+int(cosf(t)*R*1.7f),y=y0+int(sinf(t)*R*.52f);
   if(x>=0&&x<S&&y>=0&&y<S&&((x-CTR)*(x-CTR)+(y-CTR)*(y-CTR)<204*204))canvas().drawPixel(x,y,col);}
 }
 if((p.idx==3||alien) && R>=9){
   // Atmospheric limb: an extremely thin, sunlight-dependent blue scatter,
   // not two full neon circles around the nightside. Solar-side and backlit
   // pixels can glow; the unilluminated limb fades into the background.
   // Sampled unit-circle directions are prepared once, not trig per frame.
   for(int i=0;i<256;++i){
     const float nx=float(atmosphereCircle[i].x)/16384.f;
     const float ny=-float(atmosphereCircle[i].y)/16384.f;
     const float side=nx*float(lx)+ny*float(ly);
     const float backlight=std::max(0.f,float(lz))*.22f;
     const float intensity=std::max(0.f,std::min(1.f,side+backlight));
     if(intensity<.12f)continue;
     const int cx=x0+(int)((int64_t(atmosphereCircle[i].x)*(R+1))>>14);
     const int cy=y0+(int)((int64_t(atmosphereCircle[i].y)*(R+1))>>14);
     if(cx<0||cy<0||cx>=S||cy>=S)continue;
     const int dx=cx-CTR,dy=cy-CTR;
     if(dx*dx+dy*dy>204*204)continue;
     const float factor=(intensity-.12f)/.88f;
     const uint16_t color=alien?rgb(uint8_t(2+8*factor),uint8_t(20+120*factor),uint8_t(18+96*factor)):
                               rgb(uint8_t(4+17*factor),uint8_t(15+75*factor),uint8_t(27+130*factor));
     canvas().drawPixel(cx,cy,color);
   }
 }
}
static void drawBodies(View cam){
 Projected visible[11];int n=0;
 for(int i=0;i<11;i++){
   const auto &b=i==10?sim.alienWorld:sim.b[i];V q=b.p-sim.ship;double dist=astro::norm(q);if(dist<=b.r)continue;
   V d=q/dist;if(sim.beta()>.004)d=relativity::aberrate(d,astro::unit(sim.vel()),sim.beta());double z=astro::dot(d,cam.f);if(z<.1)continue;
   double ar=asin(std::min(.9999,b.r/dist));int radius=std::min(220,(int)(380*tan(std::min(1.1,ar))));
   int x=(int)(CTR+380*astro::dot(d,cam.r)/z),y=(int)(CTR-380*astro::dot(d,cam.u)/z);
   if(hypot(double(x-CTR),double(y-CTR))>237+radius)continue;
   visible[n++]={i,x,y,radius,dist};
 }
 // Painter's algorithm: farther disks drawn first.
 for(int i=0;i<n;i++)for(int j=i+1;j<n;j++)if(visible[i].dist<visible[j].dist)std::swap(visible[i],visible[j]);
 for(int i=0;i<n;i++)bodyDisk(visible[i],cam);
}
// Voyager 1/2 are recognizable spacecraft silhouettes, not HUD markers:
// high-gain dish, instrument bus, antenna/boom and RTG. No labels/brackets.
// For the watch-sized display the model is artistically magnified at short
// range; it is not claimed to have real angular size.
static void drawVoyagerCraft(int x,int y,int id,int r){
 const uint16_t metal=rgb(185,203,217),dish=rgb(222,216,184),dark=rgb(64,80,97);
 const uint16_t antenna=id?rgb(246,185,123):rgb(139,219,240);
 canvas().drawLine(x-r,y+int(.28*r),x+r,y-int(.18*r),metal);
 canvas().drawLine(x+int(.15*r),y,x+int(.58*r),y+int(.72*r),metal);
 canvas().drawLine(x-int(.18*r),y,x-int(.78*r),y-int(.32*r),antenna);
 canvas().fillRoundRect(x-r/6,y, std::max(3,r/3),std::max(3,r/3),2,metal);
 // The dish is tilted to reveal its characteristic oval shape.
 canvas().drawEllipse(x-int(.10*r),y-int(.32*r),std::max(3,r/2),std::max(2,r/4),dish);
 canvas().drawEllipse(x-int(.10*r),y-int(.32*r),std::max(3,r/2),std::max(2,r/4),dark);
 canvas().drawLine(x-int(.10*r),y-int(.32*r),x,y+int(.13*r),dark);
 canvas().fillCircle(x+int(.58*r),y+int(.72*r),std::max(2,r/7),antenna);
}
static void drawProbes(View cam){
 for(int i=0;i<2;i++){
   const V diff=astro::voyagerPosition(i,sim.jd)-sim.ship;
   const double d=astro::norm(diff);
   if(d<1. || d>warp_arrival::PROBE_VISIBLE_METERS)continue;
   const V p=diff/d;
   const double z=astro::dot(p,cam.f);
   if(z<.11)continue;
   const int x=int(CTR+380*astro::dot(p,cam.r)/z);
   const int y=int(CTR-380*astro::dot(p,cam.u)/z);
   if(hypot(double(x-CTR),double(y-CTR))>195)continue;
   const int apparent=warp_arrival::probeVisualRadius(d);
   drawVoyagerCraft(x,y,i,apparent);
 }
}
static void drawWarpFX(){
 if(!warpActive())return;
 float t=warpFrameClock.progress();
 if(t<.34f){
    for(int i=0;i<7;i++){
      float u=fmodf(i/7.f+t/.34f*.55f,1.f);
      int rx=24+(int)(u*167),ry=(int)(rx*(.55f+.13f*sinf(millis()*.004f+i)));
      int ox=CTR,oy=CTR;uint16_t color=(i&1)?rgb(64,155,255):rgb(115,218,255);
      for(int j=0;j<100;j++){
        float a=j*(2*astro::kPi/100),b=(j+1)*(2*astro::kPi/100);
        int x0=ox+rx*cosf(a),y0=oy+ry*sinf(a),x1=ox+rx*cosf(b),y1=oy+ry*sinf(b);
        if(hypot(double(x0-CTR),double(y0-CTR))<201&&hypot(double(x1-CTR),double(y1-CTR))<201)
          canvas().drawLine(x0,y0,x1,y1,color);
      }
    }
 }
 if(t>.26f&&t<.86f){
   const float phase=(t-.26f)/.60f;
   for(int i=0;i<92;i++){
    float a=i*2.399963f,r0=fmodf(i*.619f+phase*3.7f,1.f);
    int r=14+(int)(r0*r0*186.f),span=8+(int)(phase*47*r0);
    int x0=CTR+(int)(cosf(a)*r),y0=CTR+(int)(sinf(a)*r);
    int x1=CTR+(int)(cosf(a)*std::min(205,r+span)),y1=CTR+(int)(sinf(a)*std::min(205,r+span));
    if(hypot(double(x0-CTR),double(y0-CTR))<203&&hypot(double(x1-CTR),double(y1-CTR))<203)
      canvas().drawLine(x0,y0,x1,y1,(i%5==0)?rgb(255,206,175):rgb(103,189,255));
   }
 }
 if((t>.47f&&t<.58f)||(t>.83f&&t<.96f)){
    float flash=t<.58f?(1-fabsf(t-.525f)/.055f):(1-fabsf(t-.895f)/.065f);
    if(flash>.4f){uint16_t col=flash>.75f?rgb(191,228,255):rgb(56,104,168);
      for(int j=0;j<200;j++){
       float angle=j*2*astro::kPi/200.; int r=208;
       canvas().drawLine(CTR,CTR,CTR+r*cos(angle),CTR+r*sin(angle),col);
      }
    }
 }
 canvas().setTextDatum(middle_center);canvas().setFont(&fonts::FreeMonoBold12pt7b);
 canvas().setTextColor(rgb(220,243,255));
 const char* warpPhase=t<.29f?"WARP CHARGING":t<.83f?(longRangeWarpActive?"LONG-RANGE WARP":"WARP TRANSIT"):"WARP EXIT";
 canvas().drawString(warpPhase,233,172);
}
// Warp-exit animation is a short peripheral glow, NOT a target marker.
// It fades out completely, leaving only the real rendered body or craft.
static void drawArrivalGlow(){
 if(!arrivalGlowMs)return;
 const uint32_t elapsed=millis()-arrivalGlowMs;
 if(elapsed>=warp_arrival::ARRIVAL_GLOW_MS){arrivalGlowMs=0;return;}
 const float strength=1.f-float(elapsed)/warp_arrival::ARRIVAL_GLOW_MS;
 const uint16_t c=rgb(uint8_t(12+75*strength),
                      uint8_t(27+122*strength),
                      uint8_t(45+155*strength));
 const int r=194+int(5*strength);
 canvas().drawCircle(CTR,CTR,r,c);
 if(strength>.5f)canvas().drawCircle(CTR,CTR,r+1,c);
}
static uint16_t ringColor(double t){
 // Zero (6 o'clock) = white. Reverse fades to cyan/blue, forward to yellow/red.
 t=std::max(-1.0,std::min(1.0,t));
 int r,g,b;
 if(t<0){double f=-t;
   if(f<.56){double k=f/.56;r=int(247+(69-247)*k);g=int(250+(228-250)*k);b=int(250+(255-250)*k);}
   else {double k=(f-.56)/.44;r=int(69+(24-69)*k);g=int(228+(68-228)*k);b=int(255+(218-255)*k);}
 }else if(t<.55){double k=t/.55;r=int(247+(255-247)*k);g=int(250+(216-250)*k);b=int(250+(70-250)*k);}
 else {double k=(t-.55)/.45;r=255;g=int(216+(55-216)*k);b=int(70+(63-70)*k);}
 return rgb(r,g,b);
}
// The ring changes only when its pointer moves. Paint the thick 8px ring
// with one cached PSRAM pixel list; NO per-frame sin/cos and NO hundreds of
// expensive LGFX line draw calls. A purely-LGFX fallback remains available.
struct RingSegment{int16_t x0,y0,x1,y1;uint16_t col;};
static RingSegment ringSegments[240][5];
struct RingTick{int16_t x0,y0,x1,y1;};
static RingTick ringTicks[13];
struct RingPixel{uint32_t offset;uint16_t color;};
static RingPixel* ringRaster=nullptr;
static size_t ringRasterCount=0;
static bool ringPrepared=false;
static void prepareRingGeometry(){
 for(int j=0;j<240;j++){
   const double mid=-1.+(j+.5)/120.;
   const double a0=ring_geometry::angleForThrottle(-1.+j/120.);
   const double a1=ring_geometry::angleForThrottle(-1.+(j+1.)/120.);
   for(int k=0;k<5;k++){
     const int radius=205+k*2;
     ringSegments[j][k]={int16_t(CTR+int(radius*cos(a0))),int16_t(CTR+int(radius*sin(a0))),
                         int16_t(CTR+int(radius*cos(a1))),int16_t(CTR+int(radius*sin(a1))),ringColor(mid)};
   }
 }
 for(int j=0;j<=12;j++){
   const double a=ring_geometry::angleForThrottle(-1.+j/6.);
   ringTicks[j]={int16_t(CTR+int(197*cos(a))),int16_t(CTR+int(197*sin(a))),
                 int16_t(CTR+int(215*cos(a))),int16_t(CTR+int(215*sin(a)))};
 }
 if(haveSprite && framebufferRawOK()){
   // Radius 205..213, 240 degrees, under 9k pixels. Allocate a conservative
   // cap in PSRAM rather than wasting precious ESP32-S3 internal SRAM.
   constexpr size_t kCapacity=11000;
   ringRaster=static_cast<RingPixel*>(heap_caps_malloc(sizeof(RingPixel)*kCapacity,
                                                       MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
   if(ringRaster){
     for(int y=0;y<S;y++)for(int x=0;x<S;x++){
       const int dx=x-CTR,dy=y-CTR;
       const int d2=dx*dx+dy*dy;
       if(d2<205*205 || d2>213*213)continue;
       const double a=ring_geometry::normAngle(dx,dy);
       if(a<ring_geometry::END || a>ring_geometry::BEGIN)continue;
       const double t=std::max(-1.,std::min(1.,
          (ring_geometry::kPiRing*.5-a)/(ring_geometry::kPiRing*2./3.)));
       if(ringRasterCount<kCapacity)
         ringRaster[ringRasterCount++]={uint32_t(y*S+x),ringColor(t)};
     }
     Serial.printf("R15 RING_CACHE pixels=%u\n",unsigned(ringRasterCount));
   }
 }
 ringPrepared=true;
}
static void drawRing(){
 if(!ringPrepared)prepareRingGeometry();
 const double t=std::max(-1.,std::min(1.,sim.throttle));
 uint16_t* fb=ringRaster&&haveSprite&&framebufferRawOK()?
   static_cast<uint16_t*>(screen.getBuffer()):nullptr;
 if(fb){
   for(size_t j=0;j<ringRasterCount;++j){
     const RingPixel &p=ringRaster[j];fb[p.offset]=framebufferColor(p.color);
   }
 }else{
   for(int j=0;j<240;j++){
     if((j&63)==0){sampleButtons();delay(0);}
     for(int k=0;k<5;k++){
       const RingSegment &s=ringSegments[j][k];
       canvas().drawLine(s.x0,s.y0,s.x1,s.y1,s.col);
     }
   }
 }
 for(int j=0;j<=12;j++){
   const RingTick &p=ringTicks[j];
   canvas().drawLine(p.x0,p.y0,p.x1,p.y1,rgb(223,239,249));
 }
 const double a=ring_geometry::angleForThrottle(t);
 const int px=CTR+int(209*cos(a)),py=CTR+int(209*sin(a));
 canvas().fillCircle(px,py,9,rgb(9,37,52));
 canvas().drawCircle(px,py,9,rgb(122,240,248));
 canvas().fillCircle(px,py,6,rgb(255,255,255));
 if(hudVisible){
   canvas().setFont(&fonts::FreeMonoBold12pt7b);canvas().setTextDatum(middle_center);
   canvas().setTextColor(rgb(92,216,250));canvas().drawString("REV",76,232);
   canvas().setTextColor(rgb(255,184,86));canvas().drawString("FWD",390,232);
 }
}
static void drawMinimalHUD(){
 if(!hudVisible)return;
 char msg[50];
 const double speed=sim.localSpeed(); // gravity-dominant body-relative speed
 // ACC is the signed component of gravity + active engine along the ship's nose.
 const double acc=astro::dot(sim.gravity(),sim.fwd())+
                  sim.thrustAccel();
 canvas().setTextDatum(middle_center);canvas().setFont(&fonts::FreeMonoBold12pt7b);
 const uint16_t border=rgb(85,209,229);
 canvas().fillRoundRect(112,326,242,36,17,rgb(2,20,35));
 canvas().drawRoundRect(112,326,242,36,17,border);
 canvas().fillRoundRect(112,369,242,36,17,rgb(2,20,35));
 canvas().drawRoundRect(112,369,242,36,17,border);
 canvas().setTextColor(rgb(143,235,249));canvas().drawString("VEL",135,344);
 canvas().drawString("ACC",135,387);
 canvas().setTextColor(rgb(247,251,255));
 if(speed>10000000.)snprintf(msg,sizeof(msg),"%.4f c",speed/astro::C);
 else if(speed>1000.)snprintf(msg,sizeof(msg),"%.3f km/s",speed/1000.);
 else snprintf(msg,sizeof(msg),"%.0f m/s",speed);
 canvas().drawString(msg,269,344);
 if(fabs(acc)>=1000.)snprintf(msg,sizeof(msg),"%+.2f km/s2",acc/1000.);
 else snprintf(msg,sizeof(msg),"%+.2f m/s2",acc);
 canvas().drawString(msg,269,387);
}
static void drawFadingTitle(){
 if(!hudVisible||!flightTitleStart)return;
 uint32_t elapsed=millis()-flightTitleStart;
 if(elapsed>=TITLE_FADE_MS)return;
 float f=1.f-float(elapsed)/TITLE_FADE_MS;
 uint16_t c=rgb((uint8_t)(160.f*f),(uint8_t)(225.f*f),(uint8_t)(252.f*f));
 canvas().setFont(&fonts::FreeMonoBold12pt7b);canvas().setTextDatum(middle_center);
 canvas().setTextColor(c);canvas().drawString("voygerM5",233,67);
}
// M5GFX 0.3.0: S3 PSRAM sprite transfers may remain in flight after pushSprite.
// Synchronize on the DESTINATION before touching the source sprite, old-frame
// snapshot or clipping region. This is also harmless when pushSprite is synchronous.
static inline void fenceDisplayDMA() { M5.Display.waitDMA(); }
// Compare 16-row bands against the previously displayed frame, then transfer
// only changed bands when the changed area is small. In a moving scene fall
// back to one normal full push. Uses only LGFX public clipping API.
static uint8_t* previousFrame=nullptr;
static bool previousFrameValid=false;
static void presentSprite(){
  const int ox=(physicalW-S)/2,oy=(physicalH-S)/2;
  const uint8_t* frame=static_cast<const uint8_t*>(screen.getBuffer());
  if(!frame || !previousFrame){screen.pushSprite(ox,oy);fenceDisplayDMA();return;}
  constexpr int bandHeight=16;
  constexpr int bands=(S+bandHeight-1)/bandHeight;
  bool changed[bands];
  int dirtyRows=0,groups=0;bool wasDirty=false;
  for(int i=0;i<bands;i++){
    int y=i*bandHeight;
    int rows=std::min(bandHeight,S-y);
    changed[i]=!previousFrameValid ||
      std::memcmp(frame+size_t(y)*S*2,previousFrame+size_t(y)*S*2,size_t(rows)*S*2)!=0;
    if(changed[i])dirtyRows+=rows;
    if(changed[i]&&!wasDirty)groups++;
    wasDirty=changed[i];
  }
  if(dirtyRows==0)return;
  // Clipping helps when large portions of the sky are unchanged. Avoid many
  // independent SPI transactions or compares when most of the view changed.
  if(dirtyRows>int(S*.60f) || groups>5){
     screen.pushSprite(ox,oy);
     fenceDisplayDMA();
  }else{
    for(int i=0;i<bands;){
      if(!changed[i]){i++;continue;}
      int first=i;
      while(i<bands && changed[i])i++;
      int y0=first*bandHeight, y1=std::min(S,i*bandHeight);
      M5.Display.setClipRect(ox,oy+y0,S,y1-y0);
      screen.pushSprite(ox,oy);
      fenceDisplayDMA(); // keep clip valid until transfer has completed
    }
    M5.Display.clearClipRect();
  }
  fenceDisplayDMA(); // source buffer cannot be repainted until DMA is idle
  std::memcpy(previousFrame,frame,size_t(S)*S*2);
  previousFrameValid=true;
}
static void render(){
 if(haveSprite)fenceDisplayDMA(); // no in-flight read of screen PSRAM
 const uint32_t t0=micros();
 if(!haveSprite) M5.Display.startWrite();
 canvas().fillScreen(0x0000);auto cam=view();drawStars(cam);
 // Warp tunnel has priority over slow NASA sphere rasterization.
 // While in transit the objects are not shown; they reappear at arrival.
 if(!warpActive()){drawBodies(cam);drawProbes(cam);}
 if(!warpMenu){canvas().drawCircle(CTR,CTR,16,0x77df);canvas().drawLine(CTR-24,CTR,CTR-17,CTR,0x77df);
 canvas().drawLine(CTR+17,CTR,CTR+24,CTR,0x77df);canvas().drawLine(CTR,CTR-24,CTR,CTR-17,0x77df);
 canvas().drawLine(CTR,CTR+17,CTR,CTR+24,0x77df);drawRing();}
 if(warpMenu){
  canvas().fillRoundRect(42,111,382,266,22,rgb(7,23,43));
  canvas().drawRoundRect(42,111,382,266,22,rgb(83,192,238));
  canvas().setTextDatum(middle_center);
  canvas().setFont(&fonts::FreeMonoBold12pt7b);
  canvas().setTextColor(rgb(124,223,252));
  canvas().drawString("WARP DESTINATION",CTR,142);
  canvas().setTextColor(rgb(241,250,255));
  canvas().drawString(navName(navIndex),CTR,210);
  char navmsg[48];
  if(navIndex){
    double dist=astro::norm(targetPosition(navIndex)-sim.ship);
    if(dist>=astro::AU*.01)snprintf(navmsg,sizeof(navmsg),"DIST %.2f AU",dist/astro::AU);
    else snprintf(navmsg,sizeof(navmsg),"DIST %.0f km",dist/1000.);
  }else snprintf(navmsg,sizeof(navmsg),"JUMP 0.07 AU FORWARD");
  canvas().setTextColor(rgb(178,206,233));canvas().drawString(navmsg,CTR,250);
  canvas().drawString("SWIPE  <   >",CTR,294);
  canvas().setTextColor(rgb(171,224,220));canvas().drawString("L: CLOSE   R: WARP",CTR,334);
  snprintf(navmsg,sizeof(navmsg),"%02d / %02d",navIndex+1,NAV_COUNT);
  canvas().drawString(navmsg,CTR,356);
 } else {
   drawMinimalHUD();
   drawFadingTitle();
   if((int32_t)(neutralCueUntil-millis())>0 && !warpActive()){
     canvas().setTextDatum(middle_center);
     canvas().setFont(&fonts::FreeMonoBold12pt7b);
     canvas().setTextColor(rgb(229,239,251));
     canvas().drawString("THROTTLE NEUTRAL",CTR,294);
   }
 }
 drawWarpFX();
 drawArrivalGlow();
 if(!warpMenu && !warpActive() && !sim.crashed &&
    astro::norm(sim.ship-sim.alienWorld.p)<100000000.){
   // Secret destination: no warp-menu entry, no navigational guidance.
   canvas().fillRoundRect(37,72,392,61,14,rgb(3,29,33));
   canvas().drawRoundRect(37,72,392,61,14,rgb(70,229,196));
   canvas().setTextDatum(middle_center);
   canvas().setFont(&fonts::FreeMonoBold12pt7b);
   canvas().setTextColor(rgb(193,255,220));
   canvas().drawString("WRONG ADDRESS!",CTR,102);
 }
 if(sim.crashed){
   canvas().fillCircle(233,233,157,0xc002);
   canvas().setTextDatum(middle_center);
   canvas().setFont(&fonts::FreeMonoBold12pt7b);
   canvas().setTextColor(0xffff);
   canvas().drawString("ATMOSPHERIC",233,205);
   canvas().drawString("ENTRY / IMPACT",233,237);
   // Only Earth impacts reported with c-speed HUD get the relativistic TNT figure.
   if(sim.crashBody==3 && sim.crashRelativeSpeed>10000000.){
     const double beta=std::min(0.999999999,sim.crashRelativeSpeed/astro::C);
     const double energyJ=(1./sqrt(1.-beta*beta)-1.)*SHIP_MASS_KG*astro::C*astro::C;
     const double teratons=energyJ/4.184e21;
     char energyText[40];
     if(teratons>=1.)snprintf(energyText,sizeof(energyText),"%.2f Tt TNT",teratons);
     else if(teratons>=.001)snprintf(energyText,sizeof(energyText),"%.2f Gt TNT",teratons*1000.);
     else snprintf(energyText,sizeof(energyText),"%.2f Mt TNT",teratons*1.e6);
     canvas().drawString(energyText,233,275);
   }
 }
 const uint32_t tPaint=micros();
 if(haveSprite) {
   presentSprite();
 } else {
   M5.Display.endWrite();
 }
 // Advance visual time only after an actual presented frame.
 // Heavy sphere/DMA frames can no longer silently consume the whole effect.
 if(warpActive())warpFrameClock.onPresentedFrame(millis());
 static uint32_t renders=0,accPaint=0,accPresent=0;
 accPaint+=tPaint-t0;
 accPresent+=micros()-tPaint;
 if(++renders==120){
   Serial.printf("R15 FRAME painter=%luus present=%luus (mean/120)\n",
     (unsigned long)(accPaint/renders),(unsigned long)(accPresent/renders));
   renders=0;accPaint=0;accPresent=0;
 }
}
static void controls(double dt){
 (void)dt;
 sampleButtons();
 const bool rightIsDown=rightHeld();
 if(!rightIsDown)rightBlockedUntilRelease=false;
 const uint32_t clickNow=millis();
 const bool doubleFromSampler=hardware_buttons::takeLeftDoublePress();
 const bool leftClick=leftPressed();
 if(leftTapPending && uint32_t(clickNow-lastLeftTapMs)>450)leftTapPending=false;
 if((doubleFromSampler ||
     (leftClick && leftTapPending && uint32_t(clickNow-lastLeftTapMs)<=450)) &&
    !warpActive() && !sim.crashed){
    leftTapPending=false;
    warpMenu=false;sim.thrust=false;sim.engine.clear();touchLast=false;
    beginWarp(true); // 1.125 light years, straight ahead, no correction.
 } else if(leftClick && !warpActive() && !sim.crashed){
    leftTapPending=true;lastLeftTapMs=clickNow;
    warpMenu=!warpMenu;sim.thrust=false;sim.engine.clear();
    touchLast=false; // A single left click retains the original warp menu.
 }
 const bool rightTap=rightPressed();
 if(rightTap && warpMenu && !warpActive() && !sim.crashed){
    warpMenu=false;rightBlockedUntilRelease=true;beginWarp();
 } else if(rightTap && !warpMenu && !warpActive() && !sim.crashed){
    // At throttle=0 the main engine intentionally has zero acceleration.
    // A brief on-screen cue makes the physical key press discoverable.
    if(fabs(sim.throttle)<0.001)neutralCueUntil=millis()+1400;
    Serial.printf("STAR_VOYAGER_R15 RIGHT_PRESS throttle=%.3f\n",sim.throttle);
 }
 sim.thrust=rightIsDown&&!rightBlockedUntilRelease&&!warpMenu&&
            !warpActive()&&!sim.crashed&&sim.throttle!=0.;
 auto t=M5.Touch.getDetail();const bool down=t.isPressed();
 const uint32_t now=millis();
 if(down){
   const float x=float(t.x)-CTR, y=float(t.y)-CTR;
   if(!touchLast){
     ringTouch=!warpMenu && ring_geometry::onThrottleBand(x,y);
     touchStartX=lastX=menuGestureX=x;
     touchStartY=lastY=menuGestureY=y;
     gestureMoved=false;longDone=false;touchBeginMs=now;
     if(ringTouch)sim.throttle=ring_geometry::fromRingXY(x,y);
   } else {
     const float moved=hypotf(x-touchStartX,y-touchStartY);
     if(moved>14.f)gestureMoved=true;
     if(warpMenu){
       // Any central swipe works; every ~48px moves exactly one slot.
       const float dx=x-menuGestureX,dy=y-menuGestureY;
       const float dominant=(fabsf(dx)>=fabsf(dy))?dx:-dy;
       if(fabsf(dominant)>=48.f){
         selectTarget(navIndex+(dominant>0?1:-1));
         menuGestureX=x;menuGestureY=y;
       }
     }else if(ringTouch){
       if(hypotf(x,y)>75.f)sim.throttle=ring_geometry::fromRingXY(x,y);
     }else if(!longDone){
       // No rotation while deciding whether this is a long press.
       if(gestureMoved){
         arrivalFocusUntil=0; // manual RCS aiming always wins over auto-follow
         sim.yaw+=(x-lastX)*.0045;
         sim.pitch=std::max(-1.45,std::min(1.45,sim.pitch-(y-lastY)*.0045));
       }
     }
     lastX=x;lastY=y;
   }
   if(!warpMenu && !ringTouch && !gestureMoved && !longDone &&
      now-touchBeginMs>=750){hudVisible=!hudVisible;longDone=true;}
 }
 touchLast=down;
}
void setup(){
 // Dedicated StopWatch board; initialize via M5Unified exactly once.
 // R13 FIX3 independent startup order, with the project's pinned M5 0.3.0.
 auto cfg=M5.config();
 cfg.serial_baudrate=115200;
 cfg.clear_display=true;
 cfg.internal_imu=true;
 cfg.internal_rtc=true;
 cfg.internal_spk=true;
 cfg.internal_mic=true;
 cfg.external_imu=false;
 cfg.external_rtc=false;
 cfg.fallback_board=m5::board_t::board_M5StopWatch;
 M5.begin(cfg);
 M5.Display.setBrightness(180);
 M5.Display.setRotation(0);
 physicalW=M5.Display.width();
 physicalH=M5.Display.height();
 displayReady=(M5.getBoard()==m5::board_t::board_M5StopWatch &&
               physicalW>=S && physicalH>=S);
 Serial.printf("VOYGERM5_NATIVE_BOOT board=%d display=%dx%d psram=%u flash=%u\n",
               int(M5.getBoard()),physicalW,physicalH,
               (unsigned)ESP.getPsramSize(),(unsigned)ESP.getFlashChipSize());
 if(!displayReady){
   Serial.println("VOYGERM5_NATIVE_BOOT STOP: StopWatch display initialization failed");
   return; // No drawing to an invalid display.
 }
 bootSplash(); // First visible frame, before expensive graphics allocation.
 // GPIO2=A/left, GPIO1=B/right, active LOW. M5.begin owns GPIO configuration.
 leftKey.begin(M5.BtnA.isPressed(),millis());
 leftPressGate.reset();rightPressGate.reset();
 rightKey.begin(M5.BtnB.isPressed(),millis());
 const bool samplerOK=hardware_buttons::begin();
 Serial.printf("VOYGERM5_NATIVE_BOOT BUTTONS sampler=%s\n",samplerOK?"ON":"M5 FALLBACK");
 Serial.printf("STAR_VOYAGER_R9 PSRAM bytes=%u\n",(unsigned)ESP.getPsramSize());
 // Only allocate the large PSRAM sprite AFTER a visible boot splash.
 screen.setColorDepth(16);
 screen.setPsram(true);
 haveSprite=(screen.createSprite(S,S)!=nullptr);
 if(haveSprite){
   detectSpritePixelFormat();
   previousFrame=static_cast<uint8_t*>(heap_caps_malloc(size_t(S)*S*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
   previousFrameValid=false;
 }
 prepareRingGeometry();
 prepareAtmosphereCircle();
 drawTarget=haveSprite ? static_cast<lgfx::LGFXBase*>(&screen)
                       : static_cast<lgfx::LGFXBase*>(&M5.Display);
 if(!haveSprite){
   Serial.println("VOYGERM5_NATIVE_BOOT PSRAM SPRITE FAILED: direct display fallback");
 }else{
   Serial.println("VOYGERM5_NATIVE_BOOT PSRAM SPRITE OK");
 }
 buildSphereLut();
 prepareStars();
 sim.reset();warpMenu=false;hudVisible=true;
 M5.Power.setVibration(0);lastMotorPower=0;
 bootSplashUntil=millis()+400;
 prevUs=micros();lastDraw=millis();
 Serial.println("STAR_VOYAGER_R13 TUTORIAL READY");
}
void loop(){
 if(!displayReady){delay(500);return;}
 sampleButtons(true);
 if((int32_t)(millis()-bootSplashUntil)<0){delay(1);return;}
 if(tutorialOpen){
   // No physics or engine while navigating the introductory instructions.
   if(!tutorialPainted)drawTutorialPage();
   auto touch=M5.Touch.getDetail();
   bool down=touch.isPressed();
   if(down){tutorialLastX=touch.x;tutorialLastY=touch.y;}
   if(down&&!tutorialTouchPrev){tutorialStartX=touch.x;tutorialStartY=touch.y;}
   if(!down&&tutorialTouchPrev){
     float dx=tutorialLastX-tutorialStartX;
     // The START area occupies the lower part of the approved graphic.
     if(tutorialStartY>348){startFlight();}
     else if(fabsf(dx)>38.f){
       tutorialPage=(tutorialPage+(dx<0?1:3))%4;tutorialPainted=false;
     }
   }
   tutorialTouchPrev=down;
   if(leftPressed()){tutorialPage=(tutorialPage+1)%4;tutorialPainted=false;}
   if(rightPressed()){startFlight();}
   delay(10);return;
 }
 uint32_t nowUs=micros(),now=millis();double dt=std::min(.08,((uint32_t)(nowUs-prevUs))/1000000.);prevUs=nowUs;
 controls(dt);
 // Engine spool and 1.10s release tail use wall-clock time, independent of warp.
 // The physics integrator consumes engine.acceleration() even after release.
 sim.engine.advance((sim.thrust&&!warpMenu&&!warpActive()&&!sim.crashed)?sim.throttle:0.,dt);
 if(sim.crashed){if(impactMs==0){impactMs=now;warpMs=0;warpApplied=false;longRangeWarpActive=false;warpFrameClock.start();}
   if(now-impactMs>1400){sim.reset();impactMs=0;warpMenu=false;arrivalGlowMs=0;arrivalFocusUntil=0;rightBlockedUntilRelease=true;}}
 else {
   tickWarp(now);
   if(!warpActive()&&!warpMenu)sim.step(dt*timeScale);
   if(!warpActive()&&!warpMenu)maintainArrivalFocus(now);
   impactMs=0;
 }
 tickHaptics(now);
 // Direct display is a slow but functional fallback; cap to ~8 FPS.
 const uint32_t drawInterval=haveSprite?35:125;
 if(now-lastDraw>=drawInterval){lastDraw=now;render();}
 delay(1);
}
