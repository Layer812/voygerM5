#pragma once
#include <cstdint>
#include <cstddef>
namespace fonts {static const int FreeMonoBold12pt7b=0;}
static const int middle_center=0;
namespace lgfx {
class LGFXBase {
public:
  void fillScreen(uint16_t) {}
  bool drawJpg(const uint8_t*,size_t,int32_t,int32_t){return true;}
  void setFont(const int*) {}
  void setTextDatum(int) {}
  void setTextColor(uint16_t) {}
  void drawString(const char*,int,int) {}
  void setRotation(int) {}
  int width(){return 466;}
  int height(){return 466;}
  void setBrightness(int) {}
  void setClipRect(int,int,int,int) {}
  void clearClipRect() {}
  void waitDMA() {}
  void startWrite() {}
  void endWrite() {}
  void drawPixel(int,int,uint16_t) {}
  void fillCircle(int,int,int,uint16_t) {}
  void drawRect(int,int,int,int,uint16_t) {}
  void drawEllipse(int,int,int,int,uint16_t) {}
  void drawLine(int,int,int,int,uint16_t) {}
  void drawCircle(int,int,int,uint16_t) {}
  void fillRoundRect(int,int,int,int,int,uint16_t) {}
  void drawRoundRect(int,int,int,int,int,uint16_t) {}
};
} // lgfx
class M5Canvas: public lgfx::LGFXBase {
public:
 explicit M5Canvas(lgfx::LGFXBase*) {}
 void setColorDepth(int) {}
 void setPsram(bool) {}
 void* createSprite(int,int){return nullptr;}
 void pushSprite(int,int) {}
 void* getBuffer(){return nullptr;}
};
