#pragma once
#define PI 3.1415926535897932384626433832795
#include "M5GFX.h"
#include <cstdint>
struct MockButton { bool isPressed() {return false;} bool wasPressed() {return false;} };
struct MockTouchDetail {int x=0,y=0;bool isPressed(){return false;}};
struct MockTouch {MockTouchDetail getDetail(){return {};}};
struct MockPower {void setVibration(uint8_t) {}};
namespace m5 { enum class board_t { board_M5StopWatch }; }
struct MockM5Config {
  m5::board_t fallback_board = m5::board_t::board_M5StopWatch;
  bool internal_mic=true, internal_imu=true, internal_rtc=true;
  bool external_imu=true, external_rtc=true, internal_spk=true;
};
struct MockM5 {
  lgfx::LGFXBase Display;
  MockPower Power;
  MockButton BtnA,BtnB;
  MockTouch Touch;
  MockM5Config config() {return {};}
  void begin(MockM5Config) {}
  void update() {}
  int getBoard() {return 0;}
};
extern MockM5 M5;
struct MockSerial {void begin(int){};template<typename... T> void printf(const char*,T...){};void println(const char*){}};
extern MockSerial Serial;
struct MockESP {unsigned getPsramSize(){return 8*1024*1024;}};
extern MockESP ESP;
uint32_t millis();uint32_t micros();void delay(int);

#define INPUT_PULLUP 2
#define LOW 0
inline void pinMode(int,int) {}
inline int digitalRead(int) {return 1;}
