#pragma once


#include <Arduino.h>
#include "config.h"

enum SystemState : uint8_t {
  STATE_IDLE,
  STATE_UNINITIALIZED,
  STATE_CHARGING,
  STATE_CHARGE_COMPLETE,
  STATE_WALKING,
  STATE_MOVING,
  STATE_STATIONARY,
  STATE_TEST,
  STATE_BREATHING,
  STATE_RAINBOW
};

extern SystemState currentState;
extern bool emergencyActive;
extern bool ledInitialized;
extern uint32_t emergencyStartTime;
extern uint32_t testStepTime;
extern uint8_t testColorIndex;
extern uint32_t breathingStartTime;
extern uint32_t breathingLastUpdate;
extern uint32_t rainbowStartTime;
extern uint32_t rainbowLastUpdate;
extern uint16_t rainbowHue;


