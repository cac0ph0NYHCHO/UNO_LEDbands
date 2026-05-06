#ifndef EMERGENCY_H
#define EMERGENCY_H

#include <Arduino.h>
#include "config.h"

void initEmergency();
void handleEmergencyInterrupt();
void showEmergencyStatus(uint32_t currentTime);

#endif // EMERGENCY_H
