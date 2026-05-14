#pragma once


#include <Arduino.h>

/* ===========================================================
 * CAN 帧结构体定义
 * =========================================================== */
typedef struct {
  uint32_t id;         // CAN ID (标准帧: 0~0x7FF, 扩展帧: 0~0x1FFFFFFF)
  uint8_t data[8];     // 数据 (最多8字节)
  uint8_t len;         // 数据长度 (0~8)
  bool isExtended;     // true=扩展帧, false=标准帧
  bool isRemote;       // true=远程帧, false=数据帧
} CanFrame;

/* ===========================================================
 * CAN 接收回调函数类型
 * ===========================================================
 * 注册回调后，收到CAN帧时会自动调用
 * 不同设备可以注册自己的回调来处理特定ID范围的帧
 */
typedef void (*CanFrameCallback)(const CanFrame* frame);

/* ===========================================================
 * CAN 初始化与配置
 * =========================================================== */

/**
 * @brief 初始化 CAN 总线
 * @param txPin   CAN TX 引脚
 * @param rxPin   CAN RX 引脚
 * @param baudRate 波特率 (常见: 125000, 250000, 500000, 1000000)
 * @return true = 初始化成功, false = 失败
 */
bool initCAN(uint8_t txPin, uint8_t rxPin, uint32_t baudRate);

/**
 * @brief 发送一帧 CAN 数据
 * @param frame  要发送的 CAN 帧结构体指针
 * @return true = 发送成功, false = 失败
 */
bool sendCanFrame(const CanFrame* frame);

/**
 * @brief 简易发送函数 (标准帧，数据帧)
 * @param id   CAN ID
 * @param data 数据数组
 * @param len  数据长度
 * @return true = 成功, false = 失败
 */
bool sendCanMessage(uint32_t id, const uint8_t* data, uint8_t len);

/**
 * @brief 注册 CAN 接收回调
 * @param callback 回调函数指针
 */
void registerCanCallback(CanFrameCallback callback);

/**
 * @brief 取消注册 CAN 接收回调
 * @param callback 要取消的回调函数指针
 */
void unregisterCanCallback(CanFrameCallback callback);

/**
 * @brief 获取 CAN 状态
 * @return true = 总线正常运行, false = 有错误
 */
bool isCANActive();

/**
 * @brief 打印 CAN 状态信息到串口
 */
void printCANStatus();

/**
 * @brief CAN 接收处理 (需要在 loop() 中周期性调用)
 * 或者在 FreeRTOS 任务中调用
 */
void processCANReceive();

