#ifndef CAN_RELAY_H
#define CAN_RELAY_H

#include <Arduino.h>
#include "config.h"

/* ===========================================================
 * CAN 继电器模块协议定义
 *
 * 模块描述：8路继电器，通过 CAN 标准帧控制
 *
 * 帧格式（写单路）：
 *   CAN ID: 0x201  (功能码0x02 + 站号0x01)
 *   Data[0]: 寄存器地址 (0x00~0x07 对应 CH1~CH8)
 *   Data[1]: 数据高位  (0xFF=开启, 0x00=关闭)
 *   Data[2]: 数据低位  (固定 0x00)
 *
 * 帧格式（读单路）：
 *   CAN ID: 0x200  (功能码0x01 + 站号0x01)
 *   Data[0]: 寄存器地址 (0x00~0x07 对应 CH1~CH8)
 *
 * 帧格式（读多路）：
 *   CAN ID: 0x200  (功能码0x01 + 站号0x01)
 *   Data[0]: 0x00  (起始地址)
 *   Data[1]: 读取数量 (1~8)
 *
 * 回复帧：
 *   CAN ID: 0x200 (回复，带 RTR 或数据)
 *   Data[0]: 寄存器地址
 *   Data[1]: 状态 (0xFF=开, 0x00=关)
 * =========================================================== */

// ========== CAN 继电器模块 ID 定义 ==========
#define CAN_RELAY_CMD_ID      0x201   // 写命令 ID (功能码0x02 + 站号0x01)
#define CAN_RELAY_QUERY_ID    0x200   // 读命令 ID (功能码0x01 + 站号0x01)
#define CAN_RELAY_STATION     0x01    // 站号

// ========== 继电器通道映射 ==========
#define RELAY_CH1   0x00
#define RELAY_CH2   0x01
#define RELAY_CH3   0x02
#define RELAY_CH4   0x03
#define RELAY_CH5   0x04
#define RELAY_CH6   0x05
#define RELAY_CH7   0x06
#define RELAY_CH8   0x07

// ========== 继电器状态 ==========
#define RELAY_ON    0xFF
#define RELAY_OFF   0x00

// ========== 通道编号（人性化 1~8） ==========
#define RELAY_CH_MIN    1
#define RELAY_CH_MAX    8

// ========== 外部变量：记录 8 路继电器当前状态 ==========
extern bool relayState[8];  // true=导通(闭合), false=断开

/* ===========================================================
 * 函数声明
 * =========================================================== */

/**
 * @brief 控制单路继电器 (通过 CAN 发送命令给外部继电器模块)
 * @param ch   通道号 (1~8)
 * @param on   true=吸合(闭合), false=断开
 * @return true=发送成功, false=失败
 */
bool relaySet(uint8_t ch, bool on);

/**
 * @brief 翻转单路继电器状态
 * @param ch   通道号 (1~8)
 * @return true=发送成功, false=失败
 */
bool relayToggle(uint8_t ch);

/**
 * @brief 打开单路继电器
 * @param ch   通道号 (1~8)
 */
bool relayOn(uint8_t ch);

/**
 * @brief 关闭单路继电器
 * @param ch   通道号 (1~8)
 */
bool relayOff(uint8_t ch);

/**
 * @brief 批量设置所有 8 路继电器
 * @param bitmask  8位掩码 (bit0=CH1, bit7=CH8, 1=闭合)
 */
bool relaySetAll(uint8_t bitmask);

/**
 * @brief 全部打开
 */
bool relayAllOn(void);

/**
 * @brief 全部关闭
 */
bool relayAllOff(void);

/**
 * @brief 查询单路继电器状态 (发送读命令到模块)
 * @param ch   通道号 (1~8)
 * @return true=发送成功
 */
bool relayQuery(uint8_t ch);

/**
 * @brief 打印所有继电器状态到串口
 */
void relayPrintStatus(void);

/**
 * @brief 处理 CAN 继电器模块的回复帧（在 onCanCommand 中调用）
 * @param id    CAN ID
 * @param data  数据
 * @param len   数据长度
 */
void relayProcessResponse(uint32_t id, const uint8_t* data, uint8_t len);

#endif // CAN_RELAY_H
