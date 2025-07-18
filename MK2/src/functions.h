#pragma once
#include <Arduino.h>
esp_err_t gpio_declarations(void);
void blinkBuiltinLED(int times, int delayMs = 200);
void rainbowTest(int cycles, int delayMs);
void playAudioInterrupt(uint8_t trackIdx);


void parpadear_backlight(void);
void scrollText(int row, String message, int delayTime, int lcdColumns);
void clearQueue(QueueHandle_t queue);