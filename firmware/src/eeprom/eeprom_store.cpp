#include <Arduino.h>
#include <math.h>
#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include "core/vars.h"
#include "eeprom/eeprom_config.h"
#include "variables/var_access.h"
void Start_EEPROM(void)
{
  seedFactoryDefaults();
  return;
}
void Update_EEPROM(void)
{
  if(app.dosage.dosingActive != TRUE)
  {
    if(( app.dosage.changeTableAm == TRUE) || (app.dosage.rtcSynced == FALSE))
    {
         app.dosage.changeTableAm = FALSE;
         ChangeGramsAndTime();
    }
    if((app.dosage.changeTablePm == TRUE) || (app.dosage.rtcSynced == FALSE))
    {
        app.dosage.changeTablePm = FALSE; 
        ChangeGramsAndTime();
    }
    if(app.eeprom.hourLoaded == FALSE)
    {
      uint16_t rtcBase = varEepromAddr(VAR_RTC);
      VAR_WIRE_BYTE(VAR_RTC, 2) = EEPROM.read(rtcBase);
      VAR_WIRE_BYTE(VAR_RTC, 3) = 0;
      VAR_WIRE_BYTE(VAR_RTC, 4) = 0;
      VAR_WIRE_BYTE(VAR_RTC, 5) = EEPROM.read(rtcBase + 3);
      VAR_WIRE_BYTE(VAR_RTC, 6) = EEPROM.read(rtcBase + 4);
      VAR_WIRE_BYTE(VAR_RTC, 7) = EEPROM.read(rtcBase + 5);
      app.eeprom.hourLoaded = TRUE;
    }
  }
  return;
}
