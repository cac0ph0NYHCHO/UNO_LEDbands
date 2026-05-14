#include <Arduino.h>
#include "config.h"
#include "system_state.h"
#include "led_controller.h"
#include "command_handler.h"
#include "emergency.h"
#include "button.h"
#include "WS_Dout.h"
#include "WS_TCA9554PWR.h"
#include "I2C_Driver.h"
#include "WS_WIFI.h"
#include "WS_GPIO.h"
#include "can_handler.h"
#include "can_relay.h"

/* ===========================================================
 * CAN 命令回调函数
 * =========================================================== */
static void onCanCommand(const CanFrame* frame) {
  if (!frame) return;

  Serial.print(F("[CAN] 收到: ID=0x"));
  Serial.print(frame->id, HEX);
  Serial.print(F(" 数据="));
  for (uint8_t i = 0; i < frame->len; i++) {
    Serial.print(frame->data[i], HEX);
    Serial.print(F(" "));
  }
  Serial.println();

  if (frame->id == 0x100 && frame->len >= 1) {
    char cmdStr[4];
    snprintf(cmdStr, sizeof(cmdStr), "%d", frame->data[0]);
    processCommand(cmdStr);
  }
  else if (frame->id == 0x101 && frame->len >= 3) {
    setAllLEDs(frame->data[0], frame->data[1], frame->data[2]);
    currentState = STATE_IDLE;
    Serial.print(F("[CAN] 直接设色: RGB("));
    Serial.print(frame->data[0]);
    Serial.print(F(","));
    Serial.print(frame->data[1]);
    Serial.print(F(","));
    Serial.println(frame->data[2]);
  }
  else if (frame->id == 0x200) {
    relayProcessResponse(frame->id, frame->data, frame->len);
  }
}

void setup() {
  Serial.begin(115200);
  initEmergency();
  initLEDs();
  initButton();

  GPIO_Init();
  I2C_Init();
  Dout_Init();
  WIFI_Init();

  if (initCAN(CAN_TX_PIN, CAN_RX_PIN, CAN_BAUDRATE)) {
    registerCanCallback(onCanCommand);
    Serial.println(F("[CAN] 回调注册完成"));
  }

  delay(500);

#ifdef LATENCY_TEST_ENABLE
  Serial.println(F("========================================"));
  Serial.println(F("    延迟测试模式（中断+精简loop）"));
  Serial.println(F("    按键下降沿中断触发，无delay(1)"));
  Serial.println(F("========================================"));
#else
  Serial.println(F("========================================"));
  Serial.println(F("    串口控制WS2812灯带系统"));
  Serial.println(F("========================================"));
  Serial.println(F("命令: 0=关闭 1=彩色走马灯 2=红 3=绿"));
  Serial.println(F("      4=蓝 5=蓝色走马灯 6=白 7=测试"));
  Serial.println(F("      8=呼吸 9=彩虹 help=帮助"));
  Serial.println(F("----------------------------------------"));
  Serial.println(F("继电器: a~h=开 A~H=关 o=全开 p=全关 s=状态"));
  Serial.println(F("----------------------------------------"));
  Serial.print(F("硬件: D4=急停 IO1=LED 按键=GPIO"));
  Serial.print(BUTTON_PIN);
  Serial.print(F(" 数量="));
  Serial.println(NUMPIXELS);
  Serial.println(F("按键: 短按=EXIO1输出24V 长按=WiFi信号(预留)"));
  Serial.println(F("----------------------------------------"));
  Serial.println(F("等待命令..."));
  Serial.println(F("========================================"));
#endif

  showCurrentState();
}

void sendCanTestFrame() {
  uint8_t data[1] = {2};
  if (sendCanMessage(0x100, data, 1)) {
    Serial.println(F("[CAN测试] 已发送 ID=0x100  data=[2]"));
  } else {
    Serial.println(F("[CAN测试] 发送失败!"));
  }
}

// =================================================================
// 延迟测试模式：精简版 loop
// =================================================================
#ifdef LATENCY_TEST_ENABLE

void loop() {
  processCANReceive();

  updateButton();
  ButtonEvent btnEvent = getButtonEvent();

  if (btnEvent == BTN_SHORT_PRESS) {
    static bool exioState = false;
    exioState = !exioState;

    // g_btnFallTime 在中断 ISR 中已记录，精确到按键按下瞬间
    uint32_t tBefore = micros();

    if (exioState) {
      Dout_Open(1);
      uint32_t tAfter = micros();
      Serial.print(F("[延迟测试] 短按->EXIO1导通: loop响应="));
      Serial.print(tBefore - g_btnFallTime);
      Serial.print(F(" I2C读写="));
      Serial.print(tAfter - tBefore);
      Serial.print(F(" 总延迟="));
      Serial.println(tAfter - g_btnFallTime);
    } else {
      Dout_Closs(1);
      uint32_t tAfter = micros();
      Serial.print(F("[延迟测试] 短按->EXIO1断开: loop响应="));
      Serial.print(tBefore - g_btnFallTime);
      Serial.print(F(" I2C读写="));
      Serial.print(tAfter - tBefore);
      Serial.print(F(" 总延迟="));
      Serial.println(tAfter - g_btnFallTime);
    }
  }

  if (emergencyActive) {
    if (emergencyStartTime == 0) {
      emergencyStartTime = millis();
    }
    static uint32_t lastEmergPrint = 0;
    uint32_t now = millis();
    if (now - lastEmergPrint > 1000) {
      lastEmergPrint = now;
      showEmergencyStatus(now);
    }
    return;
  }
  emergencyStartTime = 0;

  handleSerialCommands();
  delay(0);
}

// =================================================================
// 正常模式：完整功能 loop
// =================================================================
#else

void loop() {
  uint32_t currentTime = millis();
  static uint32_t lastLEDUpdate = 0;
  static uint32_t lastStatusUpdate = 0;
  static uint32_t lastCanTestTime = 0;

  processCANReceive();

  updateButton();
  ButtonEvent btnEvent = getButtonEvent();
  if (btnEvent == BTN_SHORT_PRESS) {
    static bool exioState = false;
    exioState = !exioState;
    if (exioState) {
      Dout_Open(1);
      Serial.println(F("[主循环] EXIO1 导通"));
    } else {
      Dout_Closs(1);
      Serial.println(F("[主循环] EXIO1 断开"));
    }
  } else if (btnEvent == BTN_LONG_PRESS) {
    Serial.println(F("[主循环] 长按事件 - 预留: WiFi 发送信号"));
  }

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

  #ifdef CAN_LOOPBACK
  if (currentTime - lastCanTestTime > 10000) {
    lastCanTestTime = currentTime;
    sendCanTestFrame();
  }
  #endif

  delay(1);
}

#endif
