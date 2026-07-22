#include <Arduino.h>
#include <math.h>
#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include "core/vars.h"
void activarPWM(int valorPWM, int output) {
  analogWrite(output, valorPWM); // Establece el valor de PWM en el pin de salida
}
void InitLed(void)
{ 
  if ((rtc.ledTimerStart == 0) && (app.dosage.rtcSynced == FALSE))
  {
    rtc.ledTimerStart = millis();
  }
  else if ((rtc.ledTimerStart != 0) && (app.dosage.rtcSynced == FALSE))
  {
    if (rtc.ledTimerNow >= (rtc.ledTimerStart + 1000))
    {
      if(bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 4) , 7) == FALSE)
      {
        Serial.println("LED_STANDBY");
        digitalWrite(LED_STANDBY , !digitalRead(LED_STANDBY));
        digitalWrite(LED_ACTIVE , LOW);
        rtc.ledTimerStart = 0;
      }
      else
      {
        Serial.println("LED_ACTIVE");
        digitalWrite(LED_ACTIVE , !digitalRead(LED_ACTIVE));
        digitalWrite(LED_STANDBY , LOW);
        rtc.ledTimerStart = 0;
      }      
    }
    rtc.ledTimerNow = millis();
  }
}
void motorStarting (int _delay, int _interval, int _motor , uint8_t percent){
  int interval = _interval;
  int __delay = _delay;
  int motor = _motor;
  int pulse = 0;
  int res = 0;
  for(int i = 1; i <= interval; i++){
    //Serial.print("intervalo: ");
    //Serial.print(i);
    //Serial.print(" ");
    res = ( (percent / interval) * i );
    if( i >= interval ){
        res = percent;
    }
    //Serial.print("pulso: ");
    //Serial.println(res);
    analogWrite(motor, res);
    delay(__delay);
  }  
  analogWrite(motor, res);
  delay(__delay);
  return;
}
