#include <Adafruit_NeoPixel.h>

// ====== 配置参数 ======
#define LED_PIN 7            // WS2812数据引脚
#define EMERGENCY_PIN 2      // 急停开关引脚
#define NUMPIXELS 100          // LED数量
// =====================

// 创建WS2812对象
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// 系统状态枚举（使用最小数据类型）
enum SystemState : uint8_t {
  STATE_IDLE,               // 空闲状态
  STATE_UNINITIALIZED,      // 1. 未初始化
  STATE_CHARGING,           // 2. 充电中
  STATE_CHARGE_COMPLETE,    // 2. 充电完成
  STATE_WALKING,            // 3. 行走
  STATE_MOVING,             // 3. 前后运动
  STATE_STATIONARY,         // 4. 静止
  STATE_TEST,               // 5. 颜色测试
  STATE_BREATHING,          // 6. 呼吸效果
  STATE_RAINBOW,            // 7. 彩虹效果
  STATE_EMERGENCY_STOP      // 8. 急停
};

// 全局变量（最小化）
SystemState currentState = STATE_IDLE;
SystemState lastState = STATE_IDLE;
bool emergencyActive = false;
bool ledInitialized = false;
uint32_t emergencyStartTime = 0;

uint32_t testStepTime = 0;
uint8_t testColorIndex = 0;
uint32_t breathingStartTime = 0;
uint32_t breathingLastUpdate = 0;
uint32_t rainbowStartTime = 0;
uint32_t rainbowLastUpdate = 0;
uint16_t rainbowHue = 0;

// 函数预声明
void handleEmergencyInterrupt();
void handleSerialCommands();
void processCommand(const char* command);
void updateLEDDisplay(uint32_t currentTime, uint32_t* lastLEDUpdate);
void setAllLEDs(uint8_t r, uint8_t g, uint8_t b);
void showHelp();
void showCurrentState();
void showEmergencyStatus(uint32_t currentTime);
void testAllColors();
void breathingEffect();
void rainbowEffect(uint8_t wait);
uint32_t Wheel(uint8_t WheelPos);
String getTimeString();
void printStateName(SystemState state);
char toLowerCase(char c);

void setup() {
  Serial.begin(9600);
  
  // 初始化急停引脚
  pinMode(EMERGENCY_PIN, INPUT_PULLUP);

  // 这里的 CHANGE 表示引脚电平一旦发生变化（按下或松开）就触发
  attachInterrupt(digitalPinToInterrupt(EMERGENCY_PIN), handleEmergencyInterrupt, CHANGE);
  
  // 初始化WS2812
  pixels.begin();
  pixels.setBrightness(20);
  pixels.clear();
  pixels.show();
  ledInitialized = true;
  
  delay(500);
  
  // 使用F()宏节省内存
  Serial.println(F("========================================"));
  Serial.println(F("    串口控制WS2812灯带系统"));
  Serial.println(F("========================================"));
  Serial.println(F("命令: 0=关闭 1=彩色走马灯 2=红 3=绿"));
  Serial.println(F("      4=蓝 5=蓝色走马灯 6=白 7=测试"));
  Serial.println(F("      8=呼吸 9=彩虹 help=帮助"));
  Serial.println(F("----------------------------------------"));
  Serial.println(F("硬件: D2=急停 D7=LED 数量=8"));
  Serial.println(F("波特率: 9600"));
  Serial.println(F("----------------------------------------"));
  Serial.println(F("等待命令..."));
  Serial.println(F("========================================"));
  
  showCurrentState();
}

void loop() {
  uint32_t currentTime = millis();
  static uint32_t lastLEDUpdate = 0;
  static uint32_t lastStatusUpdate = 0;

  // ======================================================
  // 1. 最高优先级拦截：急停激活
  // ======================================================
  if (emergencyActive) {
    if (emergencyStartTime == 0) {
      emergencyStartTime = currentTime;
    }

    // 强制维持全红（防止被残留的动画逻辑刷掉）
    setAllLEDs(255, 0, 0); 
    
    // 每隔1秒打印一次急停状态
    if (currentTime - lastStatusUpdate > 1000) {
      lastStatusUpdate = currentTime;
      showEmergencyStatus(currentTime);
    }
    
    // 【关键】直接跳过后续所有代码，不处理串口，不更新正常动画
    return; 
  }

  emergencyStartTime = 0;

  // ======================================================
  // 2. 正常状态处理
  // ======================================================
  
  // 处理串口输入命令
  handleSerialCommands();
  
  // 更新当前的灯光模式（模式1-6是非阻塞的，会正常运行）
  updateLEDDisplay(currentTime, &lastLEDUpdate);
  
  // 3. 定时打印系统当前状态（1秒一次）
  if (currentTime - lastStatusUpdate > 1000) {
    lastStatusUpdate = currentTime;
    showCurrentState();
  }

  // 这里的极短延迟是为了让处理器有空余处理后台任务（如串口缓冲）
  delay(1); 
}


// 处理串口命令
void handleSerialCommands() {
  if (Serial.available() > 0) {
    char input[20];
    uint8_t index = 0;

    // 读取一行输入
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

// 处理命令
void processCommand(const char* command) {
  // 转换为小写比较
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
    testAllColors();
    return;
  }
  
  if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "0") == 0) {
    setAllLEDs(0, 0, 0);
    currentState = STATE_IDLE;
    Serial.println(F("命令: 关闭所有LED"));
    return;
  }
  
  // 数字命令
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

// 更新LED显示
void updateLEDDisplay(uint32_t currentTime, uint32_t* lastLEDUpdate) {
  if (!ledInitialized) return;
  
  if (emergencyActive) {
      setAllLEDs(255, 0, 0);
      return;
  }
  
  static uint16_t animationStep = 0;
  static uint8_t ledPosition = 0;

  static const uint8_t testColors[7][3] = {
    {255, 0, 0},
    {0, 255, 0},
    {0, 0, 255},
    {255, 255, 0},
    {255, 0, 255},
    {0, 255, 255},
    {255, 255, 255}
  };
  
  switch(currentState) {
    case STATE_UNINITIALIZED:
      if (currentTime - *lastLEDUpdate > 100) {
        *lastLEDUpdate = currentTime;
        animationStep = (animationStep + 1) % 256;
        
        for(uint8_t i = 0; i < NUMPIXELS; i++) {
          uint8_t hue = ((i * 256 / NUMPIXELS) + animationStep) & 255;
          pixels.setPixelColor(i, Wheel(hue));
        }
        pixels.show();
      }
      break;
      
    case STATE_CHARGING:
      setAllLEDs(255, 0, 0);
      break;
      
    case STATE_CHARGE_COMPLETE:
      setAllLEDs(0, 255, 0);
      break;
      
    case STATE_WALKING:
      setAllLEDs(0, 0, 255);
      break;
      
    case STATE_MOVING:
      if (currentTime - *lastLEDUpdate > 150) {
        *lastLEDUpdate = currentTime;
        ledPosition = (ledPosition + 1) % NUMPIXELS;
        
        for(uint8_t i = 0; i < NUMPIXELS; i++) {
          if (i == ledPosition) {
            pixels.setPixelColor(i, pixels.Color(0, 0, 255));
          } else if (i == (ledPosition + 1) % NUMPIXELS || 
                     i == (ledPosition - 1 + NUMPIXELS) % NUMPIXELS) {
            pixels.setPixelColor(i, pixels.Color(0, 0, 100));
          } else {
            pixels.setPixelColor(i, pixels.Color(0, 0, 20));
          }
        }
        pixels.show();
      }
      break;
      
    case STATE_STATIONARY:
      setAllLEDs(255, 255, 255);
      break;
      
    case STATE_TEST:
      if (testStepTime == 0) {
        testStepTime = currentTime;
        testColorIndex = 0;
      }
      if (currentTime - testStepTime >= 800) {
        testStepTime += 800;
        testColorIndex++;
      }
      if (testColorIndex >= 7) {
        currentState = STATE_IDLE;
        testStepTime = 0;
        setAllLEDs(0, 0, 0);
        break;
      }
      setAllLEDs(testColors[testColorIndex][0], testColors[testColorIndex][1], testColors[testColorIndex][2]);
      break;
      
    case STATE_BREATHING:
      if (breathingStartTime == 0) {
        breathingStartTime = currentTime;
        breathingLastUpdate = currentTime;
      }
      if (currentTime - breathingStartTime >= 10000) {
        currentState = STATE_IDLE;
        breathingStartTime = 0;
        setAllLEDs(0, 0, 0);
        break;
      }
      if (currentTime - breathingLastUpdate >= 20) {
        breathingLastUpdate = currentTime;
        uint16_t phase = ((currentTime - breathingStartTime) / 20) & 255;
        uint8_t brightness = phase < 128 ? phase * 2 : 255 - ((phase - 128) * 2);
        setAllLEDs(0, brightness, 0);
      }
      break;
      
    case STATE_RAINBOW:
      if (rainbowStartTime == 0) {
        rainbowStartTime = currentTime;
        rainbowLastUpdate = currentTime;
        rainbowHue = 0;
      }
      if (currentTime - rainbowStartTime >= 10000) {
        currentState = STATE_IDLE;
        rainbowStartTime = 0;
        setAllLEDs(0, 0, 0);
        break;
      }
      if (currentTime - rainbowLastUpdate >= 20) {
        rainbowLastUpdate = currentTime;
        for (uint8_t i = 0; i < NUMPIXELS; i++) {
          uint16_t pixelHue = rainbowHue + (i * 65536L / NUMPIXELS);
          pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(pixelHue)));
        }
        pixels.show();
        rainbowHue += 256;
      }
      break;
      
    default:
      break;
  }
}

// 设置所有LED颜色
void setAllLEDs(uint8_t r, uint8_t g, uint8_t b) {
  for(uint8_t i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();
}

// 彩虹颜色轮（优化版本）
uint32_t Wheel(uint8_t WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// 显示帮助信息
void showHelp() {
  Serial.println(F("\n=== 可用命令 ==="));
  Serial.println(F("0/clear: 关闭LED"));
  Serial.println(F("1: 彩色走马灯"));
  Serial.println(F("2: 红色常亮"));
  Serial.println(F("3: 绿色常亮"));
  Serial.println(F("4: 蓝色常亮"));
  Serial.println(F("5: 蓝色走马灯"));
  Serial.println(F("6: 白色常亮"));
  Serial.println(F("7: 测试颜色"));
  Serial.println(F("8: 呼吸效果"));
  Serial.println(F("9: 彩虹效果"));
  Serial.println(F("help: 帮助信息"));
  Serial.println(F("status: 当前状态"));
  Serial.println(F("=========================="));
}

// 显示当前状态
void showCurrentState() {
  Serial.print(F("状态: "));
  printStateName(currentState);
  
  if (emergencyActive) {
    Serial.print(F(" [急停]"));
  }
  
  Serial.print(F(" | 时间: "));
  Serial.println(getTimeString());
}

// 显示急停状态
void showEmergencyStatus(uint32_t currentTime) {
  if (emergencyStartTime == 0) {
    emergencyStartTime = currentTime;
  }
  
  uint32_t duration = (currentTime - emergencyStartTime) / 1000;
  Serial.print(F("[急停] 持续时间: "));
  Serial.print(duration);
  Serial.println(F("秒"));
}

// 测试所有颜色
void testAllColors() {
  Serial.println(F("\n开始颜色测试..."));
  
  const uint8_t colors[7][3] = {
    {255, 0, 0},    // 红
    {0, 255, 0},    // 绿
    {0, 0, 255},    // 蓝
    {255, 255, 0},  // 黄
    {255, 0, 255},  // 紫
    {0, 255, 255},  // 青
    {255, 255, 255} // 白
  };
  
  const char* colorNames[] = {"红色", "绿色", "蓝色", "黄色", "紫色", "青色", "白色"};
  
  for(uint8_t i = 0; i < 7; i++) {
    if (emergencyActive) return;
    Serial.print(F("显示: "));
    Serial.println(colorNames[i]);
    setAllLEDs(colors[i][0], colors[i][1], colors[i][2]);
    delay(800);
  }
  
  Serial.println(F("颜色测试完成"));
  currentState = STATE_IDLE;
  setAllLEDs(0, 0, 0);
}

// 呼吸效果
void breathingEffect() {
  Serial.println(F("\n开始呼吸效果 (10秒)"));
  
  currentState = STATE_IDLE;
  uint32_t startTime = millis();
  
  for(uint16_t i = 0; i < 500; i++) {
    if (emergencyActive || Serial.available() > 0) return;
    if (Serial.available() > 0 || (millis() - startTime > 10000)) {
      break;
    }
    
    uint8_t brightness = 128 + 127 * sin(i * 3.14159 / 128.0);
    setAllLEDs(0, brightness, 0);
    delay(20);
  }
  
  Serial.println(F("呼吸效果结束"));
  setAllLEDs(0, 0, 0);
}

// 彩虹效果
void rainbowEffect(uint8_t wait) {
  Serial.println(F("\n开始彩虹效果 (10秒)"));
  
  currentState = STATE_IDLE; // 进入彩虹模式时，清空当前状态防止冲突
  uint32_t startTime = millis();
  
  // 运行条件：时间没到 10 秒 且 没有按下急停 且 串口没有新命令
  while (millis() - startTime < 10000) {
    
    // 第一层：每跑一轮完整彩虹色轮前检查一次
    if (emergencyActive || Serial.available() > 0) break;

    for (uint16_t firstPixelHue = 0; firstPixelHue < 65536; firstPixelHue += 256) {
      
      // 第二层：【最重要】在每一帧动画渲染前检查。
      // 只要急停激活或串口有新指令，立刻“斩断”执行，彻底跳出函数
      if (emergencyActive || Serial.available() > 0) {
        Serial.println(F("彩虹效果被强行中断"));
        return; 
      }
      
      for (uint8_t i = 0; i < NUMPIXELS; i++) {
        uint16_t pixelHue = firstPixelHue + (i * 65536L / NUMPIXELS);
        pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(pixelHue)));
      }
      
      pixels.show();
      delay(wait); // 这里的等待时间越短，检查的频率就越高
    }
  }
  
  Serial.println(F("彩虹效果自然结束"));
  
  // 只有在非急停状态下才自动关灯，如果是急停触发的结束，交给 loop 处理红色
  if (!emergencyActive) {
    setAllLEDs(0, 0, 0);
  }
}

// 获取时间字符串
String getTimeString() {
  uint32_t totalSeconds = millis() / 1000;
  uint32_t hours = totalSeconds / 3600;
  uint32_t minutes = (totalSeconds % 3600) / 60;
  uint32_t seconds = totalSeconds % 60;
  
  char buffer[20];
  sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
  return String(buffer);
}

// 打印状态名称
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
    case STATE_EMERGENCY_STOP: Serial.print(F("急停")); break;
  }
}

// 小写转换函数
char toLowerCase(char c) {
  if (c >= 'A' && c <= 'Z') {
    return c + 32;
  }
  return c;
}

// 紧急状态处理函数（中断服务程序）
void handleEmergencyInterrupt() {
  // 直接读取引脚，判断是按下还是松开
  bool pressed = (digitalRead(EMERGENCY_PIN) == LOW);
  emergencyActive = pressed;
}
