# UNO_LEDbands 程序流程图

## 1. 启动流程 (setup)

```
程序启动
  ↓
Serial.begin(9600)  [串口初始化]
  ↓
initEmergency()  [急停初始化]
  ├─ pinMode(EMERGENCY_PIN, INPUT_PULLUP)
  └─ attachInterrupt(..., handleEmergencyInterrupt, CHANGE)
  ↓
initLEDs()  [LED初始化]
  ├─ pixels.begin()
  ├─ pixels.setBrightness(20)
  ├─ pixels.clear()
  └─ pixels.show()
  ↓
delay(500)
  ↓
打印启动信息
  ↓
showCurrentState()  [显示当前状态]
  ↓
等待主循环
```

---

## 2. 主循环流程 (loop)

```
loop() 每次都执行
  ↓
读取当前时间: currentTime = millis()
  ↓
【最高优先级】检查急停状态
  ├─ if (emergencyActive)
  │   ├─ setAllLEDs(255, 0, 0)  [全红]
  │   ├─ 每秒打印急停持续时间
  │   └─ return  [直接返回，跳过正常逻辑]
  │
  └─ else  [急停未激活，执行正常流程]
      ├─ handleSerialCommands()  [读串口命令]
      │   ├─ 读取用户输入
      │   └─ processCommand()  [解析命令]
      │       ├─ help / status / test / clear / 0-9
      │       └─ 改变 currentState
      │
      ├─ updateLEDDisplay()  [根据状态更新灯光]
      │   ├─ switch(currentState)
      │   ├─ STATE_IDLE: 不做变化
      │   ├─ STATE_UNINITIALIZED: 彩色走马灯
      │   ├─ STATE_CHARGING: 全红
      │   ├─ STATE_CHARGE_COMPLETE: 全绿
      │   ├─ STATE_WALKING: 全蓝
      │   ├─ STATE_MOVING: 蓝色跑马灯
      │   ├─ STATE_STATIONARY: 全白
      │   ├─ STATE_TEST: 依次显示7种颜色
      │   ├─ STATE_BREATHING: 绿光呼吸
      │   └─ STATE_RAINBOW: 彩虹效果
      │
      └─ 每秒打印当前状态
          └─ showCurrentState()
  ↓
delay(1)  [等待1ms]
  ↓
重复下一次循环
```

---

## 3. 模块依赖关系

```
            main.cpp (程序入口)
             ↓ ↓ ↓ ↓ ↓ ↓
    ┌────────┴─┴─┴─┴─┴─┴─────────┐
    ↓        ↓        ↓        ↓
command_   led_      emergency system_
handler   controller             state
.cpp      .cpp      .cpp      .cpp
 ↓        ↓        ↓        ↓
[从.h中]  [从.h中]  [从.h中]  [定义变量]
```

---

## 4. 数据流向

```
用户输入 (串口) 
  ↓
handleSerialCommands()  [command_handler.cpp]
  ↓
processCommand()
  ↓
改变 currentState  [system_state.cpp 中定义]
  ↓
loop() 下一次迭代时
  ↓
updateLEDDisplay()  [led_controller.cpp]
  ↓
读取 currentState
  ↓
根据状态执行动画
  ↓
通过 setAllLEDs() 或 pixels.setPixelColor() 操作灯光
  ↓
pixels.show()  [显示在灯带上]
```

---

## 5. 急停优先级

```
loop()
  ↓
【第一步】检查 emergencyActive (硬件中断维护)
  ├─ YES: 立即全红 + 打印状态 + return (跳过所有正常逻辑)
  │  (这就是为什么急停是最高优先级)
  │
  └─ NO: 执行正常流程 (命令处理 + LED 更新)
```

---

## 6. 硬件中断触发逻辑

```
用户按下急停开关 (EMERGENCY_PIN 从 HIGH 变 LOW)
  ↓
中断触发: handleEmergencyInterrupt()  [emergency.cpp]
  ↓
读取 EMERGENCY_PIN 当前电平
  ├─ LOW (按下): emergencyActive = true
  └─ HIGH (释放): emergencyActive = false
  ↓
loop() 下一次迭代时
  ↓
检查 emergencyActive
  ├─ true: 全红 + 打印
  └─ false: 回到正常模式
```

---

## 7. LED 状态机

```
currentState 可能的值:
  ├─ STATE_IDLE (0)
  ├─ STATE_UNINITIALIZED (1)  → 彩色走马灯
  ├─ STATE_CHARGING (2)        → 全红
  ├─ STATE_CHARGE_COMPLETE (3) → 全绿
  ├─ STATE_WALKING (4)         → 全蓝
  ├─ STATE_MOVING (5)          → 蓝色跑马灯
  ├─ STATE_STATIONARY (6)      → 全白
  ├─ STATE_TEST (7)            → 7种颜色轮流
  ├─ STATE_BREATHING (8)       → 绿光呼吸 (10秒自动结束)
  └─ STATE_RAINBOW (9)         → 彩虹效果 (10秒自动结束)

命令对应关系:
  0 / clear   → STATE_IDLE
  1           → STATE_UNINITIALIZED
  2           → STATE_CHARGING
  3           → STATE_CHARGE_COMPLETE
  4           → STATE_WALKING
  5           → STATE_MOVING
  6           → STATE_STATIONARY
  7           → STATE_TEST
  8           → STATE_BREATHING
  9           → STATE_RAINBOW
```

---

## 8. 文件间的调用关系

```
main.cpp
  ├─ 调用 initEmergency()          [from emergency.h]
  ├─ 调用 initLEDs()               [from led_controller.h]
  ├─ 调用 handleSerialCommands()   [from command_handler.h]
  ├─ 调用 updateLEDDisplay()       [from led_controller.h]
  ├─ 调用 showCurrentState()       [from command_handler.h]
  ├─ 调用 showEmergencyStatus()    [from emergency.h]
  └─ 使用 emergencyActive          [from system_state.h]
     等全局变量

command_handler.cpp
  ├─ 调用 setAllLEDs()             [from led_controller.h]
  ├─ 调用 printStateName()         [自身函数]
  ├─ 修改 currentState             [from system_state.h]
  └─ 使用 emergencyActive          [检查是否中断]

led_controller.cpp
  ├─ 定义 pixels 对象              [Adafruit_NeoPixel]
  ├─ 读取 currentState             [from system_state.h]
  ├─ 使用各种计时变量              [testStepTime, rainbowHue等]
  └─ 控制 WS2812 灯带

emergency.cpp
  ├─ 修改 emergencyActive          [from system_state.h]
  └─ 使用 emergencyStartTime       [计算持续时间]

system_state.cpp
  └─ 定义所有全局共享变量
```

---

## 9. 非阻塞动画运作机制

```
updateLEDDisplay() 每次被调用时:
  ├─ 检查时间间隔 currentTime - lastLEDUpdate > XXX
  │   (不是用 delay() 等待，而是比较 millis() 值)
  │
  ├─ 若满足条件:
  │   ├─ 更新 lastLEDUpdate = currentTime
  │   ├─ 推进动画状态 (例如 rainbowHue += 256)
  │   └─ pixels.show() 立即显示
  │
  └─ 若不满足条件:
      └─ 什么都不做，直接跳过 (保持上一帧画面)

好处: loop() 可以继续高频率执行，随时检查急停和串口
```

---

## 总结

这个程序采用"**状态机 + 事件驱动 + 非阻塞计时**"的设计：

1. **状态机**: 通过 `currentState` 管理 LED 模式
2. **事件驱动**: 
   - 串口输入改变状态
   - 硬件中断立即触发急停
3. **非阻塞**: 用 `millis()` 代替 `delay()`，保持系统实时响应

这种设计特别适合后续加 CAN、多传感器等功能。
