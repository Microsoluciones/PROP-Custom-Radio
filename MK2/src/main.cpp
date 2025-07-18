#include <Arduino.h>
#include "globals.h"
#include "functions.h"
#include "tasks.h"
#include "isr.h"

// --- Global variable definitions ---
const int rfPins[4] = {15 , 23, 25 , 4};
const unsigned long LONG_PRESS_MS = 800;
QueueHandle_t rfEventQueue;
QueueHandle_t mainTaskQueue;
QueueHandle_t questTaskQueue;
TaskHandle_t consequenceTaskHandle = NULL;
ShiftRegister74HC595<2> sr(5, 19, 18);
HardwareSerial myDFPlayerSerial(2);
DFRobotDFPlayerMini myDFPlayer;
PCF8574 pcf(0x20);
volatile unsigned long pressStart[4] = {0, 0, 0, 0};
volatile bool pressed[4] = {false, false, false, false};
LiquidCrystal_I2C lcd(0x27, 20, 4); // Adjust address and size as needed
bool lastQuestSuccess = false;


int bateria = 15; // Example initial value, adjust as needed

void setup() {
    Serial.begin(115200);
    Serial.println("Setup started");
    sr.setAllLow(); Serial.println("Shift Register cleared");
    gpio_declarations(); Serial.println("GPIOs initialized");
    Wire.begin(21, 22); // or your actual SDA, SCL pins
    // for hat
    /*-----------------------------------------------------------------
    
    if (!pcf.begin()) {
        Serial.println("PCF8574 not found!");
    } else {
        Serial.println("PCF8574 online.");
        // After successful I2C init
        sr.set(LED_I2C, HIGH);
    }
    -----------------------------------------------------------------*/
    // for display
    lcd.init();
    lcd.backlight();
    //-----------------------------------------------------------------
    myDFPlayerSerial.begin(9600, SERIAL_8N1, 16, 17);
    if (!myDFPlayer.begin(myDFPlayerSerial)) {
        Serial.println("Unable to begin DFPlayer Mini:\n"
                        "Check SD card.\n"
                        "check hardware connections.\n");
    } else {
        Serial.println("DFPlayer Mini online.");
        sr.set(LED_DFPLAYER, HIGH);
    }

    rfEventQueue = xQueueCreate(10, sizeof(RfEvent));
    mainTaskQueue = xQueueCreate(10, sizeof(MainTaskMsg));
    questTaskQueue = xQueueCreate(5, sizeof(MainTaskMsg));
    xTaskCreatePinnedToCore(rfControllerTask, "RF Controller", 2048, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(mainTask, "Main Task", 2048, NULL, 1, NULL, 1);
    sr.set(LED_SETUP_OK, HIGH);
    Serial.println("Setup complete, tasks started.");
}

void loop() {
    // Nothing to do in loop
}