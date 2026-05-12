#include "can_relay.h"
#include "can_handler.h"

/* ===========================================================
 * CAN 继电器模块控制实现
 *
 * 本文件负责向外部 8 路 CAN 继电器模块发送控制指令。
 * 模块通过 CAN 标准帧 0x201 控制，0x200 查询。
 * =========================================================== */

// ========== 8 路继电器状态记录 ==========
bool relayState[8] = {false, false, false, false, false, false, false, false};

/* ===========================================================
 * 内部辅助：发送 CAN 写命令帧
 * =========================================================== */
static bool sendRelayCmd(uint8_t regAddr, uint8_t value) {
  uint8_t data[3] = {
    regAddr,    // Data[0]: 寄存器地址 (0x00~0x07)
    value,      // Data[1]: 数据高位 (0xFF=开, 0x00=关)
    0x00        // Data[2]: 数据低位 (固定0x00)
  };

  bool result = sendCanMessage(CAN_RELAY_CMD_ID, data, 3);
  if (result) {
    Serial.print(F("[继电器] CAN发送 OK: ID=0x"));
    Serial.print(CAN_RELAY_CMD_ID, HEX);
    Serial.print(F(" reg=0x"));
    Serial.print(regAddr, HEX);
    Serial.print(F(" val=0x"));
    Serial.print(value, HEX);
    Serial.println();
  } else {
    Serial.println(F("[继电器] CAN发送失败!"));
  }
  return result;
}

/* ===========================================================
 * 控制单路继电器
 * =========================================================== */
bool relaySet(uint8_t ch, bool on) {
  if (ch < RELAY_CH_MIN || ch > RELAY_CH_MAX) {
    Serial.print(F("[继电器] 通道号错误: "));
    Serial.println(ch);
    return false;
  }

  uint8_t regAddr = ch - 1;           // CH1→0x00, CH2→0x01, ...
  uint8_t value = on ? RELAY_ON : RELAY_OFF;

  bool result = sendRelayCmd(regAddr, value);
  if (result) {
    relayState[ch - 1] = on;
    Serial.print(F("[继电器] CH"));
    Serial.print(ch);
    Serial.println(on ? F(" 吸合 (ON)") : F(" 断开 (OFF)"));
  }
  return result;
}

/* ===========================================================
 * 翻转单路继电器
 * =========================================================== */
bool relayToggle(uint8_t ch) {
  if (ch < RELAY_CH_MIN || ch > RELAY_CH_MAX) {
    Serial.print(F("[继电器] 通道号错误: "));
    Serial.println(ch);
    return false;
  }

  bool newState = !relayState[ch - 1];
  return relaySet(ch, newState);
}

/* ===========================================================
 * 打开单路
 * =========================================================== */
bool relayOn(uint8_t ch) {
  return relaySet(ch, true);
}

/* ===========================================================
 * 关闭单路
 * =========================================================== */
bool relayOff(uint8_t ch) {
  return relaySet(ch, false);
}

/* ===========================================================
 * 批量设置所有 8 路
 * =========================================================== */
bool relaySetAll(uint8_t bitmask) {
  bool allOk = true;
  for (uint8_t i = 0; i < 8; i++) {
    bool on = (bitmask >> i) & 0x01;
    if (!relaySet(i + 1, on)) {
      allOk = false;
    }
  }
  return allOk;
}

/* ===========================================================
 * 全部打开
 * =========================================================== */
bool relayAllOn(void) {
  return relaySetAll(0xFF);
}

/* ===========================================================
 * 全部关闭
 * =========================================================== */
bool relayAllOff(void) {
  return relaySetAll(0x00);
}

/* ===========================================================
 * 查询单路继电器状态
 * =========================================================== */
bool relayQuery(uint8_t ch) {
  if (ch < RELAY_CH_MIN || ch > RELAY_CH_MAX) {
    return false;
  }

  uint8_t regAddr = ch - 1;
  uint8_t data[2] = {
    regAddr,    // 起始地址
    0x01        // 读取数量
  };

  bool result = sendCanMessage(CAN_RELAY_QUERY_ID, data, 2);
  if (result) {
    Serial.print(F("[继电器] 查询 CH"));
    Serial.print(ch);
    Serial.println(F(" 状态..."));
  }
  return result;
}

/* ===========================================================
 * 打印所有继电器状态
 * =========================================================== */
void relayPrintStatus(void) {
  Serial.println(F("=== CAN 继电器状态 ==="));
  for (uint8_t i = 0; i < 8; i++) {
    Serial.print(F("  CH"));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.println(relayState[i] ? F("闭合 (ON)") : F("断开 (OFF)"));
  }
  Serial.println(F("====================="));
}

/* ===========================================================
 * 处理 CAN 继电器模块的回复帧
 *
 * 模块回复格式：
 *   CAN ID: 0x200
 *   Data[0]: 寄存器地址
 *   Data[1]: 状态
 *
 * 需要在 onCanCommand 回调中调用此函数
 * =========================================================== */
void relayProcessResponse(uint32_t id, const uint8_t* data, uint8_t len) {
  if (id == CAN_RELAY_QUERY_ID && len >= 2) {
    uint8_t regAddr = data[0];    // 寄存器地址 (0x00~0x07)
    uint8_t status = data[1];     // 状态

    if (regAddr < 8) {
      bool isOn = (status == RELAY_ON);
      relayState[regAddr] = isOn;

      Serial.print(F("[继电器] 收到回复: CH"));
      Serial.print(regAddr + 1);
      Serial.println(isOn ? F(" = ON") : F(" = OFF"));
    }
  }
}
