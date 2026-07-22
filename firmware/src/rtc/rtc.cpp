#include <Arduino.h>
#include <math.h>
#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include "core/vars.h"
void Internal_Clock(void)
{
  rtcSyncFromTimeLib();
  rtcPushCacheToWire();

  if(rtc.minute == 0)
  {
    rtc.breakCycle = 0;
  }
  else
  {
    rtc.breakCycle = rtc.minute % varCycleTime();
  }

  if(rtc.hour != rtc.shadowHour)
  {
    saveInEeprom(VAR_RTC);
    ChangeGramsAndTime();

    if ( (Table_ArduinoUNO) && (varCycleTime() == 0x3C) )
    {
      rtc.breakCycle = 0;
      rtc.feedMinuteLatch = 0xFF;
    }

    rtc.shadowHour = rtc.hour;
  }
  if(rtc.day != rtc.shadowDay)
  {
    saveInEeprom(VAR_RTC);
    rtc.shadowDay = rtc.day;
  }
  if(rtc.month != rtc.shadowMonth)
  {
    saveInEeprom(VAR_RTC);
    rtc.shadowMonth = rtc.month;
  }
  if(rtc.year != rtc.shadowYear)
  {
    saveInEeprom(VAR_RTC);
    rtc.shadowYear = rtc.year;
  }

  return;
}
void Print_Hour(void)
{
  if (rtc.second != rtc.shadowSecond)
  {
    if ((bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 2) , 7) == TRUE)){
      if(app.dosage.rtcSynced == TRUE){
        digitalWrite(LED, !digitalRead(LED));
      }
    }else
    {
      digitalWrite(LED, OFF);
    }
   if(bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 4) , 7) == FALSE)
    {
      digitalWrite(X0_2 , !digitalRead(X0_2));
      digitalWrite(X0_1 , OFF);
      rtc.ledTimerStart = 0;
    }
    else
    {
      digitalWrite(X0_1 , !digitalRead(X0_1));
      digitalWrite(X0_2 , OFF);
      rtc.ledTimerStart = 0;
    }
    rtc.shadowSecond = rtc.second;
  }
  return;
}
uint8_t BCD_TO_HEXA(uint8_t num) {
    uint8_t byte2 = num;
    if (byte2 <= 153 && byte2 >= 144) {
        byte2 = byte2 - (6 * 9);
    }
    else if (byte2 <= 137 && byte2 >= 128) {
        byte2 = byte2 - (6 * 8);
    }
    else if (byte2 <= 121 && byte2 >= 112) {
        byte2 = byte2 - (6 * 7);
    }
    else if (byte2 <= 105 && byte2 >= 96) {
        byte2 = byte2 - (6 * 6);
    }
    else if (byte2 <= 89 && byte2 >= 80) {
        byte2 = byte2 - (6 * 5);
    }
    else if (byte2 <= 73 && byte2 >= 64) {
        byte2 = byte2 - (6 * 4);
    }
    else if (byte2 <= 57 && byte2 >= 48) {
        byte2 = byte2 - (6 * 3);
    }
    else if (byte2 <= 41 && byte2 >= 32) {
        byte2 = byte2 - (6 * 2);
    }
    else if (byte2 <= 25 && byte2 >= 16) {
        byte2 = byte2 - (6 * 1);
    } else {
        byte2 = byte2 + (6 * 0);
    }
    num = 0;
    return byte2;
}
uint8_t HEXA_TO_BCD(uint8_t byte)  {
    int byte3 = byte;
    if (byte3 <= 19 && byte3 >= 10) {
        byte3 = byte3 + (6 * 1);
    } else if (byte3 <= 29 && byte3 >= 20) {
        byte3 = byte3 + (6 * 2);
    } else if (byte3 <= 39 && byte3 >= 30) {
        byte3 = byte3 + (6 * 3);
    } else if (byte3 <= 49 && byte3 >= 40) {
        byte3 = byte3 + (6 * 4);
    } else if (byte3 <= 59 && byte3 >= 50) {
        byte3 = byte3 + (6 * 5);
    } else if (byte3 <= 69 && byte3 >= 60) {
        byte3 = byte3 + (6 * 6);
    } else if (byte3 <= 79 && byte3 >= 70) {
        byte3 = byte3 + (6 * 7);
    } else if (byte3 <= 89 && byte3 >= 80) {
        byte3 = byte3 + (6 * 8);
    } else if (byte3 <= 99 && byte3 >= 90) {
        byte3 = byte3 + (6 * 9);
    } else {
        byte3 = byte3 + (6 * 0);
    }
    return byte3;
}
void ChangeGramsAndTime (void)   
{
  if (Table_ArduinoUNO)
  {
    int tableRow;
    uint8_t hourIdx;
    if(rtc.hour <= 11)
    {
      tableRow = VAR_AM_TABLE;
      hourIdx = rtc.hour;
    }
    else if((rtc.hour > 11) && (rtc.hour <= 24))
    {
      tableRow = VAR_PM_TABLE;
      hourIdx = rtc.hour - 12;
    }
    else
    {
      return;
    }
    varSetCycleGrams(varTableGrams(tableRow, hourIdx));
    varSetCycleTime(varTableCycleTime(tableRow, hourIdx));
    VAR_WIRE_BYTE(VAR_DOSED_GRAMS, 2) = VAR_WIRE_BYTE(VAR_CYCLE_GRAMS, 2);
    VAR_WIRE_BYTE(VAR_DOSED_GRAMS, 3) = VAR_WIRE_BYTE(VAR_CYCLE_GRAMS, 3);
  }
} 
uint8_t calculatePercentX(void)
{
  float result = VAR_WIRE_BYTE(VAR_PERCENT_MOTOR, 2) * 255;
  result = result / 100;
  uint8_t result2 = result;
  result = result - result2;
  if (result >= 0.5)
    result2 += 1;
  return (uint8_t) result2;
}
void forTimeAlam(void)
{
  uint32_t minutesNow     = 0;
  uint32_t minutesConfig  = ( (VAR_WIRE_BYTE(VAR_TIME_ALARM, 2) * 0x100) + VAR_WIRE_BYTE(VAR_TIME_ALARM, 3) );
  uint32_t ProductMinutes = minutesConfig * 60000;
  minutesNow = millis();
  if ( minutesNow >= (xbee.comm.lastActivityMs + ProductMinutes) )
  {
    if ( (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 4) , 3) == 1) && (bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 4) , 3) == 0) )
    {
      bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 4) , 3 , 1);
    }         
  }
  return;
}
