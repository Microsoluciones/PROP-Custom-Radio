#pragma once
#include <Arduino.h>
#include <ShiftRegister74HC595.h>
#include <DFRobotDFPlayerMini.h>
#include <PCF8574.h>
#include <LiquidCrystal_I2C.h> // LCD 20x4 display (I2C, PCF8574 adapter, address 0x27)

// Usage: lcd.setCursor(col, row); lcd.print("text");
extern LiquidCrystal_I2C lcd;

extern const int rfPins[4];
extern const unsigned long LONG_PRESS_MS;
extern QueueHandle_t rfEventQueue;
extern QueueHandle_t gameCommandQueue;
extern ShiftRegister74HC595<2> sr;
extern HardwareSerial myDFPlayerSerial;
extern int currentDFPlayerVolume; // Track last set volume
extern DFRobotDFPlayerMini myDFPlayer;
extern PCF8574 pcf; // I2C I/O expander - available for project-specific use

#define led_BUILTIN 2 // Define the built-in LED pin
#define buzzer 2
/*----------Digital GPIOs pins----------*/
#define digital_GPIO1 13
#define digital_GPIO2 14
#define digital_GPIO3 27
#define digital_GPIO4 26
#define digital_GPIO5 33
#define digital_GPIO6 32
/*----------Analog Input pins----------*/
#define analog_input1 34
#define analog_input2 35
#define analog_input3 36
#define analog_input4 39

enum RfEventType { SHORT_PRESS, LONG_PRESS };
struct RfEvent {
  uint8_t channel;
  RfEventType type;
};

struct MainTaskMsg {
  uint8_t channel;
  RfEventType type;
  uint32_t timestamp; // ms since boot
};

// Message max age in ms (for filtering old messages)
constexpr uint32_t MSG_MAX_AGE_MS = 2000;

extern volatile unsigned long pressStart[4];
extern volatile bool pressed[4];

// Global variables for game state
extern bool systemReady;
extern TaskHandle_t mainTaskHandle;
extern TaskHandle_t preparationTaskHandle;
extern TaskHandle_t questTaskHandle;
extern TaskHandle_t consequenceTaskHandle;

// Game state enumeration
typedef enum {
    STATE_IDLE,
    STATE_PREPARATION,
    STATE_QUEST,
    STATE_CONSEQUENCE
} GameState;

extern GameState currentGameState;
extern bool emergencyRestart;
extern bool taskCompleted;

// Shift register outputs assignments - customize for each project
enum srOutputs {
  // TODO: Define project-specific outputs
  // Example outputs (customize as needed):
  OUTPUT_1 = 1,
  OUTPUT_2,
  OUTPUT_3,
  OUTPUT_4,
  RELAY_1,
  RELAY_2,
  RELAY_3,
  RELAY_4,
  LED_SETUP_OK,
  LED_I2C,
  LED_DFPLAYER,       
  LED_WIFI,            
  LED_PREPARATION_READY,
  LED_QUEST_ACTIVE,         
  LED_CONSEQUENCE_ACTIVE,   
  LED_RESTART_PROTOCOL 
};
// Track info for DFPlayer

struct TrackInfo {
  uint8_t trackNumber;   // Track number/order in SD
  uint32_t durationMs;   // Duration in milliseconds
  uint8_t volume;        // Volume (0-20)
};

// Example track definitions for DFPlayer usage
// Track number (zero-based, matches SD file name e.g. 0000.mp3), duration (ms), volume (0-20)
extern const TrackInfo exampleTracks[];
// Function to detect broken lasers (saved for projects that need laser detection)
// Note: PCF8574 I2C bus is available for project-specific I/O expansion
struct LaserStatus {
    bool working[8]; // Adjust size as needed
    uint8_t totalLasers;
};


// === Project-specific global variables and structures ===

// Indicates if the quest was successful (true = win, false = lose)
extern bool questSuccess;

//----------Quest Variables----------//
#define boton digital_GPIO1 // acts as a button
#define circuito_llaves digital_GPIO2 // acts as a button
#define llave_sistema digital_GPIO3 // acts as a button
#define potenciometro_latitud analog_input1
#define potenciometro_longitud analog_input2

// TODO: Add more project-specific global variables and structures here