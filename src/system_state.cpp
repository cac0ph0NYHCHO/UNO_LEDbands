#include "system_state.h"

SystemState currentState = STATE_IDLE;
bool emergencyActive = false;
bool ledInitialized = false;
uint32_t emergencyStartTime = 0;
uint32_t testStepTime = 0;
uint8_t testColorIndex = 0;
uint32_t breathingStartTime = 0;
uint32_t breathingLastUpdate = 0;
uint32_t rainbowStartTime = 0;
uint32_t rainbowLastUpdate = 0;
uint16_t rainbowHue = 0;
