#include <Arduino.h>
#include <ESP32Time.h>
#include <LiquidCrystal_I2C.h>


int perilla_superior = 19;
int circuito_electrico = 17; // combinacion de jacks.
int adc_1 = 4;
int adc_2 = 2;
int boton_final = 18;

TaskHandle_t task_reloj_handle = NULL;
TaskHandle_t task_quest_handle = NULL;

int lcdColumns = 20;
int lcdRows = 4;
int bateria = 20;

//bool quest_COMPLETE = false;
//bool quest_FAILED   = false;

ESP32Time rtc(3600); // init rtc
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); // init lcd

bool bateria_restante(void);
int quest();
void quest_FAILED();
void quest_COMPLETE();
void parpadear_backlight();

void setup() {
  Serial.begin(115200);
  //init_gpio();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,2); lcd.print("Alerta: Bateria Baja"); // leyendas iniciales en las primeras 2 filas.
  lcd.setCursor(0,3); lcd.print("Bateria Restante   %");
  lcd.setCursor(17,3);lcd.print(bateria);
  rtc.setTime(01, 00, 00, 16, 7, 2023);  // 16th Junio 2023 00:00:01
  // put your setup code here, to run once:
}

void loop() {
  switch (quest()){
    case 1:
      quest_COMPLETE();
      break;
    case 2:
      quest_FAILED();
      break;
  }
  if(quest_FAILED == true){
    

  }
}

int quest(){
  lcd.setCursor(0,0); lcd.print("---Iniciar Sistema--");
  for(;digitalRead(perilla_superior) == HIGH;){
    if (bateria_restante() == true){
    return 2; // se agoto el tiempo/ bateria.
    }
  }
  parpadear_backlight();
  lcd.setCursor(0,0); lcd.print("Configurar Circuito"); lcd.setCursor(0,1); lcd.print("      Electrico     ");
  for(;digitalRead(circuito_electrico) == HIGH;){
    if (bateria_restante() == true){
    return 2; // se agoto el tiempo/ bateria.
    }
  }





  return 1; // se completo el escape exitosamente.

  return 2; // se agoto el tiempo/ bateria.
}
void quest_FAILED(){

}
void quest_COMPLETE(){

}
void parpadear_backlight(){
  lcd.noBacklight();
  delay(100);
  lcd.backlight();
  delay(100);
  lcd.noBacklight();
  delay(100);
  lcd.backlight();
  delay(100);
}
void init_gpio(void){
  /*pinMode(,);
  pinMode(,);
  pinMode(,);
  pinMode(,);
  pinMode(,);
  pinMode(,);
  */
}

bool bateria_restante(void){
  if(rtc.getSecond() == 00 ){ // modifica el porcentaje de bateria cada 1 min.
    bateria--;
    lcd.setCursor(17,1);
    lcd.print(bateria);
    if(bateria == 0){
      return true;
    }
  } return false;
}

/*
void task_reloj(void * parameters){
  rtc.setTime(01, 00, 00, 16, 7, 2023);  // 16th Junio 2023 00:00:01
  int bateria = 20; // Variable que almacena el estado de la bateria y se utiliza para imprimir en pantalla.
  lcd.print(bateria);
  for(;;){
    vTaskDelay(100 / portTICK_PERIOD_MS); // DELAY DE 100ms en tasks.
    if(rtc.getSecond() == 00 ){ // modifica el porcentaje de bateria cada 1 min.
      bateria--;
      lcd.setCursor(17,1);
      lcd.print(bateria);
      if (bateria == 00){ // quizas sea necesario remover el task_... =! NULL.
       // poner que sucede cuando se acaba la bateria. Mover a otra parte del codigo en caso de ser necesario.
      // pone algo en la pantalla o lo que quieras
      // leyenda opcional; "LA RADIO NO TIENE SUFICIENTE ENERGIA PARA TRANSMITIR" o algo asi.
      Serial.println("Se acabo la bateria, escape finalizado.");
      //lcd.clear();
      //lcd.setCursor();
      //lcd.print();
      quest_COMPLETE = true;
      vTaskDelete(task_quest_handle); // Elimina la tarea quest del procesamiento, deja de funcionar.
      }
    }
  }
}
void task_quest(void * parameters){
  // PONE ACA TU CODIGO DEL ESCAPE.
  /*
  Paso 1: Accionar Perilla Superior.
  Paso 2: Configurar Circuito electrico(Jacks 6.5mm).
  Paso 3: Indicar Coordenadas de Indicacion ( ADC con Potenciometros, 27 y 68).
  Paso 4: Presionar Boton.
  
  for(;digitalWrite() == HIGH;){
  }
  for(;;)
  
}*/