#include "globals.h"
#include "isr.h"
#include "functions.h"

void rainbowTest(int cycles, int delayMs) {
    for (int c = 0; c < cycles; ++c) {
        for (int i = 0; i < 16; ++i) { // 16 outputs for 2x 74HC595
            sr.setAllLow();
            sr.set(i, HIGH);
            vTaskDelay(delayMs / portTICK_PERIOD_MS);
        }
    }
    sr.setAllLow();
}

void blinkBuiltinLED(int times, int delayMs) {
    for (int i = 0; i < times; ++i) {
        digitalWrite(LED_BUILTIN, HIGH);
        vTaskDelay(delayMs / portTICK_PERIOD_MS);
        digitalWrite(LED_BUILTIN, LOW);
        vTaskDelay(delayMs / portTICK_PERIOD_MS);
    }
}

esp_err_t gpio_declarations(void) {
  for (int i = 0; i < 4; i++) {
    pinMode(rfPins[i], INPUT);
  }
  attachInterrupt(digitalPinToInterrupt(rfPins[0]), rf_isr0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(rfPins[1]), rf_isr1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(rfPins[2]), rf_isr2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(rfPins[3]), rf_isr3, CHANGE);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(buzzer, OUTPUT);

  /*----------custom declarations----------*/
  pinMode(digital_GPIO1, INPUT_PULLUP);
  pinMode(digital_GPIO2, INPUT_PULLUP);
  pinMode(digital_GPIO3, INPUT_PULLUP);
  pinMode(digital_GPIO4, INPUT_PULLUP);
  pinMode(digital_GPIO5, INPUT_PULLUP);
  pinMode(digital_GPIO6, INPUT_PULLUP);
  return ESP_OK;
}

void playAudioInterrupt(uint8_t trackIdx) {
    myDFPlayer.stop();
    vTaskDelay(100 / portTICK_PERIOD_MS); // Ensure stop
    myDFPlayer.play(audioTracks[trackIdx].trackNum);
}

void parpadear_backlight(void){
  lcd.noBacklight();
  delay(100);
  lcd.backlight();
  delay(100);
  lcd.noBacklight();
  delay(100);
  lcd.backlight();
  delay(100);
}