#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "config.h"
#include "WS_Struct.h"
#include "WS_Dout.h"

// W5500 SPI 引脚定义（参考产品手册）
#define ETH_CS    16
#define ETH_INT   12
#define ETH_RST   39
#define ETH_SCLK  15
#define ETH_MOSI  13
#define ETH_MISO  14

extern char ethIpStr[16];
extern bool ETH_Connected;

void ETH_Init();
void EthServerTask(void *parameter);

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
