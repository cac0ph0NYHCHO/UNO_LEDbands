#include "button.h"

// ========== 按键状态变量 ==========
static bool     btnLastState = HIGH;       // 上一次读取的电平
static bool     btnCurrentState = HIGH;    // 当前去抖后的电平
static uint32_t lastDebounceTime = 0;      // 最后去抖时刻
static bool     btnPressed = false;        // 是否正在按下
static uint32_t pressStartTime = 0;        // 按下时刻 (ms)
static ButtonEvent pendingEvent = BTN_NONE; // 待处理事件

#ifdef LATENCY_TEST_ENABLE
uint32_t g_btnFallTime = 0;                // 按键按下时刻 (us)
volatile bool g_btnPressedFlag = false;    // 中断触发的按键按下标志
uint32_t g_btnPressTimeMs = 0;            // ISR 中记录的按下时刻 (ms)
#endif

// ========== 去抖参数 ==========
#define DEBOUNCE_DELAY  50   // 去抖延时 50ms

#ifdef LATENCY_USE_INTERRUPT
/* ===========================================================
 * 按键下降沿中断服务程序
 * 在按键按下瞬间（电平变 LOW）触发
 * =========================================================== */
static void IRAM_ATTR btnIsrHandler() {
  g_btnFallTime = micros();   // 立即记录时刻 (us)
  g_btnPressedFlag = true;    // 设置标志
  g_btnPressTimeMs = millis(); // 记录按下时刻 (ms)
}
#endif

/* ===========================================================
 * 初始化按键
 * =========================================================== */
void initButton() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // 内部上拉，按下为 LOW
  btnLastState = digitalRead(BUTTON_PIN);
  btnCurrentState = btnLastState;

#ifdef LATENCY_USE_INTERRUPT
  // 中断模式：下降沿触发（按下），无需轮询
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), btnIsrHandler, FALLING);
  Serial.print(F("[按键] 中断模式已初始化, 引脚=GPIO"));
#else
  Serial.print(F("[按键] 已初始化, 引脚=GPIO"));
#endif
  Serial.println(BUTTON_PIN);
}

/* ===========================================================
 * 更新按键状态（需要在 loop 中周期性调用）
 *
 * 普通模式：轮询检测电平变化
 * 中断模式：通过中断标志检测，然后轮询读取电平判断释放
 * =========================================================== */
void updateButton() {
#ifdef LATENCY_USE_INTERRUPT
  // ========== 中断模式（高效版：按下即触发） ==========
  if (g_btnPressedFlag) {
    bool level = digitalRead(BUTTON_PIN);

    if (level == LOW) {
      // 按键按下 → 立即触发短按事件（不等待释放）
      g_btnPressedFlag = false;
      pendingEvent = BTN_SHORT_PRESS;
    } else {
      // 按键已释放，清除标志
      g_btnPressedFlag = false;
    }
  }
#else
  // ========== 普通轮询模式 ==========
  bool reading = digitalRead(BUTTON_PIN);
  static bool lastPrint = HIGH;

  if (reading != lastPrint) {
    lastPrint = reading;
    if (reading == LOW) {
      btnPressed = true;
      pressStartTime = millis();
#ifdef LATENCY_TEST_ENABLE
      g_btnFallTime = micros();
#endif
    } else {
      if (btnPressed) {
        uint32_t duration = millis() - pressStartTime;
        btnPressed = false;
        if (duration < SHORT_PRESS_MS) {
          pendingEvent = BTN_SHORT_PRESS;
        } else {
          pendingEvent = BTN_LONG_PRESS;
        }
      }
    }
  }
#endif
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
