// ALIMENTADOR Aquasonic (ATmega 2560)
// Base: biomatic-feeder v00.00.27 proyecto modularizado
#include <math.h>
#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include "core/vars.h"
#include <TimeLib.h>
#include <EEPROM.h>

static const unsigned long MOTOR_TEST_INRUSH_MS = 500;

static void motorTestShutdownAll(void) {
  digitalWrite(X1, OFF);
  digitalWrite(X2, OFF);
  digitalWrite(X3, OFF);
}

static bool motorTestSenseActive(unsigned long x1OnMs, unsigned long x2OnMs, unsigned long x3OnMs) {
  const unsigned long now = millis();
  bool tripped = false;
  if ((x1OnMs != 0) && (digitalRead(X1) == ON) && ((now - x1OnMs) >= MOTOR_TEST_INRUSH_MS)) {
    if (acsGuardMotor(X1)) {
      tripped = true;
    }
  }
  if ((x2OnMs != 0) && (digitalRead(X2) == ON) && ((now - x2OnMs) >= MOTOR_TEST_INRUSH_MS)) {
    if (acsGuardMotor(X2)) {
      tripped = true;
    }
  }
  if ((x3OnMs != 0) && (digitalRead(X3) == ON) && ((now - x3OnMs) >= MOTOR_TEST_INRUSH_MS)) {
    if (acsGuardMotor(X3)) {
      tripped = true;
    }
  }
  if (tripped) {
    motorTestShutdownAll();
    Serial.println(F("Test de motores abortado por exceso de consumo"));
  }
  return tripped;
}

static bool motorTestHold(unsigned long durationMs, unsigned long x1OnMs, unsigned long x2OnMs, unsigned long x3OnMs) {
  const unsigned long t0 = millis();
  while ((unsigned long)(millis() - t0) < durationMs) {
    if (motorTestSenseActive(x1OnMs, x2OnMs, x3OnMs)) {
      return false;
    }
    delay(20);
  }
  return true;
}

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
  /* Test de arranque: misma proteccion de consumo que en dosificacion. */
  unsigned long x1OnMs = 0;
  unsigned long x2OnMs = 0;
  unsigned long x3OnMs = 0;
  bool motorTestOk = true;
  digitalWrite(X1, ON);
  x1OnMs = millis();
  motorTestOk = motorTestHold(1000, x1OnMs, x2OnMs, x3OnMs);
  if (motorTestOk) {
    digitalWrite(X2, ON);
    x2OnMs = millis();
    motorTestOk = motorTestHold(1000, x1OnMs, x2OnMs, x3OnMs);
  }
  if (motorTestOk) {
    digitalWrite(X3, ON);
    x3OnMs = millis();
    motorTestOk = motorTestHold(1000, x1OnMs, x2OnMs, x3OnMs);
  }
  if (motorTestOk) {
    digitalWrite(X3, OFF);
    x3OnMs = 0;
    motorTestOk = motorTestHold(1000, x1OnMs, x2OnMs, x3OnMs);
  }
  if (motorTestOk) {
    digitalWrite(X2, OFF);
    x2OnMs = 0;
    motorTestOk = motorTestHold(1000, x1OnMs, x2OnMs, x3OnMs);
  }
  motorTestShutdownAll();
  acs.highCurrentX2 = false;
  acs.highCurrentX3 = false;
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
    Serial.print(acs.ampX3);
#if MOTOR_WEAR_DEBUG
    Serial.print(", ");
    Serial.print(x2WearBaselineRef());
    Serial.print(", ");
    Serial.print(x2WearBaselineSlow());
    Serial.print(", ");
    Serial.print(x2WearHealthyCount());
    Serial.print(", ");
    Serial.print(x2WearAlarmActive() ? 1 : 0);
    Serial.print(", ");
    Serial.print(bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 2), 0));
    Serial.print(", ");
    Serial.print(x3WearBaselineRef());
    Serial.print(", ");
    Serial.print(x3WearBaselineSlow());
    Serial.print(", ");
    Serial.print(x3WearHealthyCount());
    Serial.print(", ");
    Serial.print(x3WearAlarmActive() ? 1 : 0);
    Serial.print(", ");
    Serial.print(bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 2), 1));
    Serial.print(", ");
    Serial.print(bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 2), 2));
    Serial.print(", ");
    Serial.print(bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 2), 3));
    Serial.print(", ");
    Serial.print(bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 2), 4));
    Serial.print(", ");
    Serial.print(x2BladeAlarmActive() ? 1 : 0);
    Serial.print(", ");
    Serial.print(bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 5), 7));
    Serial.print(", ");
    Serial.print(motorRuntimeX2Seconds());
    Serial.print(", ");
    Serial.print(motorRuntimeX3Seconds());
#endif
    Serial.println();
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
