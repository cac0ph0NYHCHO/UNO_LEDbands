#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ========== LED 配置 ==========
// WS2812 数据线接在 IO1 (GPIO1)
#define LED_PIN 1            // IO1 (GPIO1) - 空闲GPIO
#define EMERGENCY_PIN 4      // 急停开关引脚 (GPIO4 = 数字输入1)
#define NUMPIXELS 100        // LED 数量

// ========== 按键配置 ==========
#define SHORT_PRESS_MS  2000  // 短按阈值(ms)：小于此值为短按，大于等于为长按

// ========== CAN 配置 ==========
#define CAN_TX_PIN   2      // CAN 发送引脚 (GPIO2)
#define CAN_RX_PIN   3      // CAN 接收引脚 (GPIO3)
#define CAN_BAUDRATE 250000  // CAN 波特率 (250kbps)

// CAN 模式选择：
//   若要使用外部 CAN 继电器模块，请注释掉下面这行
//   若要离线测试，取消注释（回环模式：软件模拟，无需外接硬件）
// #define CAN_LOOPBACK        // 回环模式（启用 = 软件模拟，无需外接硬件）

// ========== CAN 继电器模块配置 ==========
#define CAN_RELAY_ENABLED     // 启用外部 CAN 继电器模块支持

// 继电器模块 CAN 协议参数
#define CAN_RELAY_STATION_ID  0x01   // 模块站号（默认 0x01）

// ========== 串口命令中继电器相关 ==========
#define RELAY_CMD_PREFIX     "relay"   // 串口命令前缀

// ========== 延迟测试配置 ==========
// 取消注释启用按键→EXIO 反应时间测试模式
// 启用后会关掉不必要的串口打印，并在关键点输出微秒级时间戳
#define LATENCY_TEST_ENABLE

#ifdef LATENCY_TEST_ENABLE
  // 延迟测试模式下额外优化：
  //   - 按键改用 GPIO 中断，消除 loop 轮询延迟
  //   - 关掉每秒状态打印
  //   - 关掉 LED 动画更新（仅保持空闲）
  //   - 去掉 delay(1)
  #define LATENCY_USE_INTERRUPT      // 使用 GPIO 中断检测按键
#endif

#endif // CONFIG_H


