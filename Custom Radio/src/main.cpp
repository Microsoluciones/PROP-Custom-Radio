#include <Arduino.h>
#include <ESP32Time.h>
#include <LiquidCrystal_I2C.h>
#include <freertos/FreeRTOS.h>

void task_reloj(void * parameters); TaskHandle_t task_reloj_handle = NULL;
void task_quest(void * parameters); TaskHandle_t task_quest_handle = NULL;
int lcdColumns = 20;
int lcdRows = 4;
int bateria = 20;

ESP32Time rtc(3600); // init rtc
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); // init lcd

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  xTaskCreate( // crea la tarea del reloj
    task_reloj,
    "Task Reloj",
    2000,
    NULL,
    1,
    &task_reloj_handle
  );

  xTaskCreate( // crea la tarea del reloj
    task_quest,
    "Task Escape",
    1500,
    NULL,
    2,
    &task_quest_handle
  );

  lcd.setCursor(0,0); lcd.print("Alerta: Bateria Baja"); // leyendas iniciales en las primeras 2 filas.
  lcd.setCursor(0,1); lcd.print("Bateria Restante 20%");
  // put your setup code here, to run once:
}

void loop() {
  // put your main code here, to run repeatedly:
}

void task_reloj(void * parameters){
  for(;;){
    vTaskDelay(100 / portTICK_PERIOD_MS); // DELAY DE 100ms en tasks.
    if(rtc.getSecond() == 00 ){ // modifica el porcentaje de bateria cada 1 min.
      bateria--;
      lcd.setCursor(17,1);
      lcd.print(bateria);
      if (bateria == 0 && task_quest_handle =! NULL){ // quizas sea necesario remover el task_... =! NULL.
       // poner que sucede cuando se acaba la bateria. Mover a otra parte del codigo en caso de ser necesario.
      // pone algo en la pantalla o lo que quieras
      // leyenda opcional; "LA RADIO NO TIENE SUFICIENTE ENERGIA PARA TRANSMITIR" o algo asi.
      Serial.println("Se acabo la bateria, escape finalizado.");
      //lcd.clear();
      //lcd.setCursor();
      //lcd.print();
      vTaskDelete(task_quest_handle); // Elimina la tarea quest del procesamiento, deja de funcionar.
      }
      
    }
  }
}

void task_quest(void * parameters){
  // PONE ACA TU CODIGO DEL ESCAPE.
}

