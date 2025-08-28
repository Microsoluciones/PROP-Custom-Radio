#include "globals.h"
#include "isr.h"
#include "functions.h"
#include "tasks.h"

// --- Global variable definitions ---
const int rfPins[4] = {23, 4, 15, 25};
const unsigned long LONG_PRESS_MS = 800;
QueueHandle_t rfEventQueue;
QueueHandle_t gameCommandQueue;
ShiftRegister74HC595<2> sr(5, 19, 18);
HardwareSerial myDFPlayerSerial(2);
DFRobotDFPlayerMini myDFPlayer;
PCF8574 pcf(0x20);
LiquidCrystal_I2C lcd(0x27, 20, 4);
volatile unsigned long pressStart[4] = {0, 0, 0, 0};
volatile bool pressed[4] = {false, false, false, false};

// Global variables for game state
bool systemReady = false;
TaskHandle_t mainTaskHandle = NULL;
TaskHandle_t preparationTaskHandle = NULL;
TaskHandle_t questTaskHandle = NULL;
TaskHandle_t consequenceTaskHandle = NULL;
GameState currentGameState = STATE_IDLE;
bool emergencyRestart = false;
bool taskCompleted = false;

void setup() {
  // Initialize LCD

  lcd.init();
  //lcd.backlight();
  lcd.clear();
  //lcd.setCursor(0, 0);
  //lcd.print("System Booting...");

  Serial.begin(115200);
  Serial.println("Setup started");
  sr.setAllLow();

  gpio_declarations();

  Wire.begin(21, 22); // or your actual SDA, SCL pins
  if (!pcf.begin()) {
    Serial.println("PCF8574 not found!");
    while (1);
  } else {
    Serial.println("PCF8574 online.");
    // After successful I2C init
    sr.set(LED_I2C, HIGH);
  }

  myDFPlayerSerial.begin(9600, SERIAL_8N1, 16, 17);
  // Serial.println("DFPlayer Mini test");
  if (!myDFPlayer.begin(myDFPlayerSerial)) {
      Serial.println("Unable to begin DFPlayer Mini:\n"
                     "Check SD card.\n"
                     "check hardware connections.\n");
  } else {
      Serial.println("DFPlayer Mini online.");
      sr.set(LED_DFPLAYER, HIGH);
  }

  rfEventQueue = xQueueCreate(1, sizeof(RfEvent));
  gameCommandQueue = xQueueCreate(5, sizeof(MainTaskMsg));
  
  // Create RF controller task and main coordinator task
  xTaskCreatePinnedToCore(rfControllerTask, "RF Controller", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(mainTask, "Main Task", 4096, NULL, 1, &mainTaskHandle, 1);

  sr.set(LED_SETUP_OK, HIGH);
  Serial.println("Setup complete, main coordinator started.");
}

void loop() {
  // test only
}
