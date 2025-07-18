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
        if (event.type == SHORT_PRESS) {
            pcf.write(event.channel, !pcf.read(event.channel));
        }
        msg.channel = event.channel;
        msg.type = event.type;
        xQueueSend(mainTaskQueue, &msg, 0);
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
                xTaskCreatePinnedToCore(preparationTask, "Preparation Task", 2048, NULL, 1, NULL, 1);
            }
            // Set flag when preparation is ready
            if (msg.msgType == PREPARATION_READY) {
                prepReady = true;
                Serial.println("Preparation ready, waiting for long press to start quest...");
            }
            // Start quest on long press after preparation is ready
            if (prepReady && msg.channel == 0 && msg.type == LONG_PRESS) {
                Serial.println("Long press detected, starting quest task");
                xTaskCreatePinnedToCore(questTask, "Quest Task", 2048, NULL, 1, NULL, 1);
                prepReady = false; // Reset for next cycle
            }
            if (msg.msgType == QUEST_COMPLETED ) {
                Serial.println("Quest completed, starting consequence task");
                xTaskCreatePinnedToCore(consequenceTask, "Consequence Task", 2048, NULL, 1, NULL, 1);
            }
            if (consequenceComplete && msg.channel == 0 && msg.type == SHORT_PRESS) {
                Serial.println("Restart requested, starting restart sequence task");
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

    Serial.println("Preparation done, task ending.");
    lcd.setCursor(1, 0); lcd.print("Preparacion lista!"); sr.set(LED_PREPARATION_READY, HIGH);
    vTaskDelete(NULL); // End this task when done
}


void questTask(void *pvParameters) {
    MainTaskMsg msg;
    bool state = 0; // Example state variable, adjust as needed
    const unsigned long QUEST_TIME_MS = bateria * 60 * 1000; // 15 minutes
    unsigned long startTime = millis();
    bool questFinished = false;
    bool skipp = false;
    if (state == 0) {
        state = 1;
    } else {
        state = 0;
    }
    sr.set(LED_QUEST_0, state);
    lcd.clear();
    // lcd.setCursor(0, 0); lcd.print("Quest started!");
    lcd.setCursor(0,2);    lcd.print("Alerta: Bateria Baja");
    lcd.setCursor(0,3);    lcd.print("Bateria Restante:  %");
    lcd.setCursor(17,3);    lcd.print(bateria); lcd.print("%");

    while (!questFinished) {
        if (xQueueReceive(mainTaskQueue, &msg, 1000/portTICK_PERIOD_MS) == pdTRUE) {
            if (msg.channel == 1 && msg.type == LONG_PRESS) {
                Serial.println("Step Skipped");
                skipp = true;
            } else if (msg.channel == 2 && msg.type == LONG_PRESS) {
                Serial.println("Quest Terminated");
                questFinished = true;
            }
        }
        // Show remaining time
        unsigned long elapsed = millis() - startTime;
        unsigned long remaining = (elapsed < QUEST_TIME_MS) ? (QUEST_TIME_MS - elapsed) : 0;
        lcd.setCursor(17, 3);
        lcd.print(":   ");
        lcd.setCursor(17, 3);
        lcd.print(remaining / 60000); lcd.print("%");
        //vTaskDelay(1000 / portTICK_PERIOD_MS); // Update every second
        // Step 1: llave_sistema to GND
        lcd.setCursor(0,0); lcd.print("                    ");
        lcd.setCursor(0,1); lcd.print("                    ");
        lcd.setCursor(0,0); lcd.print("   Iniciar Sistema  ");
        if (digitalRead(llave_sistema) != LOW || skipp) continue; parpadear_backlight();
        skipp = false;
        // Step 2: circuito_llaves to GND
        lcd.setCursor(0,0); lcd.print("                    ");
        lcd.setCursor(0,1); lcd.print("                    ");
        lcd.setCursor(0,0); lcd.print("Configurar  Circuito"); 
        lcd.setCursor(0,1); lcd.print("      Electrico     ");
        if (digitalRead(circuito_llaves) != LOW) continue; parpadear_backlight();

        // Step 3: analog potentiometers in range
        lcd.setCursor(0,0); lcd.print("                    ");
        lcd.setCursor(0,1); lcd.print("                    ");
        lcd.setCursor(0,0); lcd.print("Indicar  Coordenadas");
        lcd.setCursor(0,1); lcd.print(" Latitud y longitud ");
        int pot1 = analogRead(potenciometro_latitud);
        int pot2 = analogRead(potenciometro_longitud);
        if (!(pot1 > 1000 && pot1 < 2000) && !(pot2 > 1500 && pot2 < 2500)) continue;  parpadear_backlight(); // Example range, adjust as needed

        // Step 4: boton pressed
        lcd.setCursor(0,0); lcd.print("                    ");
        lcd.setCursor(0,1); lcd.print("                    ");
        lcd.setCursor(0,0); lcd.print("  Enviar Señal SOS  ");
        if (digitalRead(boton) != LOW || skipp) continue; parpadear_backlight();
        skipp = false;

        // All steps complete
        questFinished = true;
    }

    //lcd.clear();
    //lcd.setCursor(0, 0); lcd.print("Quest completada!");

    // Notify mainTask
    //MainTaskMsg msg = {};
    msg.msgType = QUEST_COMPLETED;
    xQueueSend(mainTaskQueue, &msg, 0);

    vTaskDelete(NULL);
}


void consequenceTask(void *pvParameters) {
    while (1) {
        // Consequence logic here
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void restartSequenceTask(void *pvParameters) {
    while (1) {
        // Restart sequence logic here
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}