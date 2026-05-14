#pragma once

#include "stdio.h"
#include <stdint.h>
#include <WiFi.h>
#include <WebServer.h> 
#include <WiFiClient.h>
#include <WiFiAP.h>
#include "WS_GPIO.h"
#include "WS_Struct.h"
#include "WS_Dout.h"

// 连接WIFI的账号密码
#define STASSID       "SENAD_5845"
#define STAPSK        "12345678"

extern char ipStr[16];
void handleRoot();
void handleGetData();
void handleSwitch(uint8_t ledNumber);

void handleSwitch1();
void handleSwitch2();
void handleSwitch3();
void handleSwitch4();
void handleSwitch5();
void handleSwitch6();
void handleSwitch7();
void handleSwitch8();

void WIFI_Init();
void WifiStaTask(void *parameter);
