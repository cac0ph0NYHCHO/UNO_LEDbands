#include "button.h"

// ========== 按键状态变量 ==========
static bool     btnLastState = HIGH;       // 上一次读取的电平
static bool     btnCurrentState = HIGH;    // 当前去抖后的电平
static uint32_t lastDebounceTime = 0;      // 最后去抖时刻
static bool     btnPressed = false;        // 是否正在按下
static uint32_t pressStartTime = 0;        // 按下时刻
static ButtonEvent pendingEvent = BTN_NONE; // 待处理事件

// ========== 去抖参数 ==========
#define DEBOUNCE_DELAY  50   // 去抖延时 50ms

/* ===========================================================
 * 初始化按键
 * =========================================================== */
void initButton() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // 内部上拉，按下为 LOW
  btnLastState = digitalRead(BUTTON_PIN);
  btnCurrentState = btnLastState;
  Serial.print(F("[按键] 已初始化, 引脚=GPIO"));
  Serial.println(BUTTON_PIN);
}

/* ===========================================================
 * 更新按键状态（需要在 loop 中周期性调用）
 * =========================================================== */
void updateButton() {
  bool reading = digitalRead(BUTTON_PIN);
  static bool lastPrint = HIGH;

  // 只要电平变化就打印（不去抖，纯调试）
  if (reading != lastPrint) {
    lastPrint = reading;
    Serial.print(F("[按键] GPIO"));
    Serial.print(BUTTON_PIN);
    Serial.println(reading ? F(" HIGH") : F(" LOW"));

    if (reading == LOW) {
      btnPressed = true;
      pressStartTime = millis();
    } else {
      if (btnPressed) {
        uint32_t duration = millis() - pressStartTime;
        btnPressed = false;
        Serial.print(F("[按键] 时长="));
        Serial.print(duration);
        Serial.println(F("ms"));
        if (duration < SHORT_PRESS_MS) {
          pendingEvent = BTN_SHORT_PRESS;
          Serial.println(F("[按键] 短按"));
        } else {
          pendingEvent = BTN_LONG_PRESS;
          Serial.println(F("[按键] 长按"));
        }
      }
    }
  }
}

/* ===========================================================
 * 获取按键事件（获取后自动清除）
 * =========================================================== */
ButtonEvent getButtonEvent() {
  ButtonEvent event = pendingEvent;
  if (pendingEvent != BTN_NONE) {
    pendingEvent = BTN_NONE;
  }
  return event;
}

/* ===========================================================
 * 手动清除按键事件
 * =========================================================== */
void clearButtonEvent() {
  pendingEvent = BTN_NONE;
}

