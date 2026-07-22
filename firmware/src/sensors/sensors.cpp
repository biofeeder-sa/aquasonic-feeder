#include <Arduino.h>
#include <math.h>
#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include "core/vars.h"

void Sense_Voltajes(void)
{
  uint32_t battery = varReadBatteryMicrovolts();
  varPackBatteryValue(&VAR_WIRE_BYTE(VAR_BATTERY, 2), battery);
  uint16_t valLowBattery = varU16Wire(VAR_LOW_BATTERY, 2);
  if ((battery <= (uint32_t)valLowBattery*10) && (app.sensor.alarmLowVoltage == FALSE) && (app.dosage.dosingActive == FALSE))
  {
    Serial.println("Bateria baja");
      bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 3) , 4 , 1);
      app.sensor.alarmLowVoltage = TRUE;
  }
  else if ((battery <= 148000) && (battery >= 120000) && (app.sensor.alarmLowVoltage == TRUE))
  {
    Serial.println("Baterial nivel normal");
    bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 3) , 4 , 0);
    app.sensor.alarmLowVoltage = FALSE;
  }
  return;
}
/************************************************************************/
/*          ACTUALIZACION DE LA MEMORIA EEPROM          */
unsigned long readCount()
{
  unsigned long Count = 0;
  unsigned char i;
  digitalWrite(ADSK, LOW);
  unsigned int counter = 0;
  while(digitalRead(ADDO)){
    counter++;
    if (counter > 5000)
      break;
  }
  for(i=0;i<24;i++)
  {
     digitalWrite(ADSK, HIGH);
     Count = Count << 1;
     digitalWrite(ADSK, LOW);
     if(digitalRead(ADDO)) Count++;
  } //end for
  digitalWrite(ADSK, HIGH);
  Count = Count^0x800000;
  digitalWrite(ADSK, LOW);
  return(Count);
}
/************************************************************************/
/*              FUNCION ASIGNACION DE NUEVO SERVIDOR                    */
