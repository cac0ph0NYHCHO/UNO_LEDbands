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

#endif // BUTTON_H

