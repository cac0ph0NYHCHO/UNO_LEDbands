#include "can_handler.h"
#include "system_state.h"

/* ===========================================================
 * CAN 模块：仅在 ESP32 平台编译
 * =========================================================== */
#if defined(ESP32)

/* ===========================================================
 * ESP32-S3 TWAI (CAN) 配置
 *
 * 这块板子引脚：
 *   TX: GPIO2
 *   RX: GPIO3
 * =========================================================== */

#include <driver/twai.h>  // ESP32 TWAI 驱动库

/* ===========================================================
 * 内部变量
 * =========================================================== */

// CAN 引脚配置
static uint8_t canTxPin = 2;    // 默认 TX 引脚 (这块板子: GPIO2)
static uint8_t canRxPin = 3;    // 默认 RX 引脚 (这块板子: GPIO3)
static uint32_t canBaudRate = 500000;  // 默认 500kbps

// CAN 状态
static bool canInitialized = false;
static uint32_t canTxCount = 0;     // 发送计数
static uint32_t canRxCount = 0;     // 接收计数
static uint32_t canErrorCount = 0;  // 错误计数

// 接收回调列表（支持多个设备注册）
#define MAX_CALLBACKS 8
static CanFrameCallback callbacks[MAX_CALLBACKS];
static uint8_t callbackCount = 0;

/* ===========================================================
 * 波特率转 TWAI 时序配置
 * =========================================================== */

/**
 * @brief 根据波特率获取 TWAI 时序配置
 * 
 * ESP32-S3 主频通常为 80MHz (APB 时钟)
 * 时序计算公式: baud = 80MHz / (brp * (tseg1 + tseg2 + 1))
 * 
 * 这里提供常用配置，如需自定义可手动修改
 */
static bool getTimingConfig(uint32_t baudRate, twai_timing_config_t* tConfig) {
  switch (baudRate) {
    case 1000000:  // 1 Mbps
      tConfig->brp = 4;
      tConfig->tseg_1 = 8;
      tConfig->tseg_2 = 7;
      tConfig->sjw = 3;
      return true;

    case 500000:   // 500 kbps (默认，常用)
      tConfig->brp = 8;
      tConfig->tseg_1 = 8;
      tConfig->tseg_2 = 7;
      tConfig->sjw = 3;
      return true;

    case 250000:   // 250 kbps
      tConfig->brp = 16;
      tConfig->tseg_1 = 8;
      tConfig->tseg_2 = 7;
      tConfig->sjw = 3;
      return true;

    case 125000:   // 125 kbps
      tConfig->brp = 32;
      tConfig->tseg_1 = 8;
      tConfig->tseg_2 = 7;
      tConfig->sjw = 3;
      return true;

    default:
      return false;  // 不支持的波特率
  }
}

/* ===========================================================
 * 初始化与配置
 * =========================================================== */

bool initCAN(uint8_t txPin, uint8_t rxPin, uint32_t baudRate) {
  // 保存配置
  canTxPin = txPin;
  canRxPin = rxPin;
  canBaudRate = baudRate;

  // 1. 配置 GPIO 引脚
  twai_gpio_config_t gConfig = TWAI_GPIO_CONFIG_DEFAULT();
  gConfig.tx_io = (gpio_num_t)txPin;
  gConfig.rx_io = (gpio_num_t)rxPin;

  // 2. 配置时序（波特率）
  twai_timing_config_t tConfig;
  if (!getTimingConfig(baudRate, &tConfig)) {
    Serial.print(F("[CAN] 不支持的波特率: "));
    Serial.println(baudRate);
    return false;
  }

  // 3. 配置过滤器（接收所有帧）
  twai_filter_config_t fConfig = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // 4. 安装 TWAI 驱动
  esp_err_t err = twai_driver_install(&gConfig, &tConfig, &fConfig);
  if (err != ESP_OK) {
    Serial.print(F("[CAN] 驱动安装失败, 错误码: "));
    Serial.println(err);
    return false;
  }

  // 5. 启动 TWAI 控制器
  err = twai_start();
  if (err != ESP_OK) {
    Serial.print(F("[CAN] 启动失败, 错误码: "));
    Serial.println(err);
    twai_driver_uninstall();
    return false;
  }

  canInitialized = true;
  canTxCount = 0;
  canRxCount = 0;
  canErrorCount = 0;

  Serial.println(F("[CAN] 初始化成功!"));
  Serial.print(F("[CAN] TX="));
  Serial.print(canTxPin);
  Serial.print(F(" RX="));
  Serial.print(canRxPin);
  Serial.print(F(" 波特率="));
  Serial.print(canBaudRate);
  Serial.println(F("bps"));

  return true;
}

/* ===========================================================
 * 发送 CAN 帧
 * =========================================================== */

bool sendCanFrame(const CanFrame* frame) {
  if (!canInitialized) {
    Serial.println(F("[CAN] 未初始化, 无法发送"));
    return false;
  }

  if (!frame) {
    return false;
  }

  // 构造 TWAI 消息
  twai_message_t message;
  message.identifier = frame->id;
  message.extd = frame->isExtended ? 1 : 0;
  message.rtr = frame->isRemote ? 1 : 0;
  message.data_length_code = frame->len;

  // 复制数据
  for (uint8_t i = 0; i < frame->len && i < 8; i++) {
    message.data[i] = frame->data[i];
  }

  // 发送
  esp_err_t err = twai_transmit(&message, pdMS_TO_TICKS(100));
  if (err == ESP_OK) {
    canTxCount++;
    return true;
  } else {
    canErrorCount++;
    Serial.print(F("[CAN] 发送失败, 错误码: "));
    Serial.println(err);
    return false;
  }
}

bool sendCanMessage(uint32_t id, const uint8_t* data, uint8_t len) {
  CanFrame frame;
  frame.id = id;
  frame.len = (len > 8) ? 8 : len;
  frame.isExtended = false;
  frame.isRemote = false;

  for (uint8_t i = 0; i < frame.len; i++) {
    frame.data[i] = data[i];
  }

  return sendCanFrame(&frame);
}

/* ===========================================================
 * 接收回调管理
 * =========================================================== */

void registerCanCallback(CanFrameCallback callback) {
  if (!callback) return;
  if (callbackCount >= MAX_CALLBACKS) {
    Serial.println(F("[CAN] 回调注册失败: 已满"));
    return;
  }

  // 检查是否已注册
  for (uint8_t i = 0; i < callbackCount; i++) {
    if (callbacks[i] == callback) {
      Serial.println(F("[CAN] 回调已存在"));
      return;
    }
  }

  callbacks[callbackCount++] = callback;
  Serial.println(F("[CAN] 回调注册成功"));
}

void unregisterCanCallback(CanFrameCallback callback) {
  if (!callback) return;

  for (uint8_t i = 0; i < callbackCount; i++) {
    if (callbacks[i] == callback) {
      // 将后面的回调前移
      for (uint8_t j = i; j < callbackCount - 1; j++) {
        callbacks[j] = callbacks[j + 1];
      }
      callbacks[--callbackCount] = NULL;
      Serial.println(F("[CAN] 回调注销成功"));
      return;
    }
  }
}

/* ===========================================================
 * CAN 接收处理（需要在 loop 中周期性调用）
 * =========================================================== */

void processCANReceive() {
  if (!canInitialized) return;

  twai_message_t message;

  // 检查并接收消息（非阻塞）
  esp_err_t err = twai_receive(&message, pdMS_TO_TICKS(0));
  if (err == ESP_OK) {
    canRxCount++;

    // 将 TWAI 消息转换为通用 CanFrame
    CanFrame frame;
    frame.id = message.identifier;
    frame.len = message.data_length_code;
    frame.isExtended = (message.extd == 1);
    frame.isRemote = (message.rtr == 1);

    for (uint8_t i = 0; i < frame.len && i < 8; i++) {
      frame.data[i] = message.data[i];
    }

    // 分发到所有注册的回调
    for (uint8_t i = 0; i < callbackCount; i++) {
      if (callbacks[i]) {
        callbacks[i](&frame);
      }
    }
  } else if (err == ESP_ERR_TIMEOUT) {
    // 没有消息，正常情况
  } else {
    canErrorCount++;
  }

  // 检查总线错误状态
  twai_status_info_t status;
  if (twai_get_status_info(&status) == ESP_OK) {
    if (status.state == TWAI_STATE_BUS_OFF) {
      Serial.println(F("[CAN] 总线关闭! 尝试恢复..."));
      twai_initiate_recovery();  // 尝试恢复
    }
  }
}

/* ===========================================================
 * 状态查询
 * =========================================================== */

bool isCANActive() {
  if (!canInitialized) return false;

  twai_status_info_t status;
  if (twai_get_status_info(&status) != ESP_OK) return false;

  return (status.state == TWAI_STATE_RUNNING);
}

void printCANStatus() {
  Serial.println(F("=== CAN 总线状态 ==="));
  Serial.print(F("初始化: "));
  Serial.println(canInitialized ? F("是") : F("否"));

  Serial.print(F("引脚: TX="));
  Serial.print(canTxPin);
  Serial.print(F(" RX="));
  Serial.print(canRxPin);

  Serial.print(F(" 波特率: "));
  Serial.print(canBaudRate);
  Serial.println(F(" bps"));

  Serial.print(F("发送: "));
  Serial.print(canTxCount);
  Serial.print(F(" 接收: "));
  Serial.print(canRxCount);
  Serial.print(F(" 错误: "));
  Serial.println(canErrorCount);

  if (canInitialized) {
    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK) {
      Serial.print(F("总线状态: "));
      switch (status.state) {
        case TWAI_STATE_STOPPED:  Serial.println(F("停止")); break;
        case TWAI_STATE_RUNNING:  Serial.println(F("运行中")); break;
        case TWAI_STATE_BUS_OFF:  Serial.println(F("总线关闭")); break;
        case TWAI_STATE_RECOVERING: Serial.println(F("恢复中")); break;
        default: Serial.println(F("未知")); break;
      }

      Serial.print(F("发送错误: "));
      Serial.print(status.tx_error_counter);
      Serial.print(F(" 接收错误: "));
      Serial.println(status.rx_error_counter);
    }
  }
  Serial.println(F("==================="));
}

/* ===========================================================
 * 非 ESP32 平台：空实现（占位）
 * =========================================================== */
#else

bool initCAN(uint8_t txPin, uint8_t rxPin, uint32_t baudRate) {
  // 非 ESP32 平台不支持 CAN
  return false;
}

bool sendCanFrame(const CanFrame* frame) { return false; }
bool sendCanMessage(uint32_t id, const uint8_t* data, uint8_t len) { return false; }
void registerCanCallback(CanFrameCallback callback) {}
void unregisterCanCallback(CanFrameCallback callback) {}
void processCANReceive() {}
bool isCANActive() { return false; }
void printCANStatus() {}

#endif
