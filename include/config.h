#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ========== LED 配置 ==========
// WS2812 数据线接在 IO1 (GPIO1)
#define LED_PIN 1            // IO1 (GPIO1) - 空闲GPIO
#define EMERGENCY_PIN 4      // 急停开关引脚 (GPIO4 = 数字输入1)
#define NUMPIXELS 100        // LED 数量

// ========== CAN 配置（已禁用） ==========
// #define CAN_TX_PIN   2      // CAN 发送引脚 (GPIO2)
// #define CAN_RX_PIN   3      // CAN 接收引脚 (GPIO3)
// #define CAN_BAUDRATE 500000  // CAN 波特率 (500kbps)

// #define CAN_LOOPBACK        // 回环模式（已禁用）

#endif // CONFIG_H

