#pragma once
#include <Arduino.h>
esp_err_t gpio_declarations(void);
void blinkBuiltinLED(int times, int delayMs = 200);
void rainbowTest(int cycles, int delayMs);
void playAudioInterrupt(uint8_t trackIdx);