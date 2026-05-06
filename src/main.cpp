#include <Arduino.h>
#include "config.h"
#include "system_state.h"
#include "led_controller.h"
#include "command_handler.h"
#include "emergency.h"

void setup() {
  Serial.begin(9600);
  initEmergency();
  initLEDs();
  delay(500);

  Serial.println(F("========================================"));
  Serial.println(F("    串口控制WS2812灯带系统"));
  Serial.println(F("========================================"));
  Serial.println(F("命令: 0=关闭 1=彩色走马灯 2=红 3=绿"));
  Serial.println(F("      4=蓝 5=蓝色走马灯 6=白 7=测试"));
  Serial.println(F("      8=呼吸 9=彩虹 help=帮助"));
  Serial.println(F("----------------------------------------"));
  Serial.println(F("硬件: D2=急停 D7=LED 数量=8"));
  Serial.println(F("波特率: 9600"));
  Serial.println(F("----------------------------------------"));
  Serial.println(F("等待命令..."));
  Serial.println(F("========================================"));

  showCurrentState();
}

void loop() {
  uint32_t currentTime = millis();
  static uint32_t lastLEDUpdate = 0;
  static uint32_t lastStatusUpdate = 0;

  if (emergencyActive) {
    if (emergencyStartTime == 0) {
      emergencyStartTime = currentTime;
    }

    setAllLEDs(255, 0, 0);

    if (currentTime - lastStatusUpdate > 1000) {
      lastStatusUpdate = currentTime;
      showEmergencyStatus(currentTime);
    }

    return;
  }

  emergencyStartTime = 0;
  handleSerialCommands();
  updateLEDDisplay(currentTime, &lastLEDUpdate);

  if (currentTime - lastStatusUpdate > 1000) {
    lastStatusUpdate = currentTime;
    showCurrentState();
  }

  delay(1);
}
