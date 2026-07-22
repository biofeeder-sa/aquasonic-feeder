// ALIMENTADOR Aquasonic (ATmega 2560)
// Base: biomatic-feeder v00.00.27 â€” proyecto modularizado
#include <math.h>
#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include "core/vars.h"
#include <TimeLib.h>
#include <EEPROM.h>
void setup()
{
  /*****  INICIALIZACION DEL PUERTO SERIAL  *****/
  analogReference(DEFAULT);   // voltaje de referencia
  //Puerto Serial
  Serial.begin(9600);
  Serial3.begin(9600);
  //Pin Mode
  pinMode(X1, OUTPUT);    //MOTOR COMO SALIDA
  pinMode(X2, OUTPUT);    //MOTOR COMO SALIDA
  pinMode(X3, OUTPUT);    //MOTOR COMO SALIDA
  pinMode(X0_1, OUTPUT);    //MOTOR COMO SALIDA
  pinMode(X0_2, OUTPUT);    //MOTOR COMO SALIDA
  pinMode(OE_DRIVER, OUTPUT);   //OE DRIVER
  pinMode(PIN_RESET, OUTPUT);   //RESET DEL XBEE
  //pinMode(LED_ACTIVE , OUTPUT);
  //pinMode(LED_STANDBY , OUTPUT);
  pinMode(LED, OUTPUT);        //LED COMO SALIDA
  pinMode(CH_BATTERY , INPUT);//BATERIA COMO ENTRADA
  pinMode(CH_PANNEL , INPUT); //PANEL COMO ENTRADA
  pinMode(ADDO, INPUT_PULLUP);   //entrada para recibir datos del peso
  pinMode(ADSK, OUTPUT);         //saÃ­da para SCK
  digitalWrite(X1, LOW);
  digitalWrite(X2, LOW);
  digitalWrite(X3, LOW);
  digitalWrite(X0_1, LOW);
  digitalWrite(X0_2, LOW);
  digitalWrite(OE_DRIVER, HIGH);   
  digitalWrite(PIN_RESET, HIGH);
  digitalWrite(LED, OFF);
  digitalWrite(8, OFF);
  appStateInit();
  Start_EEPROM();
  initConfig();
  postInitConfig();
  sei(); // HABILITACION DE LAS INTERRUPCIONES GLOBALES
  digitalWrite(X1, ON);
  delay(1000);
  digitalWrite(X2, ON);
  delay(1000);
  digitalWrite(X3, ON);
  delay(1000);
  /* Test de arranque de motor */
  digitalWrite(X3, OFF);              //APAGAR MOTORES
  delay(1000);
  digitalWrite(X2, OFF);
  delay(1000);
  digitalWrite(X1, OFF);
  delay(2000);
}
void loop()
{
  Send_Sequence_Broadcast();//ENVIO DEL BROADCAST CON SECUENCIA (0,2,4,8,16,32)
  xbee_communication();       //COMUNICACION CON XBEE (Serial3)
  Sense_Voltajes();         //SENSAR VOLTAJE DE BATERIA Y PANEL
  calibration();
  if(app.dosage.rtcSynced == TRUE)
  {
    Internal_Clock();   //RTC INTERNO
    Print_Hour();       //IMPRIMIR HORA ACTUAL EN PUERTO SERIE
    Update_EEPROM();    //ACTUALIZAR LAS TABLAS DE ALIMENTACION
    if(FeedRate_Dosage)
    {
      Dosage_FeedRate();           //DOSIFICACION CON FEED-RATE
      if(Events_ArduinoUNO || Table_ArduinoUNO)
      {
        Send_Sequence_Alarms();
      }
    }
  if (app.dosage.dosingActive == true || app.dosage.calibrating == true){
    //*********** VOLTAJE ******************
    //   // Lee el valor analÃ³gico del sensor de la baterÃ­a
    // int batteryValue = analogRead(CH_BATTERY);
    // // Calcula el voltaje de la baterÃ­a usando el divisor resistivo
    // float batteryVoltage = batteryValue * (referenceVoltage / 1023.0) * (r1 + r2) / r2;
    //*********** FIN VOLTAJE ******************
    // Imprime el valor de la corriente en current_avgerios en el monitor serial
    Serial.print(millis());
    Serial.print(", ");
    Serial.print(acs.corriente_actual);
    Serial.print(", ");
    Serial.print(acs.corriente_minima);
    Serial.print(", ");
    Serial.print(acs.corriente_maxima);
    Serial.print(", ");
    Serial.print(acs.umbral);
    // Serial.print(", ");
    // Serial.print(batteryVoltage);
    Serial.print(", ");
    Serial.print(acs.nPicos);
    Serial.print(", ");
    Serial.print(app.dosageRt.revolutionFraction);
    Serial.print(", ");
    Serial.print(acs.tiempoEntreVueltas);
    Serial.print(", ");
    Serial.print(acs.previousAmpMinX2);
    Serial.print(", ");
    Serial.print(acs.previousAmpMaxX2);
    Serial.print(", ");
    Serial.print(acs.previousAmpMinX3);
    Serial.print(", ");
    Serial.print(acs.previousAmpMaxX3);
    Serial.print(", ");
    Serial.println(acs.ampX3);
    }
  }
  /*    FUERA DEL RTC REAL    */
  //TODO: OPTIMIZACION DEL CONDICIONAL: CREAR FUNCION QUE EJECUTE EL SET DE LA HORA CUANDO ESTA SEA CONFIGURADA INICIALMENTE Y NUEVAMENTE
  if((app.dosage.rtcSynced == FALSE))
  {
    Update_EEPROM();
    rtcBootstrapUntilSynced();
  }
}
