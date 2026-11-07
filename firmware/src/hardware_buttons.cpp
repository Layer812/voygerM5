#include <Arduino.h>
#include <atomic>
#include "hardware_buttons.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
namespace hardware_buttons {
namespace {
struct DebouncedKey {
  std::atomic<bool> held{false};
  std::atomic<unsigned> presses{0};
  bool lastRaw=false, stable=false;
  uint8_t repeated=0;
  void init(bool down) {
    lastRaw=stable=down;
    repeated=3;
    held.store(down, std::memory_order_relaxed);
    presses.store(0, std::memory_order_relaxed);
  }
  void sample(bool down) {
    if(down!=lastRaw){lastRaw=down;repeated=1;}
    else if(repeated<3) ++repeated;
    if(repeated>=3 && down!=stable) {
      stable=down;
      held.store(down, std::memory_order_release);
      if(down) presses.fetch_add(1, std::memory_order_release);
    }
  }
  bool takePress(){return presses.exchange(0, std::memory_order_acq_rel)!=0;}
};
DebouncedKey keyA,keyB;
std::atomic<bool> running{false};
TaskHandle_t handle=nullptr;
void sampleTask(void*) {
  for(;;) {
    keyA.sample(digitalRead(2)==LOW);
    keyB.sample(digitalRead(1)==LOW);
    vTaskDelay(pdMS_TO_TICKS(5) > 0 ? pdMS_TO_TICKS(5) : 1);
  }
}
}
bool begin() {
  // M5.begin(cfg) already configured the two StopWatch keys as plain GPIO inputs.
  keyA.init(digitalRead(2)==LOW);
  keyB.init(digitalRead(1)==LOW);
  BaseType_t ok=xTaskCreatePinnedToCore(sampleTask,"sv_keys",2048,nullptr,2,&handle,0);
  running.store(ok==pdPASS,std::memory_order_release);
  return ok==pdPASS;
}
bool active(){return running.load(std::memory_order_acquire);}
bool leftDown(){return keyA.held.load(std::memory_order_acquire);}
bool rightDown(){return keyB.held.load(std::memory_order_acquire);}
bool takeLeftPress(){return keyA.takePress();}
bool takeRightPress(){return keyB.takePress();}
}
