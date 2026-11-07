#pragma once
namespace hardware_buttons {
// Samples both active-low StopWatch buttons every 5ms, with 15ms debounce.
// Input polling task ONLY reads GPIO. M5.update() stays on the loop core.
bool begin();
bool active();
bool leftDown();
bool rightDown();
bool takeLeftPress();
bool takeRightPress();
}
