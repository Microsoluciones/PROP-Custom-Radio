#include <Arduino.h>
#include <ESP32Time.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>


int perilla_superior = 19;
int circuito_electrico = 13; // combinacion de jacks. estaba en el 17 y no funcionaba, se cambio al 13 para probar...
int adc_lat = 4; // combinacion de frecuencias de fichas.
int adc_lon = 2;
int boton_final = 18;

int lcdColumns = 20;
int lcdRows = 4;
int bateria = 20; // tiempo total de escape en minutos... se ve reflejado en la pantalla como porcentaje de bateria.
String mensaje_escape = "Ya hemos enviado un avion de rescate, esperar fuera del refugio con la bengala";

ESP32Time rtc(3600); // init rtc
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); // init lcd

bool bateria_restante(void);
void init_gpio(void);
int quest(void);
void quest_FAILED(void);
void quest_COMPLETE(void);
void parpadear_backlight(void);
void scrollText(int row, String message, int delayTime, int lcdColumns);

void setup() {
  Serial.begin(115200);
  init_gpio();
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
}
bool medida_adc_lat (void){
  while (true){
    int valor_adc = analogRead(adc_lat);

    if(valor_adc >= 15 && valor_adc <= 780) {
        return 0;
    }else{
      if (bateria_restante() == true){
      return 1; // se agoto el tiempo/ bateria.
      }
    }
  }
}

bool medida_adc_lon (void){
  while (true){
    int valor_adc = analogRead(adc_lon);

    if(valor_adc <= 2400 && valor_adc >= 1850) {
        return 0;
    }else{
      if (bateria_restante() == true){
      return 1; // se agoto el tiempo/ bateria.
      }
    }
  }
}

bool espera_valores_en_rango() {
    while (true) {
        int latitud = medida_adc_lat();
        int longitud = medida_adc_lon();

        if (latitud == 0 && longitud == 0) {
            // Ambos valores están dentro del rango, puedes continuar.
            return true;
        }

        if (latitud == 2 || longitud == 2) {
            // Se agotó la batería durante la espera.
            return false;
        }
    }
}
int quest(){
  lcd.setCursor(0,0); lcd.print("                    ");
  lcd.setCursor(0,1); lcd.print("                    ");
  lcd.setCursor(0,0); lcd.print("   Iniciar Sistema  ");
  
  
  for(;digitalRead(perilla_superior) == HIGH;){ // cambiar low a high.
    if (bateria_restante() == true){
    return 2; // se agoto el tiempo/ bateria.
    }
  }parpadear_backlight();
  
  
  lcd.setCursor(0,0); lcd.print("                    ");
  lcd.setCursor(0,1); lcd.print("                    ");
  lcd.setCursor(0,0); lcd.print("Configurar  Circuito"); 
  lcd.setCursor(0,1); lcd.print("      Electrico     ");
  
  for(;digitalRead(circuito_electrico) == HIGH;){ // cambiar low a high.
    if (bateria_restante() == true){
    return 2; // se agoto el tiempo/ bateria.
    }
  }parpadear_backlight();


  lcd.setCursor(0,0); lcd.print("                    ");
  lcd.setCursor(0,1); lcd.print("                    ");
  lcd.setCursor(0,0); lcd.print("Indicar  Coordenadas");
  lcd.setCursor(0,1); lcd.print(" Latitud y longitud ");

  if (espera_valores_en_rango() == true){
    parpadear_backlight();
  }

  lcd.setCursor(0,0); lcd.print("                    ");
  lcd.setCursor(0,1); lcd.print("                    ");
  lcd.setCursor(0,0); lcd.print("  Enviar Señal SOS  ");
  for(;digitalRead(boton_final) == HIGH;){
    if (bateria_restante() == true){
    return 2; // se agoto el tiempo/ bateria.
    }
  }parpadear_backlight();
  return 1; // se completo el escape exitosamente.
}

void quest_FAILED(void){
  parpadear_backlight();
  parpadear_backlight();
  parpadear_backlight();
  parpadear_backlight();
  for(;;){
    lcd.noBacklight();
    delay(10000);
  }
}
void quest_COMPLETE(void){
  lcd.clear(); lcd.setCursor(0,0); 
  lcd.print("   Muy Bien Equipo! ");
  for(;;){
    scrollText(1, mensaje_escape, 500, 20);
  }
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
void init_gpio(void){
  pinMode(perilla_superior, INPUT_PULLDOWN);
  pinMode(circuito_electrico,INPUT);
  //pinMode(adc_1, INPUT);
  //pinMode(adc_2,INPUT);
  pinMode(boton_final,INPUT_PULLDOWN);
}
bool bateria_restante(void){ // funciona 9, quizas se pierde un minuto por los delays.
  if(rtc.getSecond() == 00 ){ // modifica el porcentaje de bateria cada 1 min.
    bateria--;
    if(bateria < 10){
      lcd.setCursor(17,3); lcd.print(" ");
      lcd.print(bateria);
    }
    else {
      lcd.setCursor(17,3); lcd.print(bateria);
    }
    if(bateria == 0){
      return true;
    }
    delay(1000); // evita que pase los minutos de una
  } return false;
}
void scrollText(int row, String message, int delayTime, int lcdColumns) {
  for (int i=0; i < lcdColumns; i++) {
    message = " " + message;  
  } 
  message = message + " "; 
  for (int pos = 0; pos < message.length(); pos++) {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}
