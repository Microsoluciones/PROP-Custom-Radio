void lcdStepSuccessAnimation();
#pragma once
#include <Arduino.h>
esp_err_t gpio_declarations(void);
void flushGameCommandQueue();
// Helper: Get next valid and fresh message for a phase, skipping unwanted/old ones
bool getRfValidMessage(MainTaskMsg* outMsg, bool (*isValid)(const MainTaskMsg&));
// Play a DFPlayer track with volume and optional blocking delay
void playTrack(const TrackInfo& track, bool doDelay);
void playAudioInterrupt(uint8_t trackIdx);
// TODO: Add project-specific function declarations here
void lcdPrintCentered(LiquidCrystal_I2C& lcd, const char* line1, const char* line2, const char* line3, const char* line4);
void lcdStepSuccessAnimation();