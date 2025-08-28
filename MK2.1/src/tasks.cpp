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
                if (xQueueReceive(gameCommandQueue, &msg, portMAX_DELAY) == pdTRUE) {
                    if (msg.channel == 0 && msg.type == SHORT_PRESS) {
                        Serial.println("Starting preparation phase...");
                        flushGameCommandQueue();
                        currentGameState = STATE_PREPARATION;
                    }
                }
                break;
            case STATE_PREPARATION:
                xTaskCreatePinnedToCore(preparationTask, "PreparationTask", 4096, NULL, 1, &preparationTaskHandle, 1);
                while (currentGameState == STATE_PREPARATION) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                Serial.println("Preparation phase completed");
                break;
            case STATE_QUEST:
                Serial.println("Starting quest phase...");
                xTaskCreatePinnedToCore(questTask, "QuestTask", 4096, NULL, 1, &questTaskHandle, 1);
                while (currentGameState == STATE_QUEST) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                Serial.println("Quest phase completed");
                break;
            case STATE_CONSEQUENCE:
                Serial.println("Starting consequence phase...");
                xTaskCreatePinnedToCore(consequenceTask, "ConsequenceTask", 4096, NULL, 1, &consequenceTaskHandle, 1);
                while (currentGameState == STATE_CONSEQUENCE) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
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

    // --- LCD PREPARATION PHASE MESSAGE ---
    lcd.init();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Pulsacion Larga");
    lcd.setCursor(0, 1);
    lcd.print("en A para iniciar");
    lcd.backlight();

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
        // CUSTOMIZE: Add additional controller logic for configuration here if needed
        if (getRfValidMessage(&msg, isValid)) {
            Serial.printf("Received RF%d %s in preparation phase\n", msg.channel + 1, msg.type == LONG_PRESS ? "long" : "short");
            Serial.println("Preparation complete - Moving to quest phase!");
            setupComplete = true;
        }
    }
    flushGameCommandQueue();
    currentGameState = STATE_QUEST;
    vTaskDelete(NULL);
}

void questTask(void *pvParameters) {


    // --- Adapted quest logic with new architecture ---
    int bateria = 15; // battery percent (15 minutes)
    const unsigned long QUEST_TIME_MS = bateria * 60 * 1000UL;
    unsigned long startTime = millis();
    bool questComplete = false;
    questSuccess = false;
    int step = 0;

    sr.set(LED_QUEST_ACTIVE, 1);
    lcd.clear();
    lcd.setCursor((20-19)/2,2); lcd.print("Alerta: Bateria Baja");
    lcd.setCursor((20-20)/2,3); lcd.print("Bateria Restante:   %");
    lcd.setCursor(17,3); lcd.print(bateria); lcd.print("%");

    while (!questComplete) {
        // Show remaining time
        unsigned long elapsed = millis() - startTime;
        unsigned long remaining = (elapsed < QUEST_TIME_MS) ? (QUEST_TIME_MS - elapsed) : 0;
        int batteryPercent = (remaining + 59999) / 60000; // round up to next minute
        lcd.setCursor(17, 3);
        lcd.print("   ");
        lcd.setCursor(17, 3);
        lcd.print(batteryPercent);

        // Check for timeout FIRST
        if (remaining == 0) {
            Serial.println("Quest time expired - QUEST FAILED");
            questSuccess = false;
            questComplete = true;
            break;
        }

        // Check for skip or end messages (RF2/RF3 long press)
        MainTaskMsg msg;
        auto skipValid = [](const MainTaskMsg& m) { return (m.channel == 1 && m.type == LONG_PRESS); };
        auto failValid = [](const MainTaskMsg& m) { return (m.channel == 2 && m.type == LONG_PRESS); };
        if (getRfValidMessage(&msg, skipValid)) {
            step++;
            blink_backlight();
            Serial.printf("Skipping to step %d\n", step);
            lcd.setCursor(0,0);lcd.print("                    ");
            lcd.setCursor(0,1);lcd.print("                    ");
            if (step > 3) {
                questSuccess = true;
                questComplete = true;
                break;
            }
        } else if (getRfValidMessage(&msg, failValid)) {
            Serial.println("Quest manually failed - QUEST FAILED");
            questSuccess = false;
            questComplete = true;
            break;
        }

        switch (step) {
            case 0: // Step 1: llave_sistema to GND
                lcd.setCursor(0,0); lcd.print("   Iniciar Sistema  ");
                if (digitalRead(llave_sistema) == LOW){
                    lcd.setCursor(0,0);lcd.print("                    ");
                    lcd.setCursor(0,1);lcd.print("                    ");
                    step++;
                    blink_backlight();
                }
                break;
            case 1: // Step 2: circuito_llaves to GND
                lcd.setCursor(0,0); lcd.print("Configurar  Circuito");
                lcd.setCursor(0,1); lcd.print("      Electrico     ");
                if (digitalRead(circuito_llaves) == LOW){
                    lcd.setCursor(0,0);lcd.print("                    ");
                    lcd.setCursor(0,1);lcd.print("                    ");
                    step++;
                    blink_backlight();
                }
                break;
            case 2: { // Step 3: analog potentiometers in range
                lcd.setCursor(0,0); lcd.print("Indicar  Coordenadas");
                lcd.setCursor(0,1); lcd.print(" Latitud y longitud ");
                int pot2 = analogRead(potenciometro_latitud);
                int pot1 = analogRead(potenciometro_longitud);
                if ((pot1 > 600 && pot1 < 1100) && (pot2 > 1800 && pot2 < 2300)){
                    lcd.setCursor(0,0);lcd.print("                    ");
                    lcd.setCursor(0,1);lcd.print("                    ");
                    step++;
                    blink_backlight();
                }
                break;
            }
            case 3: {// Step 4: boton pressed
                lcd.setCursor(0,0); lcd.print("  Enviar Senal SOS  ");
                if (digitalRead(boton) == LOW){
                    lcd.setCursor(0,0);lcd.print("                    ");
                    lcd.setCursor(0,1);lcd.print("                    ");
                    step++;
                    blink_backlight();
                }
                break;
            }
            default:
                questSuccess = true;
                questComplete = true;
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // End of quest phase
    sr.set(LED_QUEST_ACTIVE, 0);
    //lcd.noBacklight();
    currentGameState = STATE_CONSEQUENCE;
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
    while (!consequenceComplete) {
        // CUSTOMIZE: Add additional controller logic for results/cleanup here if needed
        if (!autoEnd && getRfValidMessage(&msg, isValid)) {
            Serial.printf("Received RF%d %s in consequence phase\n", msg.channel + 1, msg.type == LONG_PRESS ? "long" : "short");
            Serial.println("Restarting system...");
            consequenceComplete = true;
        }
        // CUSTOMIZE: Add hardware/logic to end consequence automatically if needed
        if (autoEnd) {
            // Example: End consequence after a timer or hardware event
            // consequenceComplete = true;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    flushGameCommandQueue();
    Serial.println("Consequence phase completed - Restarting system");
    currentGameState = STATE_IDLE;
    vTaskDelete(NULL);
}