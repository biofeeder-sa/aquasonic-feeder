#include <Arduino.h>
#include <math.h>
#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include "core/vars.h"

void Dosage_FeedRate(void)
{
  static uint32_t dosageTimeMs = 0;
  static uint16_t activeCycleGrams = 0;
  const uint16_t cycleGrams = varCycleGrams();
  const float grVuelta = varFeedRateGramsPerTurn();
  if(bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 4) , 7) == TRUE && app.dosage.calibrating == false)
  {
    if( ( ( (Events_ArduinoUNO) && (bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 3) , 3) == FALSE) && (cycleGrams != 0) )
      ||
      ( (Table_ArduinoUNO) && (rtc.breakCycle == 0) && (rtc.feedMinuteLatch != rtc.minute) && (cycleGrams != 0 && varCycleTime() != 0) ) )
      &&
      (  app.dosage.dosingActive == FALSE ) )
    {
      dosageTimeMs = (uint32_t)(grVuelta * cycleGrams);
      activeCycleGrams = cycleGrams;
      app.dosage.dosingActive = TRUE;
      rtc.feedMinuteLatch = rtc.minute;
      app.dosageRt.targetRevolutions = int(cycleGrams/grVuelta);
      app.dosageRt.revolutionFraction += (cycleGrams/grVuelta) - app.dosageRt.targetRevolutions;
      if(app.dosageRt.revolutionFraction >= 1.0){
        app.dosageRt.targetRevolutions += int(app.dosageRt.revolutionFraction);
        app.dosageRt.revolutionFraction = app.dosageRt.revolutionFraction - int(app.dosageRt.revolutionFraction);
      }
      acs.tiempoInicioInactividad = millis();
      Serial.print(F("Gramos por ciclo: "));
      Serial.println(cycleGrams);
      Serial.print(F("Gramos por vuelta: "));
      Serial.println(grVuelta);
      Serial.print(F("Numero de vueltas: "));
      Serial.println(app.dosageRt.targetRevolutions);
      VAR_WIRE_BYTE(VAR_ALARMS, 5) = 0;
      digitalWrite(X1, ON);
      Serial.println(F("Encender X1"));
      app.dosageRt.motorX1StartMs = millis();
      acs.tiempoAnteriorVueltas = millis();
      acs.previousAmpMaxX2 = 0;
      acs.previousAmpMinX2 = 10000;
      acs.previousAmpMaxX3 = 0;
      acs.previousAmpMinX3 = 10000;
    }
    if(cycleGrams != 0
      &&
      digitalRead(X3)==ON
      &&
      acs.inactividadFlag == false
      &&
      bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 3) , 4 ) == TRUE
      &&
      app.dosage.dosingActive == TRUE)
    {
      currentPeakCounter();
      if (acs.inactividadFlag) {
        Serial.println(F("Inactividad detectada"));
      }
    }
    if( digitalRead(X3)==ON ){
      uint32_t battery = varReadBatteryMicrovolts();
      if(battery < app.dosageRt.minBatteryInFeed){
        varPackBatteryWire(xbee.battBeforeFeed, battery);
      }
    }
    if ( ( millis() > ( app.dosageRt.motorX1StartMs + 500 ) ) &&
         ( ( digitalRead(X1) == ON ) && ( digitalRead(X2) == OFF ) ) &&
         ( app.dosage.dosingActive == TRUE ) &&
         (!bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 5) , 4))
        ) {
            uint8_t outX2 = calculatePercentX();
            (void)outX2;
            digitalWrite(X2, ON);
            Serial.println(F("Encender X2"));
            app.dosageRt.sprayerStartMs = millis();
          }
    if ( ( millis() > ( app.dosageRt.sprayerStartMs + 2000 ) ) &&
         ( ( digitalRead(X1) == ON ) && (digitalRead(X2) == ON) && ( digitalRead(X3) == OFF ) ) &&
         ( app.dosage.dosingActive == TRUE )
      ) {
          digitalWrite(X3, ON);
          Serial.println(F("Encender X3"));
          app.dosageRt.dosingStartMs = millis();
    }
    if(
        ((
          (( bitRead( VAR_WIRE_BYTE(VAR_SETTING_BYTES, 3) , 4 ) == FALSE ) && ( millis()-app.dosageRt.dosingStartMs >= dosageTimeMs)) ||
          (((bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 3) , 4 ) == TRUE) && (acs.nPicos >= app.dosageRt.targetRevolutions) && app.dosage.calibrating == false) || acs.inactividadFlag == true ))
        &&
        (( app.dosage.dosingActive == TRUE && (digitalRead(X3)==TRUE))))
        ||
        (acs.highCurrentX2 == TRUE)
        ||
        (acs.highCurrentX3 == TRUE)
      )
    {
      digitalWrite(X3 , OFF);
      if(app.dosageRt.dosingStartMs == 0){
        app.dosageRt.dosingTotalMs = 0;
      }else{
        app.dosageRt.dosingTotalMs = millis() - app.dosageRt.dosingStartMs;
      }
      Serial.print(F("Tiempo de dosificacion: "));
      Serial.println(app.dosageRt.dosingTotalMs);
      app.dosageRt.dosingStartMs = 0;
      delay(1000);
      digitalWrite(X2 , OFF);
      digitalWrite(X1 , OFF);
      acs.emptyHopperint = 0;
      acs.highestValueAmpX2 = 0;
      acs.highCurrentX2 = false;
      acs.highCurrentX3 = false;
      VAR_WIRE_BYTE(VAR_N_VUELTAS, 2) = acs.nPicos;
      VAR_WIRE_BYTE(VAR_T_VUELTAS, 5) = (byte)(app.dosageRt.dosingTotalMs & 0xFF);
      VAR_WIRE_BYTE(VAR_T_VUELTAS, 4) = (byte)((app.dosageRt.dosingTotalMs >> 8) & 0xFF);
      VAR_WIRE_BYTE(VAR_T_VUELTAS, 3) = (byte)((app.dosageRt.dosingTotalMs >> 16) & 0xFF);
      VAR_WIRE_BYTE(VAR_T_VUELTAS, 2) = (byte)((app.dosageRt.dosingTotalMs >> 24) & 0xFF);
      if((bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 3) , 4 ) == TRUE) && (acs.nPicos > 0 && acs.nPicos < app.dosageRt.targetRevolutions) ){
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 3),7,1);
      }else{
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 3),7,0);
      }
      app.dosage.dosingActive = FALSE;
      if(Events_ArduinoUNO || Table_ArduinoUNO)
      {
        if( bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 3) , 3) == TRUE ){
          bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 3) , 3 , 1);
          if(Table_ArduinoUNO){
            if(day() != rtc.dailyCompareDay){
              rtc.dailyCompareDay  = day();
              EEPROM.put(EEPROM_ADDR_COMPARING_DAY, rtc.dailyCompareDay);
              varSetAccumulatedGrams(0);
              varSetDosingId(0);
            }
            varSetDosingId(varDosingIdGet() + 1);
          }
        }
        if(day() != rtc.dailyCompareDay){
          rtc.dailyCompareDay  = day();
          EEPROM.put(EEPROM_ADDR_COMPARING_DAY, rtc.dailyCompareDay);
          varSetAccumulatedGrams(0);
        }
        varSetAccumulatedGrams(varAccumulatedGramsGet() + activeCycleGrams);
      }
      acs.nPicos = 0;
      acs.corriente_maxima = 0.0;
      acs.corriente_minima = 10.0;
      app.dosageRt.minBatteryInFeed = 0xFFFFFFFF;
      if(bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 5) , 6) == 1 || acs.inactividadFlag == TRUE){
        app.dosageRt.revolutionFraction = 0.0;
      }
      varPackBatteryWire(xbee.battAfterFeed, varReadBatteryMicrovolts());
    }
  }
  else if ((bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 4) , 7) == FALSE) && (digitalRead(X3) == ON))
  {
    digitalWrite(X1 , OFF);
    digitalWrite(X2 , OFF);
    digitalWrite(X3 , OFF);
    app.dosage.dosingActive = FALSE;
    if( bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 3) , 3) == TRUE ){
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 3) , 3 , 1);
    }
  }
  else if(bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 4) , 7) != TRUE){
    if((Events_ArduinoUNO) && (bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 3) , 3) == FALSE) && (cycleGrams !=0)){
      varSetCycleGrams(0);
      VAR_WIRE_BYTE(VAR_DOSED_GRAMS, 2) = 0;
      VAR_WIRE_BYTE(VAR_DOSED_GRAMS, 3) = 0;
      if( bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 3) , 3) == TRUE ){
        bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 3) , 3 , 1);
      }
    }
  }
  if(app.dosage.calibrating == false){
    acs.corriente_actual = convertADCtoAMP(ANALOGX2)/100.0;
    if(digitalRead(X1) == ON) sensorAmp(X1);
    if(digitalRead(X2) == ON && (millis()-app.dosageRt.sprayerStartMs >= 500)) sensorAmp(X2);
    if(digitalRead(X3) == ON && (millis()-app.dosageRt.dosingStartMs >= 500)) sensorAmp(X3);
  }
  if(acs.inactividadFlag == true ){
    bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 5) , 6, 1);
  }
}

void calibration(){
  if(bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 3) , 4) == TRUE && app.dosageRt.calibGrams != 0){
    if(app.dosage.calibrating == false){
      Serial.print(F("Gramos de calibracion: "));
      Serial.println(app.dosageRt.calibGrams);
      app.dosage.calibrating = true;
      acs.nPicos = 0;
      acs.inactividadFlag = false;
      acs.tiempoInicioInactividad = millis();
      digitalWrite(X2 , ON);
      delay(1000);
      digitalWrite(X3 , ON);
      app.dosageRt.calibStartMs = millis();
    }
    currentPeakCounter();
    if(acs.inactividadFlag == true){
      digitalWrite(X3 , OFF);
      app.dosageRt.calibDurationMs = millis() - app.dosageRt.calibStartMs;
      delay(1000);
      digitalWrite(X2 , OFF);
      if(acs.nPicos > 50){
        int new_feedrate = ((float)app.dosageRt.calibGrams / (float)acs.nPicos)*100;
        VAR_WIRE_BYTE(VAR_FEED_RATE, 2) = (byte)(new_feedrate >> 8);
        VAR_WIRE_BYTE(VAR_FEED_RATE, 3) = (byte)(new_feedrate & 0xFF);
        saveInEeprom(VAR_FEED_RATE);
        Serial.print(F("Cantidad de vueltas: "));
        Serial.println(acs.nPicos);
        Serial.print(F("Nuevo Feedrate: "));
        Serial.println(varFeedRateGramsPerTurn());
      }else{
        Serial.println(F("Cantidad de vueltas muy baja"));
      }
      uint8_t tCalibracion[4] = {0,0,0,0};
      tCalibracion[3] = (byte)(app.dosageRt.calibDurationMs & 0xFF);
      tCalibracion[2] = (byte)((app.dosageRt.calibDurationMs >> 8) & 0xFF);
      tCalibracion[1] = (byte)((app.dosageRt.calibDurationMs >> 16) & 0xFF);
      tCalibracion[0] = (byte)((app.dosageRt.calibDurationMs >> 24) & 0xFF);
      uint8_t nvBytes[2] = {0x00,0x00};
      nvBytes[0] = (byte)(acs.nPicos >> 8);
      nvBytes[1] = (byte)(acs.nPicos & 0xFF);
      xbee.payloadSize = 11;
      uint8_t cResponse[xbee.payloadSize] = {0xCB, 0x00, 0x01, nvBytes[0], nvBytes[1], VAR_WIRE_BYTE(VAR_FEED_RATE, 2), VAR_WIRE_BYTE(VAR_FEED_RATE, 3),
                                      tCalibracion[0],tCalibracion[1],tCalibracion[2],tCalibracion[3]};
      app.dosageRt.calibGrams = 0;
      acs.nPicos = 0;
      app.dosage.calibrating = false;
      create_frame(xbee.txFrame, cResponse, xbee.payloadSize, 0);
      print_frame("Calibration response: <",xbee.txFrame, xbee.payloadSize," >");
      send_frame(xbee.txFrame, xbee.payloadSize);
      delay(200);
    }
  }
}
