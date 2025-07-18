#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "globals.h"

LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
    Serial.begin(115200);
    
    // Initialize I2C and LCD
    Wire.begin(21, 22); // SDA, SCL pins
    lcd.init();
    lcd.backlight();
    lcd.clear();
    
    // Initialize potentiometer pins
    pinMode(potenciometro_latitud, INPUT);
    pinMode(potenciometro_longitud, INPUT);
    
    // Display header
    lcd.setCursor(0, 0); lcd.print("Potentiometer Values");
    lcd.setCursor(0, 1); lcd.print("Latitud:");
    lcd.setCursor(0, 2); lcd.print("Longitud:");
    lcd.setCursor(0, 3); lcd.print("Press to set target");
    
    Serial.println("Potentiometer Calibration Started");
    Serial.println("Latitud\tLongitud");
}

void loop() {
    // Read both potentiometers
    int latitud = analogRead(potenciometro_latitud);
    int longitud = analogRead(potenciometro_longitud);
    
    // Display on LCD
    lcd.setCursor(9, 1);  // Position after "Latitud:"
    lcd.print("    ");    // Clear old value
    lcd.setCursor(9, 1);
    lcd.print(latitud);
    
    lcd.setCursor(10, 2); // Position after "Longitud:"
    lcd.print("    ");    // Clear old value
    lcd.setCursor(10, 2);
    lcd.print(longitud);
    
    // Display on Serial Monitor
    Serial.print(latitud);
    Serial.print("\t");
    Serial.println(longitud);
    
    // Check if both are in your current target range
    bool latitudOK = (latitud > 1000 && latitud < 2000);
    bool longitudOK = (longitud > 1500 && longitud < 2500);
    
    // Show status
    lcd.setCursor(15, 1);
    if (latitudOK) {
        lcd.print("OK");
    } else {
        lcd.print("--");
    }
    
    lcd.setCursor(15, 2);
    if (longitudOK) {
        lcd.print("OK");
    } else {
        lcd.print("--");
    }
    
    // Overall status
    lcd.setCursor(0, 3);
    if (latitudOK && longitudOK) {
        lcd.print("TARGET REACHED!     ");
    } else {
        lcd.print("Adjust potentiometers");
    }
    
    delay(100); // Update every 100ms
}