
#pragma once
#include <Arduino.h>
esp_err_t gpio_declarations(void);
void playAudioInterrupt(uint8_t trackIdx);
// Play a DFPlayer track with volume and optional blocking delay
void playTrack(const TrackInfo& track, bool doDelay);
void flushGameCommandQueue();
// Helper: Get next valid and fresh message for a phase, skipping unwanted/old ones
bool getRfValidMessage(MainTaskMsg* outMsg, bool (*isValid)(const MainTaskMsg&));
// TODO: Add project-specific function declarations here
// Laser detection function (available for projects that need it)
// Note: Requires PCF8574 I2C expander for laser sensor input
void blink_backlight(void);