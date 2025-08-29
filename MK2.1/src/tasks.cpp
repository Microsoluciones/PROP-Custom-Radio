#include "globals.h"
#include "functions.h"
#include "tasks.h"

/*
Generic Escape Room Board Template
=================================
This template provides a framework for escape room projects using:
- ESP32 with FreeRTOS tasks
- RF remote control (4 buttons: RF1-RF4)
- DFPlayer Mini for audio
- 74HC595 shift register for outputs
- PCF8574 I2C expander (available for project-specific I/O)
- Smart message routing architecture

Task Architecture:
- mainTask: Coordinates game phases and state transitions
- preparationTask: Setup and configuration phase
- questTask: Main game/puzzle phase  
- consequenceTask: End game phase (win/lose/cleanup)
- rfControllerTask: Smart message router based on game state

Key Features:
- State-aware message filtering prevents unwanted button presses
- Individual task queues eliminate race conditions
- Emergency restart via RF4 long press
- Template structure ready for project-specific logic
*/

void rfControllerTask(void *pvParameters) {
    RfEvent event;
    MainTaskMsg msg;
    while (1) {
        if (xQueueReceive(rfEventQueue, &event, portMAX_DELAY) == pdTRUE) {
            if (event.type == LONG_PRESS) {
                Serial.printf("Long press detected on channel %d\n", event.channel + 1);
            } else {
                Serial.printf("Short press detected on channel %d\n", event.channel + 1);
            }
            if (event.channel == 3 && event.type == LONG_PRESS) {
                Serial.println("EMERGENCY RESTART - RF4 long press detected!");
                Serial.println("Killing all tasks for complete system restart...");
                emergencyRestart = true;
                if (mainTaskHandle != NULL) {
                    vTaskDelete(mainTaskHandle);
                    mainTaskHandle = NULL;
                    Serial.println("Main task killed");
                }
                if (preparationTaskHandle != NULL) {
                    vTaskDelete(preparationTaskHandle);
                    preparationTaskHandle = NULL;
                    Serial.println("Preparation task killed");
                }
                if (questTaskHandle != NULL) {
                    vTaskDelete(questTaskHandle);
                    questTaskHandle = NULL;
                    Serial.println("Quest task killed");
                }
                if (consequenceTaskHandle != NULL) {
                    vTaskDelete(consequenceTaskHandle);
                    consequenceTaskHandle = NULL;
                    Serial.println("Consequence task killed");
                }
                Serial.println("Hardware reset to safe state");
                currentGameState = STATE_IDLE;
                systemReady = false;
                taskCompleted = false;
                Serial.println("Global variables reset");
                vTaskDelay(500 / portTICK_PERIOD_MS);
                xTaskCreatePinnedToCore(mainTask, "Main Task", 4096, NULL, 1, &mainTaskHandle, 1);
                Serial.println("Main task restarted - Emergency restart complete!");
                continue;
            }
            // Prepare message with timestamp
            msg.channel = event.channel;
            msg.type = event.type;
            msg.timestamp = millis();
            // Remove old/unused messages if queue is full or contains old messages
            while (uxQueueSpacesAvailable(gameCommandQueue) == 0) {
                MainTaskMsg oldMsg;
                if (xQueueReceive(gameCommandQueue, &oldMsg, 0) == pdTRUE) {
                    uint32_t age = msg.timestamp - oldMsg.timestamp;
                    if (age > MSG_MAX_AGE_MS) {
                        Serial.println("Removed old message from gameCommandQueue");
                    } else {
                        Serial.println("Removed message to make space in gameCommandQueue");
                    }
                } else {
                    break;
                }
            }
            if (xQueueSend(gameCommandQueue, &msg, 0) == pdTRUE) {
                Serial.printf("Message routed to gameCommandQueue: RF%d %s\n", event.channel + 1, event.type == LONG_PRESS ? "long" : "short");
            } else {
                Serial.printf("Failed to route message to gameCommandQueue: RF%d %s\n", event.channel + 1, event.type == LONG_PRESS ? "long" : "short");
            }
        }
    }
}

void mainTask(void *pvParameters) {
    Serial.println("Main task started - Game coordinator");
    
    // Reset emergency flag if it was set
    emergencyRestart = false;
    
    // Safety cleanup: ensure all task handles are NULL if this is a restart
    preparationTaskHandle = NULL;
    questTaskHandle = NULL;
    consequenceTaskHandle = NULL;
    
    // TODO: Initialize all systems to safe/default state (project-specific)
    systemReady = false;
    
    // Ensure we start in IDLE state
    currentGameState = STATE_IDLE;
    
    Serial.println("System fully reset and ready");
    Serial.println("Press RF1 (short press) to start preparation..."); // TODO: Customize start message
    
    MainTaskMsg msg;
    while (1) {
        switch (currentGameState) {
            case STATE_IDLE:
                // Show centered message on LCD (only lines 2 and 3 used)
                lcd.init();
                lcd.backlight();
                lcd.clear();
                lcdPrintCentered(lcd, "", "Pulsacion Corta en A", "Para", "Iniciar Preparacion");
                while (currentGameState == STATE_IDLE) {
                    if (xQueueReceive(gameCommandQueue, &msg, 100 / portTICK_PERIOD_MS) == pdTRUE) {
                        if (msg.channel == 0 && msg.type == SHORT_PRESS) {
                            Serial.println("Starting preparation phase...");
                            flushGameCommandQueue();
                            currentGameState = STATE_PREPARATION;
                        }
                    }
                    // Optionally, add a small delay to avoid busy loop
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                }
                break;
            case STATE_PREPARATION:
                xTaskCreatePinnedToCore(preparationTask, "PreparationTask", 4096, NULL, 1, &preparationTaskHandle, 1);
                while (currentGameState == STATE_PREPARATION) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                Serial.println("Preparation phase completed");
                flushGameCommandQueue();
                break;
            case STATE_QUEST:
                Serial.println("Starting quest phase...");
                xTaskCreatePinnedToCore(questTask, "QuestTask", 4096, NULL, 1, &questTaskHandle, 1);
                while (currentGameState == STATE_QUEST) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                flushGameCommandQueue();
                Serial.println("Quest phase completed");
                break;
            case STATE_CONSEQUENCE:
                Serial.println("Starting consequence phase...");
                xTaskCreatePinnedToCore(consequenceTask, "ConsequenceTask", 4096, NULL, 1, &consequenceTaskHandle, 1);
                while (currentGameState == STATE_CONSEQUENCE) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                flushGameCommandQueue();
                Serial.println("Consequence phase completed");
                break;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void preparationTask(void *pvParameters) {
    Serial.println("Preparation task started");
    for (uint8_t i = 0; i < 8; i++) {
        pcf.write(i, HIGH);
    }
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.println("Preparation phase: Initial setup");
    MainTaskMsg msg;
    // --- STANDARDIZED CONTROLLER USAGE ---
    // - Use controller for configuration as needed (customize below)
    // - RF1 long press ALWAYS ends preparation and starts quest (required for all projects)
    // --------------------------------------
    bool setupComplete = false;
    auto isValid = [](const MainTaskMsg& m) {
        // Accept only RF1 long press to finish preparation
        return (m.channel == 0 && m.type == LONG_PRESS);
    };
    while (!setupComplete) {
        // Read digital inputs (HIGH = not pressed/on)
        int sw1 = digitalRead(SOS_BUTTON);
        int sw2 = digitalRead(CABLE_CIRCUIT);
        int sw3 = digitalRead(SYSTEM_SWITCH);
        // Read potentiometers
        int pot_lat = analogRead(POT_LATITUDE);
        int pot_long = analogRead(POT_LONGITUDE);
        // Check if all digital inputs are HIGH and both pots are outside their valid ranges
        bool pots_outside = (pot_lat < pot_latitude_min || pot_lat > pot_latitude_max) && (pot_long < pot_longitude_min || pot_long > pot_longitude_max);
        bool switches_ok = (sw1 == HIGH && sw2 == HIGH && sw3 == HIGH);
        if (!switches_ok || !pots_outside) {
            lcdPrintCentered(lcd, "", "Ajuste todos los", "controles a cero", "");
        } else {
            lcdPrintCentered(lcd, "", "Listo para iniciar", "Pulsacion Larga en A", "Para continuar");
            // Now allow RF1 long press to finish preparation
            if (getRfValidMessage(&msg, isValid)) {
                Serial.printf("Received RF%d %s in preparation phase\n", msg.channel + 1, msg.type == LONG_PRESS ? "long" : "short");
                Serial.println("Preparation complete - Moving to quest phase!");
                setupComplete = true;
            }
        }
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    flushGameCommandQueue();
    currentGameState = STATE_QUEST;
    vTaskDelete(NULL);
}

void questTask(void *pvParameters) {
    Serial.println("Quest task started - Main game phase");
    Serial.println("Quest phase: Initializing game logic...");
    lcd.clear();
    lcdPrintCentered(lcd, "", "Comienza la mision", "Buena Suerte!", "");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    const int totalSteps = 4;
    int currentStep = 0;
    battery = 15; // battery = minutes left, also percent
    unsigned long lastMinuteTick = millis();
    bool questComplete = false;
    questSuccess = false;
    MainTaskMsg msg;

    auto isValid = [](const MainTaskMsg& m) {
        return ( (m.channel == 1 && m.type == LONG_PRESS) || (m.channel == 2 && m.type == LONG_PRESS) );
    };

    while (!questComplete) {
        // Battery timer logic: only update between steps
        unsigned long now = millis();
        if (now - lastMinuteTick >= 60000 && battery > 0) {
            battery--;
            lastMinuteTick = now;
        }
    int percent = battery; // battery is percent, starts at 15 and drops to 0
    char alertLine[21] = "Alerta: Bateria Baja";
    char battLine[21];
    snprintf(battLine, sizeof(battLine), "Bateria Restante:%2d%%", percent);

        if (battery == 0) {
            lcdPrintCentered(lcd, "", "Bateria agotada", "Mision fallida", "");
            questSuccess = false;
            questComplete = true;
            vTaskDelay(3000 / portTICK_PERIOD_MS);
            break;
        }

        // Step logic for each quest step
        const char* legends1[] = {"Iniciar Sistema", "Configurar Circuito", "Indicar Coordenadas", "Enviar Senal SOS"};
        const char* legends2[] = {"", "Electrico", "Latitud y Longitud", ""};
        lcd.clear();
        lcdPrintCentered(lcd, legends1[currentStep], legends2[currentStep], alertLine, battLine);

        // Wait for step completion, skip, or quest abort
        bool stepDone = false;
        unsigned long lastDisplayUpdate = millis();
        while (!stepDone && !questComplete) {
            unsigned long nowStep = millis();
            // Update battery every minute
            if (nowStep - lastMinuteTick >= 60000 && battery > 0) {
                battery--;
                lastMinuteTick = nowStep;
                percent = battery;
                snprintf(battLine, sizeof(battLine), "Bateria Restante:%2d%%", percent);
                // Always show alert and update bottom lines
                lcd.setCursor(0, 2); lcd.print("                    ");
                lcd.setCursor(0, 2); lcd.print(alertLine);
                lcd.setCursor(0, 3); lcd.print("                    ");
                lcd.setCursor(0, 3); lcd.print(battLine);
            }
            if(battery == 0){
                questComplete = true;
                questSuccess = false;
            }
            if (getRfValidMessage(&msg, isValid)) {
                if (msg.channel == 1 && msg.type == LONG_PRESS) {
                    Serial.printf("Step %d skipped by RF2 long press\n", currentStep+1);
                    stepDone = true;
                } else if (msg.channel == 2 && msg.type == LONG_PRESS) {
                    Serial.println("Quest aborted by RF3 long press");
                    lcdPrintCentered(lcd, "", "Mision abortada", "Mision fallida", "");
                    questSuccess = false;
                    questComplete = true;
                    vTaskDelay(2000 / portTICK_PERIOD_MS);
                    break;
                }
            }
            // Step completion conditions
            switch (currentStep) {
                case 0: // System Switch to GND
                    if (digitalRead(SYSTEM_SWITCH) == LOW) stepDone = true;
                    break;
                case 1: // Cable Circuit to GND
                    if (digitalRead(CABLE_CIRCUIT) == LOW) stepDone = true;
                    break;
                case 2: { // Potentiometers in range
                    int lat = analogRead(POT_LATITUDE);
                    int lon = analogRead(POT_LONGITUDE);
                    if (lat >= pot_latitude_min && lat <= pot_latitude_max && lon >= pot_longitude_min && lon <= pot_longitude_max) stepDone = true;
                    break;
                }
                case 3: // SOS Button to GND
                    if (digitalRead(SOS_BUTTON) == LOW) stepDone = true;
                    break;
            }
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
        if (questComplete) break;
        currentStep++;
        lcdStepSuccessAnimation();
        if (currentStep >= totalSteps) {
            questSuccess = true;
            questComplete = true;
            lcdPrintCentered(lcd, "", "Mision completada", "Felicidades!", "");
            vTaskDelay(3000 / portTICK_PERIOD_MS);
            break;
        }
    }
    flushGameCommandQueue();
    currentGameState = STATE_CONSEQUENCE;
    vTaskDelete(NULL);
    vTaskDelete(NULL);
}

void consequenceTask(void *pvParameters) {
    Serial.println("Consequence task started - End game phase");
    Serial.println("Consequence phase: Processing game results...");
    MainTaskMsg msg;
    // --- STANDARDIZED CONTROLLER USAGE ---
    // - Consequence can end automatically (return to IDLE) or require RF1 short press to return to preparation.
    // - Both behaviors are supported. Choose per project below.
    // --------------------------------------
    bool consequenceComplete = false;
    bool autoEnd = false; // Set to true for auto-end, false to require RF1 short press
    auto isValid = [](const MainTaskMsg& m) {
        // Accept only RF1 short press to finish consequence (if not auto-end)
        return (m.channel == 0 && m.type == SHORT_PRESS);
    };
    bool lcdOff = false;
    while (!consequenceComplete) {
        if (questSuccess) {
            // WIN: Show success message
            lcd.clear();
            lcdPrintCentered(lcd, "", "Mision Cumplida!", "Muy Bien Equipo", "");
        } else {
            // LOSE: Show battery drained and shutdown message, then turn off LCD
            if (!lcdOff) {
                lcd.clear();
                lcdPrintCentered(lcd, "", "Bateria Agotada", "Apagando...", "");
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                lcd.noBacklight();
                lcdOff = true;
            }
        }
        // Wait for RF1 short press to restart
        if (!autoEnd && getRfValidMessage(&msg, isValid)) {
            Serial.printf("Received RF%d %s in consequence phase\n", msg.channel + 1, msg.type == LONG_PRESS ? "long" : "short");
            Serial.println("Restarting system...");
            consequenceComplete = true;
        }
        if (autoEnd) {
            // Example: End consequence after a timer or hardware event
            // consequenceComplete = true;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    // Restore LCD for next game
    lcd.backlight();
    flushGameCommandQueue();
    Serial.println("Consequence phase completed - Restarting system");
    currentGameState = STATE_IDLE;
    vTaskDelete(NULL);
}
