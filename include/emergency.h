#pragma once


#include <Arduino.h>
#include "config.h"

void initEmergency();
void handleEmergencyInterrupt();
void showEmergencyStatus(uint32_t currentTime);

