#include "globals.h"
#include "isr.h"
#include "functions.h"

esp_err_t gpio_declarations(void) {
  for (int i = 0; i < 4; i++) {
    pinMode(rfPins[i], INPUT);
  }
  attachInterrupt(digitalPinToInterrupt(rfPins[0]), rf_isr0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(rfPins[1]), rf_isr1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(rfPins[2]), rf_isr2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(rfPins[3]), rf_isr3, CHANGE);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_I2C, OUTPUT);
  pinMode(LED_DFPLAYER, OUTPUT);
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_PREPARATION_READY, OUTPUT);
  pinMode(LED_QUEST_ACTIVE, OUTPUT);
  pinMode(LED_CONSEQUENCE_ACTIVE, OUTPUT);
  pinMode(LED_RESTART_PROTOCOL, OUTPUT);
  pinMode(boton, INPUT_PULLUP);
  pinMode(circuito_llaves, INPUT_PULLUP);
  pinMode(llave_sistema, INPUT_PULLUP);
  pinMode(potenciometro_latitud, INPUT);
  pinMode(potenciometro_longitud, INPUT);
  return ESP_OK;
}
void playTrack(const TrackInfo& track, bool doDelay) {
    // Set volume only if different
    if (track.volume != currentDFPlayerVolume) {
        myDFPlayer.volume(track.volume);
        currentDFPlayerVolume = track.volume;
    }
    myDFPlayer.play(track.trackNumber);
    if (doDelay) {
        vTaskDelay(track.durationMs / portTICK_PERIOD_MS);
    }
}
void playAudioInterrupt(uint8_t trackNum) {
    myDFPlayer.stop();
    vTaskDelay(100 / portTICK_PERIOD_MS); // Ensure stop
    myDFPlayer.play(trackNum); // Direct track number (1-based)
}
// Queue management functions

void flushGameCommandQueue() {
    MainTaskMsg dummyMsg;
    while (uxQueueMessagesWaiting(gameCommandQueue) > 0) {
        xQueueReceive(gameCommandQueue, &dummyMsg, 0);
    }
}

// Helper: Get next valid and fresh message for a phase, skipping unwanted/old ones
bool getRfValidMessage(MainTaskMsg* outMsg, bool (*isValid)(const MainTaskMsg&)) {
    TickType_t startTick = xTaskGetTickCount();
    while (1) {
        MainTaskMsg msg;
        if (xQueueReceive(gameCommandQueue, &msg, 0) == pdTRUE) {
            uint32_t now = millis();
            if ((now - msg.timestamp) > MSG_MAX_AGE_MS) {
                Serial.println("Skipped old message in queue");
                continue;
            }
            if (isValid && !isValid(msg)) {
                Serial.println("Skipped unwanted message in queue");
                continue;
            }
            *outMsg = msg;
            return true;
        } else {
            vTaskDelay(10 / portTICK_PERIOD_MS);
            if ((xTaskGetTickCount() - startTick) * portTICK_PERIOD_MS > MSG_MAX_AGE_MS) {
                return false;
            }
        }
    }
}
// TODO: Add project-specific functions here
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
