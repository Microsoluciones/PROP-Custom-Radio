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
    pinMode(LED_BUILTIN, OUTPUT);
    
}


void preparationTask(void *pvParameters) {
    while (1) {
        // Preparation logic here
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void questTask(void *pvParameters) {
    while (1) {
        // Quest logic here
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
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