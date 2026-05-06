#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

extern Adafruit_NeoPixel pixels;

void initLEDs();
void updateLEDDisplay(uint32_t currentTime, uint32_t* lastLEDUpdate);
void setAllLEDs(uint8_t r, uint8_t g, uint8_t b);
uint32_t Wheel(uint8_t WheelPos);

#endif // LED_CONTROLLER_H
