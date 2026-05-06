#include "emergency.h"
#include "system_state.h"

void initEmergency() {
  pinMode(EMERGENCY_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(EMERGENCY_PIN), handleEmergencyInterrupt, CHANGE);
}

void handleEmergencyInterrupt() {
  bool pressed = (digitalRead(EMERGENCY_PIN) == LOW);
  emergencyActive = pressed;
}

void showEmergencyStatus(uint32_t currentTime) {
  uint32_t duration = (currentTime - emergencyStartTime) / 1000;
  Serial.print(F("[急停] 持续时间: "));
  Serial.print(duration);
  Serial.println(F("秒"));
}
