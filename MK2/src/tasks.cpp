#include "globals.h"
#include "functions.h"
#include "tasks.h"
/*
A FreeRTOS task runs the code inside its function. When the function returns (reaches the end or executes a return), the task is deleted automatically and its resources are freed.

Best practice:
If you want a task to run forever, use a while(1) loop inside it.
If you want it to run once and finish, just let the function end.

After the task ends:

The task is removed from the scheduler.
If you want to run it again, you must create it again with xTaskCreatePinnedToCore().
Summary:

Task ends → it is deleted.
To run again, create it again.
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
        msg.channel = event.channel;
        msg.type = event.type;
        xQueueSend(mainTaskQueue, &msg, 0);
        xQueueSend(questTaskQueue, &msg, 0); // Send to questTaskQueue if quest is running
        }
        
    }
}

/* receive from rf task/-.
    MainTaskMsg msg;
    bool lightsOn = false;
    while (1) {
        if (xQueueReceive(mainTaskQueue, &msg, portMAX_DELAY) == pdTRUE) {
            if (msg.channel == 0 && msg.type == SHORT_PRESS) {
                if (!lightsOn) {
                    setRedLighting(true);
                    setGreenLighting(true);
                    lightsOn = true;
                } else {
                    playAudioInterrupt(1); // Play instructions
                }
            }
            if (msg.channel == 0 && msg.type == LONG_PRESS) {
                break; // Start the game
            }
        }
    }
*/
void mainTask(void *pvParameters) {
    MainTaskMsg msg;
    bool prepReady = false;
    bool questComplete = false;
    bool consequenceComplete = false;
    lcd.setCursor(0, 0); lcd.print("Esperando...");
    lcd.setCursor(0, 1); lcd.print("   Pulsar boton A"); lcd.setCursor(0, 2); lcd.print("    para iniciar");
    Serial.println("Main task started, waiting for events...");
    while (1) {
        if (xQueueReceive(mainTaskQueue, &msg, portMAX_DELAY) == pdTRUE) {
            // Start preparation on short press
            if (!prepReady && msg.channel == 0 && msg.type == SHORT_PRESS) {
                Serial.println("Preparation task started");
                clearQueue(mainTaskQueue); // Clear any pending messages
                xTaskCreatePinnedToCore(preparationTask, "Preparation Task", 2048, NULL, 1, NULL, 1);
            }
            // Set flag when preparation is ready
            if (msg.msgType == PREPARATION_READY) {
                prepReady = true;
                clearQueue(mainTaskQueue); // Clear any pending messages
                Serial.println("Preparation ready, waiting for long press to start quest...");
            }
            // Start quest on long press after preparation is ready
            if (prepReady && msg.channel == 0 && msg.type == LONG_PRESS) {
                Serial.println("Long press detected, starting quest task");
                clearQueue(mainTaskQueue); // Clear any pending messages
                clearQueue(questTaskQueue); // Clear quest queue too
                xTaskCreatePinnedToCore(questTask, "Quest Task", 2048, NULL, 1, NULL, 1);
                prepReady = false; // Reset for next cycle
            }
            // Start consequence task and STORE THE HANDLE
            if (msg.msgType == QUEST_COMPLETED || msg.msgType == QUEST_FAILED) {
                Serial.println("Quest ended, starting consequence task");
                
                // Store the quest result globally
                lastQuestSuccess = (msg.msgType == QUEST_COMPLETED);
                
                clearQueue(mainTaskQueue); // Clear any pending messages
                xTaskCreatePinnedToCore(consequenceTask, "Consequence Task", 2048, NULL, 1, &consequenceTaskHandle, 1);
            }
            // DELETE consequence task on game master short press (channel 1)
            if (msg.channel == 0 && msg.type == SHORT_PRESS && consequenceTaskHandle != NULL) {
                Serial.println("Game master requested consequence task deletion");
                vTaskDelete(consequenceTaskHandle);
                consequenceTaskHandle = NULL;
                clearQueue(mainTaskQueue); // Clear any pending messages
                lcd.clear();
                lcd.setCursor(0, 0); lcd.print("Esperando...");
                lcd.setCursor(0, 1); lcd.print("   Pulsar boton A"); 
                lcd.setCursor(0, 2); lcd.print("    para iniciar");
            }
            // Handle consequence completion
            if (msg.msgType == CONSEQUENCE_COMPLETED) {
                consequenceComplete = true;
                clearQueue(mainTaskQueue); // Clear any pending messages
                Serial.println("Consequence task completed");
                xTaskCreatePinnedToCore(restartSequenceTask, "Restart Sequence Task", 2048, NULL, 1, NULL, 1);
            }
        }
    }
}

void preparationTask(void *pvParameters) {
    lcd.clear();
    lcd.backlight();
    lcd.setCursor(0,2); lcd.print("Alerta: Bateria Baja");
    lcd.setCursor(0,3); lcd.print("Bateria Restante:");
    lcd.setCursor(17,3);lcd.print(bateria);

    // Notify mainTask that preparation is ready
    MainTaskMsg readyMsg = {};
    readyMsg.msgType = PREPARATION_READY;
    xQueueSend(mainTaskQueue, &readyMsg, 0);

    lcd.setCursor(1, 0); lcd.print("Preparacion lista!"); 
    sr.set(LED_PREPARATION_READY, HIGH);
    vTaskDelete(NULL); // End this task when done
}


void questTask(void *pvParameters) {
    MainTaskMsg msg;
    const unsigned long QUEST_TIME_MS = bateria * 60 * 1000; // Battery minutes in milliseconds
    unsigned long startTime = millis();
    bool questFinished = false;
    int step = 0;

    sr.set(LED_QUEST_0, 1);
    lcd.clear();
    lcd.setCursor(0,2); lcd.print("Alerta: Bateria Baja");
    lcd.setCursor(0,3); lcd.print("Bateria Restante:  %");
    lcd.setCursor(17,3); lcd.print(bateria); lcd.print("%");

    while (!questFinished) {
        // Show remaining time
        unsigned long elapsed = millis() - startTime;
        unsigned long remaining = (elapsed < QUEST_TIME_MS) ? (QUEST_TIME_MS - elapsed) : 0;
        lcd.setCursor(17, 3);
        lcd.print(":   ");
        lcd.setCursor(17, 3);
        lcd.print(remaining / 60000); // Show minutes remaining
        
        // Check for timeout FIRST
        if (remaining == 0) {
            Serial.println("Quest time expired - QUEST FAILED");
            // Send QUEST_FAILED message to mainTask ONLY
            msg.msgType = QUEST_FAILED;
            xQueueSend(mainTaskQueue, &msg, 0);
            // Do NOT send to questTaskQueue
            vTaskDelete(NULL);
            return; // Exit task
        }
        
        // Check for skip or end messages
        if (xQueueReceive(questTaskQueue, &msg, 10/portTICK_PERIOD_MS) == pdTRUE) {
            if (msg.channel == 1 && msg.type == LONG_PRESS) {
                step++; // Skip to next step
                parpadear_backlight();
                Serial.printf("Skipping to step %d\n", step);
                lcd.setCursor(0,0);lcd.print("                    ");
                lcd.setCursor(0,1);lcd.print("                    ");
                if (step > 3) {
                    questFinished = true;
                    Serial.println("Quest completed successfully");
                    msg.msgType = QUEST_COMPLETED;
                    xQueueSend(mainTaskQueue, &msg, 0);
                    // Do NOT send to questTaskQueue
                    vTaskDelete(NULL); // Exit task
                    return;
                }
            } else if (msg.channel == 2 && msg.type == LONG_PRESS) {
                Serial.println("Quest manually failed - QUEST FAILED");
                // Send QUEST_FAILED message to mainTask ONLY
                msg.msgType = QUEST_FAILED;
                xQueueSend(mainTaskQueue, &msg, 0);
                // Do NOT send to questTaskQueue
                vTaskDelete(NULL);
                return; // Exit task
            }
        }
        switch (step) {
            case 0: // Step 1: llave_sistema to GND
                lcd.setCursor(0,0); lcd.print("   Iniciar Sistema  ");
                if (digitalRead(llave_sistema) == LOW){
                    lcd.setCursor(0,0);lcd.print("                    ");
                    lcd.setCursor(0,1);lcd.print("                    ");
                    step++;
                    parpadear_backlight();
                } 
                break;
            case 1: // Step 2: circuito_llaves to GND
                lcd.setCursor(0,0); lcd.print("Configurar  Circuito");
                lcd.setCursor(0,1); lcd.print("      Electrico     ");
                if (digitalRead(circuito_llaves) == LOW){
                    lcd.setCursor(0,0);lcd.print("                    ");
                    lcd.setCursor(0,1);lcd.print("                    ");
                    step++;
                    parpadear_backlight();
                } 
                break;
            case 2: {// Step 3: analog potentiometers in range
                lcd.setCursor(0,0); lcd.print("Indicar  Coordenadas");
                lcd.setCursor(0,1); lcd.print(" Latitud y longitud ");
                int pot2 = analogRead(potenciometro_latitud);
                int pot1 = analogRead(potenciometro_longitud);
                if ((pot1 > 600 && pot1 < 1100) && (pot2 > 1800 && pot2 < 2300)){
                    lcd.setCursor(0,0);lcd.print("                    ");
                    lcd.setCursor(0,1);lcd.print("                    ");
                    step++;
                    parpadear_backlight();
                } 
            }
                break;
            case 3: // Step 4: boton pressed
                lcd.setCursor(0,0); lcd.print("  Enviar Senal SOS  ");
                if (digitalRead(boton) == LOW){
                    lcd.setCursor(0,0);lcd.print("                    ");
                    lcd.setCursor(0,1);lcd.print("                    ");
                    step++;
                    parpadear_backlight();
                } 
                break;
            default:
                questFinished = true;
                break;
        }
    }

    // Natural completion
    msg.msgType = QUEST_COMPLETED;
    xQueueSend(mainTaskQueue, &msg, 0);
    // Do NOT send to questTaskQueue
    vTaskDelete(NULL);
}


void consequenceTask(void *pvParameters) {
    MainTaskMsg msg;
    lcd.clear();    
    
    if (lastQuestSuccess) {
        // Success sequence (same as before)
        parpadear_backlight();
        lcd.clear(); lcd.setCursor(0,0); 
        lcd.print("  Mision Cumplida!  ");
        for(int i = 0; i < 3 ; i++){
            scrollText(1, "Muy Bien Equipo! ", 500, 20);
            vTaskDelay(100/portTICK_PERIOD_MS);
        }
    } else {
        // Failure sequence
        for(int i = 0; i < 5; i++) {
            parpadear_backlight();
            vTaskDelay(200/portTICK_PERIOD_MS);
        }
        lcd.noBacklight();
        lcd.clear();
        //lcd.setCursor(0,0); lcd.print("   Mision Fallida   ");
        //lcd.setCursor(0,1); lcd.print("   Tiempo Agotado   ");
        vTaskDelay(20000/portTICK_PERIOD_MS);
    }
    
    msg.msgType = CONSEQUENCE_COMPLETED;
    xQueueSend(mainTaskQueue, &msg, 0);
    consequenceTaskHandle = NULL;
    vTaskDelete(NULL); // End this task when done
}

void restartSequenceTask(void *pvParameters) {
    bool prepReady = false;
    bool questComplete = false;
    bool consequenceComplete = false;
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Esperando...");
    lcd.setCursor(0, 1); lcd.print("   Pulsar boton A"); lcd.setCursor(0, 2); lcd.print("    para iniciar");
    vTaskDelete(NULL); // End this task when done
}