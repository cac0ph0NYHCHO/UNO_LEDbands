# ESP32-UNO_LEDbands 程序流程图

> 从 Arduino UNO 移植到 ESP32-S3 后的完整项目结构

---

## 1. 启动流程 (setup)

```
程序启动 (ESP32-S3)
  ↓
Serial.begin(115200)          [串口初始化，速率从 UNO 的 9600 提升到 115200]
  ↓
initEmergency()               [急停初始化 - 硬件中断]
  ├─ pinMode(EMERGENCY_PIN, INPUT_PULLUP)
  └─ attachInterrupt(..., handleEmergencyInterrupt, CHANGE)
  ↓
initLEDs()                    [WS2812 LED 初始化]
  ├─ pixels.begin()
  ├─ pixels.setBrightness(20)
  ├─ pixels.clear()
  └─ pixels.show()
  ↓
initButton()                  [按键初始化 - GPIO 轮询检测]
  └─ pinMode(BUTTON_PIN, INPUT_PULLUP)
  ↓
GPIO_Init()                   [板载 RGB + 蜂鸣器初始化]
  ├─ pinMode(GPIO_PIN_RGB, OUTPUT)
  ├─ pinMode(GPIO_PIN_Buzzer, OUTPUT)
  ├─ ledcSetup(PWM_Channel, Frequency, Resolution)
  ├─ ledcAttachPin(GPIO_PIN_Buzzer, PWM_Channel)
  ├─ xTaskCreatePinnedToCore(RGBTask, ..., Core 0)      ← FreeRTOS 独立任务
  └─ xTaskCreatePinnedToCore(BuzzerTask, ..., Core 0)   ← FreeRTOS 独立任务
  ↓
I2C_Init()                    [I2C 总线初始化 - 驱动 EXIO 扩展器]
  └─ Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN)    [SDA=42, SCL=41]
  ↓
Dout_Init()                   [EXIO 数字输出初始化 - TCA9554PWR]
  └─ TCA9554PWR_Init(0x00, 0xFF)  [全部设为输出，初始高电平]
  ↓
WIFI_Init()                   [WiFi STA 初始化 - FreeRTOS 独立任务]
  └─ xTaskCreatePinnedToCore(WifiStaTask, ..., Core 0)  ← 独立任务
      ├─ 连接路由器 (SSID: SENAD_5845)
      ├─ 配置静态 IP (10.7.5.82)
      ├─ 启动 WebServer (端口 80)
      └─ 循环处理 HTTP 客户端请求
  ↓
initCAN()                     [CAN 总线初始化 - TWAI 控制器]
  ├─ 回环模式: 软件模拟，无需硬件
  │   └─ 发送的帧直接注入接收回调
  ├─ 正常模式: 配置 TWAI 驱动
  │   ├─ twai_driver_install()
  │   ├─ twai_start()
  │   └─ twai_reconfigure_alerts()
  └─ registerCanCallback(onCanCommand)
  ↓
delay(500)
  ↓
打印启动信息 + 命令帮助
  ↓
showCurrentState()            [显示当前状态]
  ↓
等待主循环
```

---

## 2. 主循环流程 (loop) — Core 1 上运行

```
loop()  每一次迭代
  ↓
processCANReceive()           [CAN 接收轮询]
  ├─ 回环模式: 跳过（发送时已注入回调）
  └─ 正常模式: twai_receive() 非阻塞读取
      ├─ 有消息 → 分发到所有注册回调
      └─ 无消息 → 跳过
  ↓
updateButton()                [按键轮询检测]
  ├─ 读取 GPIO 电平
  ├─ 电平变化时: 记录按下/释放时刻
  └─ 释放时判断时长:
      ├─ < SHORT_PRESS_MS → BTN_SHORT_PRESS
      └─ ≥ SHORT_PRESS_MS → BTN_LONG_PRESS
  ↓
getButtonEvent()              [获取按键事件]
  ├─ 短按: 切换 EXIO1 输出 (24V 通/断)
  └─ 长按: 打印预留信息 (TODO: WiFi 发出信号)
  ↓
【最高优先级】检查急停
  ├─ if (emergencyActive == true)
  │   ├─ setAllLEDs(255, 0, 0)     [全红]
  │   ├─ 每秒打印急停持续时间
  │   └─ return                    [跳过所有正常逻辑]
  │
  └─ else  [急停未激活，执行正常流程]
      ├─ handleSerialCommands()    [串口命令轮询]
      │   ├─ 读取用户输入
      │   └─ processCommand() 解析:
      │       ├─ help / ? / status / test / cantest / canstatus
      │       ├─ clear / 0~9 改变 currentState
      │       └─ 未知命令 → 提示
      │
      ├─ updateLEDDisplay()       [根据 currentState 更新 WS2812]
      │   └─ 非阻塞 millis() 轮询，各状态独立计时
      │
      └─ 每秒打印当前状态 (showCurrentState)
  ↓
【自动测试】每 10 秒发送 CAN 测试帧 (回环模式)
  └─ sendCanTestFrame()
      └─ 发送 ID=0x100, data=[2] (红色常亮)
          → 回环模式下直接注入 onCanCommand 回调
          → 触发 processCommand("2") → STATE_CHARGING → 全红
  ↓
delay(1)                      [让出 CPU]
```

---

## 3. FreeRTOS 独立任务 — Core 0 上运行

```
┌══════════════════════════════════════════════════════════════┐
║                     Core 0 (PRO_CPU)                         ║
║                                                              ║
║  ┌─────────────────────────────────────────────────────────┐  ║
║  │ 任务 1: WifiStaTask  (优先级 3, 栈 4096)                │  ║
║  │                                                        │  ║
║  │  while(1) {                                            │  ║
║  │    if (WiFi 未连接) {                                  │  ║
║  │      尝试连接, 30次失败后重置 WiFi 模块                  │  ║
║  │    } else {                                            │  ║
║  │      连接成功 → RGB 绿灯亮 1秒                          │  ║
║  │      注册 HTTP 路由:                                    │  ║
║  │        /         → 返回网页控制面板 (HTML+CSS+JS)       │  ║
║  │        /getData  → 返回 8 通道 JSON 状态               │  ║
║  │        /SwitchN  → 翻转第 N 通道输出                    │  ║
║  │        /AllOn    → 全部打开                             │  ║
║  │        /AllOff   → 全部关闭                             │  ║
║  │      server.handleClient()  // 循环处理 HTTP 请求       │  ║
║  │    }                                                    │  ║
║  │    vTaskDelay(10ms)                                     │  ║
║  │  }                                                      │  ║
║  └─────────────────────────────────────────────────────────┘  ║
║                                                              ║
║  ┌─────────────────────────────────────────────────────────┐  ║
║  │ 任务 2: RGBTask  (优先级 2, 栈 4096)                    │  ║
║  │                                                        │  ║
║  │  while(1) {                                            │  ║
║  │    检查 RGB_indicate[0] 队列是否有指示                   │  ║
║  │    有 → 设置 RGB 颜色                                   │  ║
║  │         如果设置了闪烁间隔 → 按间隔亮/灭                  │  ║
║  │         计时到达 → 关闭 RGB, 处理下一条指示              │  ║
║  │    无 → 跳过                                           │  ║
║  │    vTaskDelay(50ms)                                     │  ║
║  │  }                                                      │  ║
║  └─────────────────────────────────────────────────────────┘  ║
║                                                              ║
║  ┌─────────────────────────────────────────────────────────┐  ║
║  │ 任务 3: BuzzerTask  (优先级 2, 栈 4096)                  │  ║
║  │                                                        │  ║
║  │  while(1) {                                            │  ║
║  │    检查 Buzzer_indicate[0] 队列是否有指示                │  ║
║  │    有 → PWM 输出 (频率 1kHz, 占空比 200/255)            │  ║
║  │         如果设置了闪烁间隔 → 按间隔响/停                  │  ║
║  │         计时到达 → 关闭, 处理下一条指示                  │  ║
║  │    无 → 跳过                                           │  ║
║  │    vTaskDelay(50ms)                                     │  ║
║  │  }                                                      │  ║
║  └─────────────────────────────────────────────────────────┘  ║
║                                                              ║
╚══════════════════════════════════════════════════════════════════╝
```

---

## 4. 模块依赖关系 (完整版)

```
                        main.cpp (程序入口 - Core 1)
                         ↙  ↓  ↓  ↓  ↓  ↓  ↓  ↖
          ┌──────────────┐  │  │  │  │  │  │  ┌──────────────────┐
          │  FreeRTOS    │  │  │  │  │  │  │  │   Core 0 任务    │
          │  用户任务      │  │  │  │  │  │  │                  │
          │  (Core 0)     │  │  │  │  │  │  │  WifiStaTask     │
          │              │  │  │  │  │  │  │  RGBTask          │
          │  WS_WIFI     │  │  │  │  │  │  │  BuzzerTask       │
          │  WS_GPIO     │  │  │  │  │  │  │                  │
          └──────────────┘  │  │  │  │  │  │  └──────────────────┘
                            ↓  ↓  ↓  ↓  ↓  ↓
              ┌──────────┬──┬──┬──┬──┬──┬──┬──────────┐
              ↓          ↓       ↓       ↓          ↓
         command_    led_    emergency  system_    can_
         handler   controller             state    handler
         .cpp       .cpp     .cpp      .cpp       .cpp
              ↓          ↓       ↓       ↓          ↓
         button.cpp   WS_Dout.cpp      I2C_Driver.cpp
                                       WS_TCA9554PWR.cpp
```

---

## 5. 数据流向 (完整版)

### 5.1 WS2812 LED 控制流
```
串口输入 "2"
  ↓
handleSerialCommands()        [command_handler.cpp]
  ↓
processCommand("2")
  ↓
currentState = STATE_CHARGING [system_state.cpp]
  ↓
loop() 下一次迭代
  ↓
updateLEDDisplay()            [led_controller.cpp]
  ↓
setAllLEDs(255, 0, 0)
  ↓
pixels.show()                 [WS2812 灯带全红]
```

### 5.2 CAN 通信流
```
【发送】                          【接收】
sendCanMessage(0x100, data, 1)    processCANReceive()
  ↓                                  ↓
回环模式: 直接注入接收回调          twai_receive() 或回环注入
  ↓                                  ↓
onCanCommand(frame)                CanFrame → 分发回调
  ↓                                  ↓
processCommand("2")                onCanCommand()
  ↓                                  ↓
STATE_CHARGING → 全红              处理命令/设色
```

### 5.3 按键 → EXIO 输出流
```
用户按下按键 (GPIO5)
  ↓
updateButton()                 [button.cpp - 轮询]
  ↓
检测到短按
  ↓
getButtonEvent() → BTN_SHORT_PRESS
  ↓
Dout_Open(1) / Dout_Closs(1)   [WS_Dout.cpp]
  ↓
Set_EXIO(PIN, HIGH/LOW)        [WS_TCA9554PWR.cpp]
  ↓
I2C_Write(TCA9554_ADDR, OUTPUT_REG, data)
  ↓
TCA9554PWR 芯片物理输出 24V 通/断
```

### 5.4 WiFi 网页控制流
```
用户浏览器输入 10.7.5.82
  ↓
WifiStaTask 收到 HTTP GET /
  ↓
server.send(200, "text/html", HTML页面)
  ↓
浏览器渲染控制面板 (8通道 + 全开/全关按钮)
  ↓
用户点击 "CH1 Flip Output"
  ↓
AJAX → /Switch1
  ↓
handleSwitch(1) → Dout_Analysis(data, WIFI_Mode_Trigger)
  ↓
Dout_CHx_Toggle(1) → Set_Toggle(1) → I2C 写寄存器
  ↓
浏览器每隔 200ms 轮询 /getData
  ↓
handleGetData() → 返回 JSON: [1,0,1,0,1,0,1,0]
  ↓
JavaScript 更新页面显示
```

---

## 6. 急停优先级

```
硬件中断 (CHANGE 触发)
  ↓
handleEmergencyInterrupt()     [emergency.cpp]
  ↓
读取 EMERGENCY_PIN 电平
  ├─ LOW  → emergencyActive = true
  └─ HIGH → emergencyActive = false
  ↓
┌─────────────────────────────────────────┐
│ loop() 每次迭代【最先检查】              │
│                                         │
│ if (emergencyActive) {                  │
│   setAllLEDs(255, 0, 0)  ← 全红覆盖     │
│   打印持续时间                           │
│   return;                ← 跳过一切      │
│ }                                       │
│                                         │
│ // 急停未激活才执行下面                   │
│ handleSerialCommands();                  │
│ updateLEDDisplay();                      │
│ ...                                      │
└─────────────────────────────────────────┘

注意: 急停全红 优先于所有动画
      即使当前是彩虹/呼吸效果也会被立即覆盖
```

---

## 7. LED 状态机

```
currentState 枚举值:

  值  名称                效果                说明
 ─────────────────────────────────────────────────────
  0   STATE_IDLE          全灭                默认状态
  1   STATE_UNINITIALIZED 彩色走马灯          256色循环滚动
  2   STATE_CHARGING      全红                红色常亮
  3   STATE_CHARGE_COMPLETE 全绿              绿色常亮
  4   STATE_WALKING       全蓝                蓝色常亮
  5   STATE_MOVING        蓝色跑马灯          单蓝灯+拖尾滚动
  6   STATE_STATIONARY    全白                白色常亮
  7   STATE_TEST          颜色测试            7色每800ms切换，循环后自动IDLE
  8   STATE_BREATHING     呼吸效果            绿色呼吸，10秒后自动IDLE
  9   STATE_RAINBOW       彩虹效果            全彩渐变，10秒后自动IDLE

串口命令对应:
  0 / clear → IDLE      1 → UNINITIALIZED    2 → CHARGING
  3 → CHARGE_COMPLETE   4 → WALKING          5 → MOVING
  6 → STATIONARY        7 / test → TEST      8 → BREATHING
  9 → RAINBOW
  help / ?  → 显示帮助
  status    → 显示当前状态
  cantest   → 发送 CAN 测试帧 (回环: 自动触发红色)
  canstatus → 显示 CAN 总线状态
```

---

## 8. 非阻塞动画机制 (millis() 轮询)

```
updateLEDDisplay() 调用时:
  │
  ├─ STATE_UNINITIALIZED (彩色走马灯)
  │   每 100ms: 所有灯顺移一个色相位置
  │
  ├─ STATE_MOVING (蓝色跑马灯)
  │   每 150ms: 亮点前移一位，前后有拖尾
  │
  ├─ STATE_TEST (颜色测试)
  │   每 800ms: 切换到下一种颜色，7种后自动结束
  │
  ├─ STATE_BREATHING (呼吸)
  │   每 20ms: 按正弦规律改变亮度，10秒后自动结束
  │
  └─ STATE_RAINBOW (彩虹)
      每 20ms: 全体色相递增，10秒后自动结束

  其他状态 (全红/绿/蓝/白/灭):
      只调一次 setAllLEDs()，不耗时
```

---

## 9. 硬件资源分配

```
ESP32-S3 引脚分配:

  GPIO1  → WS2812 灯带数据线 (100 颗 LED)
  GPIO2  → CAN TX (TWAI 控制器)
  GPIO3  → CAN RX (TWAI 控制器)
  GPIO4  → 急停开关 (外部中断, INPUT_PULLUP)
  GPIO5  → 按键 (INPUT_PULLUP)
  GPIO17 → UART1 TX (RS485 - 预留)
  GPIO18 → UART1 RX (RS485 - 预留)
  GPIO21 → UART1 方向控制 (预留)
  GPIO38 → 板载 RGB 灯 (neopixelWrite)
  GPIO41 → I2C SCL (TCA9554PWR IO 扩展器)
  GPIO42 → I2C SDA (TCA9554PWR IO 扩展器)
  GPIO46 → 蜂鸣器 (PWM 输出, 1kHz)

  I2C 地址 0x20 → TCA9554PWR (8 路数字输出)
```

---

## 10. 核心架构总结

```
┌────────────────────────────────────────────────────────────┐
│                    ESP32-S3 双核架构                        │
│                                                            │
│  ┌──────────── Core 0 (PRO_CPU) ────────────┐              │
│  │                                           │              │
│  │  FreeRTOS 调度器                           │              │
│  │    ├─ WifiStaTask  (优先级 3)  ← WiFi     │              │
│  │    ├─ RGBTask      (优先级 2)  ← 指示灯    │              │
│  │    └─ BuzzerTask   (优先级 2)  ← 蜂鸣器    │              │
│  │                                           │              │
│  └───────────────────────────────────────────┘              │
│                            ↑ 共享 RAM / 外设                │
│  ┌──────────── Core 1 (APP_CPU) ────────────┐              │
│  │                                           │              │
│  │  Arduino loop()                            │              │
│  │    ├─ CAN 接收轮询                         │              │
│  │    ├─ 按键轮询                             │              │
│  │    ├─ 急停检测 (由中断设置标志)              │              │
│  │    ├─ 串口命令处理                          │              │
│  │    └─ WS2812 LED 动画 (非阻塞 millis)      │              │
│  │                                           │              │
│  └───────────────────────────────────────────┘              │
│                                                            │
│  通信方式:                                                  │
│    ├─ 串口 (115200) → 用户命令输入                          │
│    ├─ I2C (GPIO41/42) → TCA9554PWR 8路输出                 │
│    ├─ CAN (GPIO2/3, 500kbps) → 外部设备通信                 │
│    └─ WiFi (STA, 10.7.5.82) → HTTP 网页控制面板             │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

---

## 附录：从 UNO 到 ESP32 的演进

| 模块 | UNO 时代 | ESP32 时代 | 变化 |
|------|---------|-----------|------|
| 主控芯片 | ATmega328P (16MHz, 2KB RAM) | ESP32-S3 (240MHz, 512KB RAM) | 🔄 换芯片 |
| 串口波特率 | 9600 | 115200 | 🔼 提升 |
| LED 动画 | loop 轮询 (millis) | loop 轮询 (millis) | 保持 |
| 按键检测 | loop 轮询 | loop 轮询 | 保持 |
| 急停 | 硬件中断 | 硬件中断 | 保持 |
| **WiFi 网页控制** | ❌ 不支持 | ✅ FreeRTOS 任务 (Core 0) | 🆕 新增 |
| **CAN 总线** | ❌ 不支持 | ✅ TWAI 控制器 (回环/正常) | 🆕 新增 |
| **I2C 扩展 IO** | ❌ 不支持 | ✅ TCA9554PWR (8路DO) | 🆕 新增 |
| **板载 RGB 指示** | ❌ 不支持 | ✅ FreeRTOS 任务 (Core 0) | 🆕 新增 |
| **蜂鸣器** | ❌ 不支持 | ✅ FreeRTOS 任务 (Core 0) | 🆕 新增 |
| **实时时钟** | ❌ 不支持 | 🚧 WS_Struct.h 预留枚举 | 预留 |
| **RS485** | ❌ 不支持 | 🚧 WS_GPIO.h 预留引脚 | 预留 |
