#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ========== LED 配置 ==========
#define LED_PIN 7            // WS2812 数据引脚
#define EMERGENCY_PIN 4      // 急停开关引脚
#define NUMPIXELS 100        // LED 数量

// ========== CAN 配置 ==========
#define CAN_TX_PIN   2      // CAN 发送引脚 (根据板子实际接线修改)
#define CAN_RX_PIN   3      // CAN 接收引脚
#define CAN_BAUDRATE 500000  // CAN 波特率 (500kbps)

#endif // CONFIG_H

