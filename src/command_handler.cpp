#include "command_handler.h"
#include "led_controller.h"
#include "system_state.h"
#include "emergency.h"
#include "can_handler.h"  // CAN 模块

void handleSerialCommands() {
  if (Serial.available() > 0) {
    char input[20];
    uint8_t index = 0;

    while (Serial.available() > 0 && index < 19) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (index == 0) continue;
        break;
      }
      input[index++] = c;
    }
    input[index] = '\0';

    if (index > 0) {
      processCommand(input);
    }
  }
}

void processCommand(const char* command) {
  char cmd[20];
  strcpy(cmd, command);
  for(uint8_t i = 0; cmd[i]; i++) {
    cmd[i] = toLowerCase(cmd[i]);
  }

  if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
    showHelp();
    return;
  }

  if (strcmp(cmd, "status") == 0) {
    showCurrentState();
    return;
  }

        if (strcmp(cmd, "test") == 0) {
      currentState = STATE_TEST;
      testStepTime = 0;
      testColorIndex = 0;
      Serial.println(F("命令: 7 - 颜色测试"));
      showCurrentState();
      return;
    }

            // CAN 测试命令
    if (strcmp(cmd, "cantest") == 0) {
      uint8_t data[1] = {2};
      Serial.println(F("[命令] 发送CAN测试帧(红色常亮)..."));
      if (sendCanMessage(0x100, data, 1)) {
        Serial.println(F("[命令] CAN测试帧已发送, LED将变为红色"));
      } else {
        Serial.println(F("[命令] CAN发送失败(未初始化?)"));
      }
      return;
    }

    if (strcmp(cmd, "canstatus") == 0) {
      printCANStatus();
      return;
    }

  if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "0") == 0) {
    setAllLEDs(0, 0, 0);
    currentState = STATE_IDLE;
    Serial.println(F("命令: 关闭所有LED"));
    return;
  }

  uint8_t commandNumber = atoi(cmd);

  switch(commandNumber) {
    case 1:
      currentState = STATE_UNINITIALIZED;
      Serial.println(F("命令: 1 - 彩色走马灯"));
      break;
    case 2:
      currentState = STATE_CHARGING;
      Serial.println(F("命令: 2 - 红色常亮"));
      break;
    case 3:
      currentState = STATE_CHARGE_COMPLETE;
      Serial.println(F("命令: 3 - 绿色常亮"));
      break;
    case 4:
      currentState = STATE_WALKING;
      Serial.println(F("命令: 4 - 蓝色常亮"));
      break;
    case 5:
      currentState = STATE_MOVING;
      Serial.println(F("命令: 5 - 蓝色走马灯"));
      break;
    case 6:
      currentState = STATE_STATIONARY;
      Serial.println(F("命令: 6 - 白色常亮"));
      break;
    case 7:
      currentState = STATE_TEST;
      testStepTime = 0;
      testColorIndex = 0;
      Serial.println(F("命令: 7 - 颜色测试"));
      break;
    case 8:
      currentState = STATE_BREATHING;
      breathingStartTime = 0;
      breathingLastUpdate = 0;
      Serial.println(F("命令: 8 - 呼吸效果"));
      break;
    case 9:
      currentState = STATE_RAINBOW;
      rainbowStartTime = 0;
      rainbowLastUpdate = 0;
      rainbowHue = 0;
      Serial.println(F("命令: 9 - 彩虹效果"));
      break;
    default:
      Serial.print(F("未知命令: "));
      Serial.println(command);
      Serial.println(F("输入 'help' 查看命令"));
      return;
  }

  showCurrentState();
}

void showHelp() {
  Serial.println(F("\n=== 可用命令 ==="));
  Serial.println(F("0/clear: 关闭LED"));
  Serial.println(F("1: 彩色走马灯"));
  Serial.println(F("2: 红色常亮"));
  Serial.println(F("3: 绿色常亮"));
  Serial.println(F("4: 蓝色常亮"));
  Serial.println(F("5: 蓝色走马灯"));
  Serial.println(F("6: 白色常亮"));
  Serial.println(F("7/test: 测试颜色"));
  Serial.println(F("8: 呼吸效果"));
  Serial.println(F("9: 彩虹效果"));
    Serial.println(F("help: 帮助信息"));
    Serial.println(F("status: 当前状态"));
    Serial.println(F("cantest: CAN自发自收测试"));
    Serial.println(F("canstatus: CAN总线状态"));
    Serial.println(F("=========================="));
}

void showCurrentState() {
  Serial.print(F("状态: "));
  printStateName(currentState);

  if (emergencyActive) {
    Serial.print(F(" [急停]"));
  }

  Serial.print(F(" | 时间: "));
  Serial.println(getTimeString());
}

// testAllColors() 已废弃，功能由 STATE_TEST 在 updateLEDDisplay() 中以非阻塞方式实现

String getTimeString() {
  uint32_t totalSeconds = millis() / 1000;
  uint32_t hours = totalSeconds / 3600;
  uint32_t minutes = (totalSeconds % 3600) / 60;
  uint32_t seconds = totalSeconds % 60;

  char buffer[20];
  sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
  return String(buffer);
}

void printStateName(SystemState state) {
  switch(state) {
    case STATE_IDLE: Serial.print(F("空闲")); break;
    case STATE_UNINITIALIZED: Serial.print(F("未初始化")); break;
    case STATE_CHARGING: Serial.print(F("充电中")); break;
    case STATE_CHARGE_COMPLETE: Serial.print(F("充电完成")); break;
    case STATE_WALKING: Serial.print(F("行走")); break;
    case STATE_MOVING: Serial.print(F("前后运动")); break;
    case STATE_STATIONARY: Serial.print(F("静止")); break;
    case STATE_TEST: Serial.print(F("颜色测试")); break;
    case STATE_BREATHING: Serial.print(F("呼吸效果")); break;
    case STATE_RAINBOW: Serial.print(F("彩虹效果")); break;
  }
}

char toLowerCase(char c) {
  if (c >= 'A' && c <= 'Z') {
    return c + 32;
  }
  return c;
}
