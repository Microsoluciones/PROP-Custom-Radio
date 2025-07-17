#pragma once
#include <Arduino.h>
#include <ShiftRegister74HC595.h>
#include <DFRobotDFPlayerMini.h>
#include <PCF8574.h>
#include <LiquidCrystal_I2C.h>

#define led_BUILTIN 2 // Define the built-in LED pin

extern const int rfPins[4];
extern const unsigned long LONG_PRESS_MS;
extern QueueHandle_t rfEventQueue;
extern QueueHandle_t mainTaskQueue;
extern ShiftRegister74HC595<2> sr;
extern HardwareSerial myDFPlayerSerial;
extern DFRobotDFPlayerMini myDFPlayer;
extern PCF8574 pcf;
extern LiquidCrystal_I2C lcd;

enum RfEventType { SHORT_PRESS, LONG_PRESS };
struct RfEvent {
  uint8_t channel;
  RfEventType type;
};
struct MainTaskMsg {
  uint8_t channel;
  RfEventType type;
};

extern volatile unsigned long pressStart[4];
extern volatile bool pressed[4];

// shift register outputs assignments.
enum srOutputs {
  ecm_1 = 1,
  ecm_2,
  ecm_3,
  ecm_4,
  k1,
  k2,
  k3,
  k4,
  LED_SETUP_OK, //not available on hardware.
  LED_I2C,
  LED_DFPLAYER,       
  LED_WIFI,            
  LED_PREPARATION_READY,
  LED_QUEST_0,         
  LED_CONSEQUENCE_0,   
  LED_RESTART_PROTOCOL 
};

struct AudioTrack {
    uint8_t trackNum;
    uint16_t durationMs; // in milliseconds
};

const AudioTrack audioTracks[] = {
    {1, 27000}, // Instructions
    {2, 6000},  // 2 lives left
    {3, 9000},  // 1 life left
    {4, 8000},  // 0 lives left
    {5, 11000}, // Mission successful
    {6, 4000},  // Timeout 1
    {7, 6000},  // Timeout 2
    {8, 5000},  // Timeout 3
    {9, 11000}  // Reset audio
};