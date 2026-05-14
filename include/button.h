#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include "config.h"

// ========== 按键引脚 ==========
#ifndef BUTTON_PIN
#define BUTTON_PIN  5      // GPIO5（数字输入2）
#endif

// 长按判定阈值（毫秒）
// 可在 config.h 中提前定义来覆盖
#ifndef SHORT_PRESS_MS
#define SHORT_PRESS_MS  500   // 默认：短按 < 500ms ≤ 长按
#endif
// ========== 按键事件类型 ==========
typedef enum {
  BTN_NONE,         // 无事件
  BTN_SHORT_PRESS,  // 短按
  BTN_LONG_PRESS    // 长按
} ButtonEvent;

// ========== 函数声明 ==========
void initButton();
ButtonEvent getButtonEvent();
void clearButtonEvent();
void updateButton();

// ========== 延迟测试用全局变量 ==========
#ifdef LATENCY_TEST_ENABLE
extern uint32_t g_btnFallTime;   // 按键下降沿（按下）时刻，单位 us
extern volatile bool g_btnPressedFlag;  // 中断触发的按键按下标志
extern uint32_t g_btnPressTimeMs;      // ISR 中记录的按下时刻 (ms)
#endif

#endif // BUTTON_H

