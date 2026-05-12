#include <Arduino.h>
#include "config.h"
#include "system_state.h"
#include "led_controller.h"
#include "command_handler.h"
#include "emergency.h"
#include "button.h"
#include "WS_Dout.h"        // EXIO 数字输出控制
#include "WS_TCA9554PWR.h"  // I2C IO 扩展器
#include "I2C_Driver.h"     // I2C 驱动
#include "WS_WIFI.h"        // WiFi STA 连接路由器 + 网页控制DOUT
#include "WS_GPIO.h"        // RGB + 蜂鸣器（WiFi连接状态指示）
#include "can_handler.h"    // CAN 模块
#include "can_relay.h"      // CAN 继电器模块

/* ===========================================================
 * CAN 命令回调函数
 *
 * 本板作为 CAN 总线上的设备，收到以下 ID 的帧时执行相应操作：
 *   0x100 — LED 灯带命令（兼容旧协议）
 *   0x101 — LED 直接设色
 *   0x200 — 外部继电器模块的回复/查询响应
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

  // -------- LED 控制命令 (ID=0x100) --------
  if (frame->id == 0x100 && frame->len >= 1) {
    char cmdStr[4];
    snprintf(cmdStr, sizeof(cmdStr), "%d", frame->data[0]);
    processCommand(cmdStr);
  }
  // -------- LED 直接设色 (ID=0x101) --------
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
  // -------- 外部继电器模块回复 (ID=0x200) --------
  else if (frame->id == 0x200) {
    relayProcessResponse(frame->id, frame->data, frame->len);
  }
}

void setup() {
  Serial.begin(115200);
  initEmergency();
  initLEDs();
  initButton();

  // 初始化 GPIO（RGB灯和蜂鸣器）
  GPIO_Init();

  // 初始化 I2C 和 EXIO 数字输出
  I2C_Init();
  Dout_Init();

  // 初始化 WiFi STA（连接路由器 + 启动网页服务器控制DOUT）
  WIFI_Init();

  // CAN 总线
  if (initCAN(CAN_TX_PIN, CAN_RX_PIN, CAN_BAUDRATE)) {
    registerCanCallback(onCanCommand);
    Serial.println(F("[CAN] 回调注册完成"));
  }

  delay(500);

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

  showCurrentState();
}

/* ===========================================================
 * 发送 CAN 测试帧
 * =========================================================== */
void sendCanTestFrame() {
  // data[0]=2 对应命令"2"=红色常亮
  uint8_t data[1] = {2};
  if (sendCanMessage(0x100, data, 1)) {
    Serial.println(F("[CAN测试] 已发送 ID=0x100  data=[2] (红色常亮)"));
    Serial.println(F("[CAN测试] 等待回环接收..."));
  } else {
    Serial.println(F("[CAN测试] 发送失败!"));
  }
}

void loop() {
  uint32_t currentTime = millis();
  static uint32_t lastLEDUpdate = 0;
  static uint32_t lastStatusUpdate = 0;
  static uint32_t lastCanTestTime = 0;  // CAN 定时测试

  // CAN 接收处理
  processCANReceive();

  // 更新按键状态
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

  // 每秒打印一次状态
  if (currentTime - lastStatusUpdate > 1000) {
    lastStatusUpdate = currentTime;
    showCurrentState();
  }

  // 回环模式：每 10 秒自动发送一次测试帧
  #ifdef CAN_LOOPBACK
  if (currentTime - lastCanTestTime > 10000) {
    lastCanTestTime = currentTime;
    sendCanTestFrame();
  }
  #endif

  delay(1);
}