#include "led_controller.h"
#include "system_state.h"

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

void initLEDs() {
  pixels.begin();
  pixels.setBrightness(20);
  pixels.clear();
  pixels.show();
  ledInitialized = true;
}

void updateLEDDisplay(uint32_t currentTime, uint32_t* lastLEDUpdate) {
  if (!ledInitialized) return;

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

void setAllLEDs(uint8_t r, uint8_t g, uint8_t b) {
  for(uint8_t i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();
}

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
