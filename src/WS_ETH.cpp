/**
 * WS_ETH.cpp - 以太网驱动 (ESP32-S3 + W5500 SPI Ethernet)
 *
 * 适用于 ESP32-S3-POE-ETH-8DI-8DO (微雪 Waveshare)
 *   板载 W5500 以太网芯片，通过 SPI 接口连接
 *
 * 引脚定义（参考产品手册）：
 *   ETH_CS    = GPIO16 (SPI 片选)
 *   ETH_INT   = GPIO12 (中断)
 *   ETH_RST   = GPIO39 (复位)
 *   ETH_SCLK  = GPIO15 (SPI 时钟)
 *   ETH_MOSI  = GPIO13 (SPI MOSI)
 *   ETH_MISO  = GPIO14 (SPI MISO)
 */

#include "WS_ETH.h"

// ESP-IDF 以太网相关头文件
#include "esp_eth.h"
#include "esp_eth_mac.h"
#include "esp_eth_phy.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "driver/spi_master.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

char ethIpStr[16];
WebServer ethServer(80);
bool ETH_Connected = false;

// W5500 MAC 地址
static uint8_t eth_mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// 以太网句柄
static esp_eth_handle_t eth_handle = NULL;

// ===== 以太网事件回调（ESP-IDF 级别） =====
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data) {
  uint8_t mac_addr[6];
  esp_eth_handle_t handle = (esp_eth_handle_t)event_data;

  switch (event_id) {
    case ETHERNET_EVENT_START:
      Serial.println(F("[ETH] 以太网启动"));
      break;

    case ETHERNET_EVENT_CONNECTED:
      Serial.println(F("[ETH] ⚡ 网线已连接 (Link Up)"));
      break;

    case ETHERNET_EVENT_DISCONNECTED:
      Serial.println(F("[ETH] ⚡ 网线断开 (Link Down)"));
      ETH_Connected = false;
      neopixelWrite(GPIO_PIN_RGB, 255, 0, 0);  // 红色
      break;

    case ETHERNET_EVENT_STOP:
      Serial.println(F("[ETH] 以太网停止"));
      ETH_Connected = false;
      neopixelWrite(GPIO_PIN_RGB, 255, 0, 0);  // 红色
      break;

    default:
      break;
  }
}

// ===== IP 事件回调 =====
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data) {
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  const esp_netif_ip_info_t *ip_info = &event->ip_info;

  ETH_Connected = true;

  // ★★★★★ 醒目打印 IP ★★★★★
  Serial.println();
  Serial.println(F("╔══════════════════════════════════════╗"));
  Serial.println(F("║        ✅ 以太网连接成功！           ║"));
  Serial.println(F("╠══════════════════════════════════════╣"));
  Serial.print(F("║  IP 地址:  "));
  Serial.printf(IPSTR, IP2STR(&ip_info->ip));
  Serial.println();
  Serial.println(F("║  浏览器打开上面地址访问控制页面       ║"));
  Serial.println(F("╚══════════════════════════════════════╝"));
  Serial.println();
  Serial.flush();

  sprintf(ethIpStr, IPSTR, IP2STR(&ip_info->ip));

  // ===== 连接成功 → RGB 常亮绿色 =====
  neopixelWrite(GPIO_PIN_RGB, 0, 255, 0);  // GRB顺序: G=255, R=0, B=0
}

// ========== Web 页面 ==========
void handleRoot() {
  String myhtmlPage =
    String("") +
    "<html>"+
    "<head>"+
    "    <meta charset=\"utf-8\">"+
    "    <title>ESP32-S3-POE-ETH-8DI-8DO</title>"+
    "    <style>" +
    "        body {" +
    "            font-family: Arial, sans-serif;" +
    "            background-color: #f0f0f0;" +
    "            margin: 0;" +
    "            padding: 0;" +
    "        }" +
    "        .header {" +
    "            text-align: center;" +
    "            padding: 20px 0;" +
    "            background-color: #333;" +
    "            color: #fff;" +
    "            margin-bottom: 20px;" +
    "        }" +
    "        .container {" +
    "            max-width: 600px;" +
    "            margin: 0 auto;" +
    "            padding: 20px;" +
    "            background-color: #fff;" +
    "            border-radius: 5px;" +
    "            box-shadow: 0 0 5px rgba(0, 0, 0, 0.3);" +
    "        }" +
    "        .input-container {" +
    "            display: flex;" +
    "            align-items: center;" +
    "            margin-bottom: 10px;" +
    "        }" +
    "        .input-container label {" +
    "            width: 80px;" + 
    "            margin-right: 10px;" +
    "        }" +
    "        .input-container input[type=\"text\"] {" +
    "            flex: 1;" +
    "            padding: 5px;" +
    "            border: 1px solid #ccc;" +
    "            border-radius: 3px;" +
    "            margin-right: 10px; "+ 
    "        }" +
    "        .input-container button {" +
    "            padding: 5px 10px;" +
    "            background-color: #333;" +
    "            color: #fff;" +
    "            font-size: 14px;" +
    "            font-weight: bold;" +
    "            border: none;" +
    "            border-radius: 3px;" +
    "            text-transform: uppercase;" +
    "            cursor: pointer;" +
    "        }" +
    "        .button-container {" +
    "            margin-top: 20px;" +
    "            text-align: center;" +
    "        }" +
    "        .button-container button {" +
    "            margin: 0 5px;" +
    "            padding: 10px 15px;" +
    "            background-color: #333;" +
    "            color: #fff;" +
    "            font-size: 14px;" +
    "            font-weight: bold;" +
    "            border: none;" +
    "            border-radius: 3px;" +
    "            text-transform: uppercase;" +
    "            cursor: pointer;" +
    "        }" +
    "        .button-container button:hover {" +
    "            background-color: #555;" +
    "        }" +
    "    </style>" +
    "</head>"+
    "<body>"+
    "    <script defer=\"defer\">"+
    "        function ledSwitch(ledNumber) {"+
    "            var xhttp = new XMLHttpRequest();" +
    "            xhttp.onreadystatechange = function() {" +
    "                if (this.readyState == 4 && this.status == 200) {" +
    "                    console.log('LED ' + ledNumber + ' state changed');" +
    "                }" +
    "            };" +
    "            if (ledNumber < 9 && ledNumber > 0) {" +
    "             xhttp.open('GET', '/Switch' + ledNumber, true);" +
    "            }" +
    "            else if(ledNumber == 9){" +
    "            xhttp.open('GET', '/AllOn', true);" +
    "            }" +
    "            else if(ledNumber == 0){" +
    "            xhttp.open('GET', '/AllOff', true);" +
    "            }" +
    "            xhttp.send();" +
    "        }" +
    "        function updateData() {"
    "            var xhr = new XMLHttpRequest();"
    "            xhr.open('GET', '/getData', true);"
    "            xhr.onreadystatechange = function() {"
    "              if (xhr.readyState === 4 && xhr.status === 200) {"
    "                var dataArray = JSON.parse(xhr.responseText);"
    "                document.getElementById('ch1').value = dataArray[0];"
    "                document.getElementById('ch2').value = dataArray[1];"
    "                document.getElementById('ch3').value = dataArray[2];"
    "                document.getElementById('ch4').value = dataArray[3];"
    "                document.getElementById('ch5').value = dataArray[4];"
    "                document.getElementById('ch6').value = dataArray[5];"
    "                document.getElementById('ch7').value = dataArray[6];"
    "                document.getElementById('ch8').value = dataArray[7];"
    "                document.getElementById('btn1').removeAttribute('disabled');"+
    "                document.getElementById('btn2').removeAttribute('disabled');"+
    "                document.getElementById('btn3').removeAttribute('disabled');"+
    "                document.getElementById('btn4').removeAttribute('disabled');"+
    "                document.getElementById('btn5').removeAttribute('disabled');"+
    "                document.getElementById('btn6').removeAttribute('disabled');"+
    "                document.getElementById('btn7').removeAttribute('disabled');"+
    "                document.getElementById('btn8').removeAttribute('disabled');"+
    "                document.getElementById('btn9').removeAttribute('disabled');"+
    "                document.getElementById('btn0').removeAttribute('disabled');"+
    "              }"+
    "            };"+
    "            xhr.send();"+
    "        }"+
    "        var refreshInterval = 200;"+                                     
    "        setInterval(updateData, refreshInterval);"+       
    "    </script>" +
    "    <div class=\"header\">"+
    "        <h1>ESP32-S3-8DI-8DO (ETH)</h1>"+
    "    </div>"+
    "    <div class=\"container\">"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input1\">CH1 Output</label>"+
    "            <input type=\"text\" id=\"ch1\" />"+
    "            <button value=\"Switch1\" id=\"btn1\" disabled onclick=\"ledSwitch(1)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input2\">CH2 Output</label>"+
    "            <input type=\"text\" id=\"ch2\" />"+
    "            <button value=\"Switch2\" id=\"btn2\" disabled onclick=\"ledSwitch(2)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input3\">CH3 Output</label>"+
    "            <input type=\"text\" id=\"ch3\" />"+
    "            <button value=\"Switch3\" id=\"btn3\" disabled onclick=\"ledSwitch(3)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input4\">CH4 Output</label>"+
    "            <input type=\"text\" id=\"ch4\" />"+
    "            <button value=\"Switch4\" id=\"btn4\" disabled onclick=\"ledSwitch(4)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input5\">CH5 Output</label>"+
    "            <input type=\"text\" id=\"ch5\" />"+
    "            <button value=\"Switch5\" id=\"btn5\" disabled onclick=\"ledSwitch(5)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input6\">CH6 Output</label>"+
    "            <input type=\"text\" id=\"ch6\" />"+
    "            <button value=\"Switch6\" id=\"btn6\" disabled onclick=\"ledSwitch(6)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input7\">CH7 Output</label>"+
    "            <input type=\"text\" id=\"ch7\" />"+
    "            <button value=\"Switch7\" id=\"btn7\" disabled onclick=\"ledSwitch(7)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input8\">CH8 Output</label>"+
    "            <input type=\"text\" id=\"ch8\" />"+
    "            <button value=\"Switch8\" id=\"btn8\" disabled onclick=\"ledSwitch(8)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"button-container\">"+
    "            <button value=\"AllOn\" id=\"btn9\" disabled onclick=\"ledSwitch(9)\">All High</button>"+
    "            <button value=\"AllOff\" id=\"btn0\" disabled onclick=\"ledSwitch(0)\">All Low</button>"+
    "        </div>"+
    "    </div>"+
    "</body>"+
    "</html>";
    
  ethServer.send(200, "text/html", myhtmlPage); 
  Serial.println(F("[ETH] 用户访问了主页"));
}

void handleGetData() {
  String json = "[";
  for (int i = 0; i < sizeof(Dout_Flag) / sizeof(Dout_Flag[0]); i++) {
    json += String(Dout_Flag[i]);
    if (i < sizeof(Dout_Flag) / sizeof(Dout_Flag[0]) - 1) {
      json += ",";
    }
  }
  json += "]";
  ethServer.send(200, "application/json", json);
}

void handleSwitch(uint8_t ledNumber) {
  uint8_t Data[1] = {0};
  Data[0] = ledNumber + 48;
  Dout_Analysis(Data, ETH_Mode_Trigger);
  ethServer.send(200, "text/plain", "OK");
}

void handleSwitch1() { handleSwitch(1); }
void handleSwitch2() { handleSwitch(2); }
void handleSwitch3() { handleSwitch(3); }
void handleSwitch4() { handleSwitch(4); }
void handleSwitch5() { handleSwitch(5); }
void handleSwitch6() { handleSwitch(6); }
void handleSwitch7() { handleSwitch(7); }
void handleSwitch8() { handleSwitch(8); }
void handleSwitch9() { handleSwitch(9); }
void handleSwitch0() { handleSwitch(0); }

// ========== 以太网初始化 (ESP-IDF 原生 API 驱动 W5500) ==========
void ETH_Init() {
  esp_err_t ret;

  Serial.println(F("[ETH] 初始化 W5500 以太网..."));
  Serial.flush();

  // 1. 复位 W5500 芯片
  pinMode(ETH_RST, OUTPUT);
  digitalWrite(ETH_RST, LOW);
  delay(10);
  digitalWrite(ETH_RST, HIGH);
  delay(300);
  Serial.println(F("[ETH] W5500 复位完成"));
  Serial.flush();

  // 2. 创建 SPI 总线配置
  //    W5500 使用 SPI 接口，ESP32-S3 的 SPI2_HOST (HSPI)
  spi_bus_config_t buscfg = {
    .mosi_io_num = ETH_MOSI,      // GPIO13
    .miso_io_num = ETH_MISO,      // GPIO14
    .sclk_io_num = ETH_SCLK,      // GPIO15
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
  };

  ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
  if (ret == ESP_OK) {
    Serial.println(F("[ETH] SPI 总线初始化成功"));
  } else {
    Serial.printf("[ETH] SPI 总线初始化失败: %d\n", ret);
    neopixelWrite(GPIO_PIN_RGB, 255, 0, 0);
    return;
  }
  Serial.flush();

  // 3. 添加 SPI 设备（W5500 从设备）
  spi_device_interface_config_t devcfg = {
    .mode = 0,                    // SPI 模式 0
    .clock_speed_hz = 30 * 1000 * 1000,  // 30MHz
    .spics_io_num = ETH_CS,       // GPIO16 (CS)
    .queue_size = 1,
  };

  spi_device_handle_t spi_handle;
  ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);
  if (ret == ESP_OK) {
    Serial.println(F("[ETH] SPI 设备添加成功"));
  } else {
    Serial.printf("[ETH] SPI 设备添加失败: %d\n", ret);
    neopixelWrite(GPIO_PIN_RGB, 255, 0, 0);
    return;
  }
  Serial.flush();

  // 4. 创建 W5500 MAC 实例
  eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(spi_handle);
  w5500_config.int_gpio_num = ETH_INT;  // GPIO12

  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
  esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);

  // 5. 创建 W5500 PHY 实例
  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
  phy_config.phy_addr = 0;
  phy_config.reset_gpio_num = -1;  // 手动复位
  esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);

  if (mac == NULL || phy == NULL) {
    Serial.println(F("[ETH] MAC/PHY 创建失败!"));
    neopixelWrite(GPIO_PIN_RGB, 255, 0, 0);
    return;
  }
  Serial.println(F("[ETH] MAC/PHY 实例创建成功"));
  Serial.flush();

  // 6. 安装以太网驱动
  esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
  ret = esp_eth_driver_install(&eth_config, &eth_handle);
  if (ret != ESP_OK) {
    Serial.printf("[ETH] 驱动安装失败: %d\n", ret);
    neopixelWrite(GPIO_PIN_RGB, 255, 0, 0);
    return;
  }
  Serial.println(F("[ETH] 驱动安装成功"));
  Serial.flush();

  // 7. 设置 MAC 地址
  esp_eth_ioctl(eth_handle, ETH_CMD_S_MAC_ADDR, eth_mac);

  // 8. 注册以太网事件
  esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL);

  // 9. 启动以太网
  ret = esp_eth_start(eth_handle);
  if (ret != ESP_OK) {
    Serial.printf("[ETH] 启动失败: %d\n", ret);
    neopixelWrite(GPIO_PIN_RGB, 255, 0, 0);
    return;
  }
  Serial.println(F("[ETH] W5500 已启动，等待网线连接..."));
  Serial.println(F("[ETH] 插上网线后，等待 DHCP 获取 IP"));
  Serial.println(F("[ETH] 状态: 网线未连接=红灯 | 连接成功=绿灯"));
  Serial.flush();

  // 10. 注册 IP 获取回调
  esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL);

  // 11. 启动 Web 服务器任务 → 绑定到 Core 1
  //     Core 0 留给 loop() 独占，避免 lwIP 网络中断干扰实时控制
  xTaskCreatePinnedToCore(
    EthServerTask,
    "EthServerTask",
    6144,     // WebServer 需要更多栈空间
    NULL,
    3,
    NULL,
    1         // Core 1（与 loop 分核运行）
  );
}

// ========== Web 服务器任务 (使用原生套接字) ==========
void EthServerTask(void *parameter) {
  while (1) {
    if (ETH_Connected) {
      Serial.print(F("[ETH] Web服务器启动于: http://"));
      Serial.println(ethIpStr);

      ethServer.on("/", handleRoot);
      ethServer.on("/getData", handleGetData);
      ethServer.on("/Switch1", handleSwitch1);
      ethServer.on("/Switch2", handleSwitch2);
      ethServer.on("/Switch3", handleSwitch3);
      ethServer.on("/Switch4", handleSwitch4);
      ethServer.on("/Switch5", handleSwitch5);
      ethServer.on("/Switch6", handleSwitch6);
      ethServer.on("/Switch7", handleSwitch7);
      ethServer.on("/Switch8", handleSwitch8);
      ethServer.on("/AllOn", handleSwitch9);
      ethServer.on("/AllOff", handleSwitch0);

      ethServer.begin();
      Serial.println(F("[ETH] Web服务器已启动"));

      while (ETH_Connected) {
        ethServer.handleClient();
        vTaskDelay(pdMS_TO_TICKS(10));
      }

      Serial.println(F("[ETH] 连接断开，等待重新连接..."));
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  vTaskDelete(NULL);
}
