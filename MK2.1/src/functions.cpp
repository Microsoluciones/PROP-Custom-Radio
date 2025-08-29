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
  pinMode(POT_LATITUDE, INPUT);
  pinMode(POT_LONGITUDE, INPUT);
  pinMode(SYSTEM_SWITCH, INPUT_PULLUP);
  pinMode(SOS_BUTTON, INPUT_PULLUP);
  pinMode(CABLE_CIRCUIT, INPUT_PULLUP);
  return ESP_OK;
}
void playAudioInterrupt(uint8_t trackNum) {
    myDFPlayer.stop();
    vTaskDelay(100 / portTICK_PERIOD_MS); // Ensure stop
    myDFPlayer.play(trackNum); // Direct track number (1-based)
}
void playTrack(const TrackInfo& track, bool doDelay) {
    // Set volume only if different
    myDFPlayer.stop();
    if (track.volume != currentDFPlayerVolume) {
        myDFPlayer.volume(track.volume);
        currentDFPlayerVolume = track.volume;
    }
    myDFPlayer.play(track.trackNumber);
    if (doDelay) {
        vTaskDelay(track.durationMs / portTICK_PERIOD_MS);
    }
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
void lcdPrintCentered(LiquidCrystal_I2C& lcd, const char* line1, const char* line2, const char* line3, const char* line4) {
    const int width = 20;
    const char* lines[4] = {line1, line2, line3, line4};
    lcd.clear();
    for (int i = 0; i < 4; ++i) {
        if (lines[i][0] != '\0') {
            int len = strlen(lines[i]);
            int pad = (width - len) / 2;
            lcd.setCursor(pad > 0 ? pad : 0, i);
            lcd.print(lines[i]);
        }
    }
}

void lcdStepSuccessAnimation() {
	lcd.clear();
	lcd.backlight();
	const char* animChar = "\xFF"; // Full block character
	for (int col = 0; col < 20; ++col) {
		lcd.setCursor(col, 1); // Use line 2 (centered visually)
		lcd.print(animChar);
        lcd.setCursor(col, 2);
        lcd.print(animChar);
		delay(20);
	}
	for (int col = 0; col < 20; ++col) {
		lcd.setCursor(col, 1);
		lcd.print(" ");
        lcd.setCursor(col, 2);
		lcd.print(" ");
		delay(10);
	}
}
