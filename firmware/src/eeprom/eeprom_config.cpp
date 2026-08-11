#include <Arduino.h>
#include <EEPROM.h>
#include "core/vars.h"
#include "variables/var_access.h"
#include "variables/var_io.h"
#include "eeprom/eeprom_config.h"
static void loadRtcFromEeprom(void) {
  VAR_VAL(VAR_RTC, 0) = EEPROM.read(0x000F);
  VAR_VAL(VAR_RTC, 1) = EEPROM.read(0x0010);
  VAR_VAL(VAR_RTC, 2) = EEPROM.read(0x0011);
  VAR_VAL(VAR_RTC, 3) = EEPROM.read(0x0012);
  VAR_VAL(VAR_RTC, 4) = EEPROM.read(0x0013);
  VAR_VAL(VAR_RTC, 5) = EEPROM.read(0x0014);
}
static void saveRtcToEeprom(void) {
  EEPROM.update(0x000F, VAR_VAL(VAR_RTC, 0));
  EEPROM.update(0x0010, VAR_VAL(VAR_RTC, 1));
  EEPROM.update(0x0011, VAR_VAL(VAR_RTC, 2));
  EEPROM.update(0x0012, VAR_VAL(VAR_RTC, 3));
  EEPROM.update(0x0013, VAR_VAL(VAR_RTC, 4));
  EEPROM.update(0x0014, VAR_VAL(VAR_RTC, 5));
}
static bool eepromSlotEmpty(int row) {
  if (transferVar[row][VAR_EEPROM] == 0) {
    return false;
  }
  uint16_t addr = varEepromAddr(row);
  uint8_t size = transferVar[row][VAR_SIZE];
  uint8_t ffCount = 0;
  for (uint8_t j = 0; j < size; j++) {
    if (EEPROM.read(addr + j) == 0xFF) {
      ffCount++;
    }
  }
  return ffCount == size;
}
void initConfig(void) {
  for (int i = 0; i < ROW; i++) {
    if (transferVar[i][VAR_EEPROM] == 0) {
      continue;
    }
    if (i == VAR_RTC) {
      loadRtcFromEeprom();
      if (EEPROM.read(0x000F) == 0xFF || EEPROM.read(0x0012) == 0xFF) {
        saveRtcToEeprom();
      }
      continue;
    }
    if (eepromSlotEmpty(i)) {
      continue;
    }
    uint16_t addr = varEepromAddr(i);
    uint8_t size = transferVar[i][VAR_SIZE];
    for (uint8_t j = 0; j < size; j++) {
      VAR_VAL(i, j) = EEPROM.read(addr + j);
    }
  }
}
void saveInEeprom(int row) {
  if (row < 0 || row >= ROW || transferVar[row][VAR_EEPROM] == 0) {
    return;
  }
  if (row == VAR_RTC) {
    saveRtcToEeprom();
    return;
  }
  uint16_t addr = varEepromAddr(row);
  uint8_t size = transferVar[row][VAR_SIZE];
  for (uint8_t i = 0; i < size; i++) {
    EEPROM.update(addr + i, VAR_VAL(row, i));
  }
}
void seedFactoryDefaults(void) {
  while (app.eeprom.ready == 0) {
    app.eeprom.ready = eeprom_is_ready();
  }
  if (EEPROM.read(0x20) == 0xDD) {
    app.eeprom.ready = 0;
    return;
  }
  for (int i = 0; i < ROW; i++) {
    if (transferVar[i][VAR_EEPROM] != 0) {
      saveInEeprom(i);
    }
  }
  EEPROM.update(0x20, 0xDD);
  VAR_VAL(VAR_RESET_EEPROM, 0) = 0xDD;
  app.eeprom.ready = 0;
}
void postInitConfig(void) {
  if (((VAR_WIRE_BYTE(VAR_SERVER, 2) == 0xFF) || (VAR_WIRE_BYTE(VAR_SERVER, 2) == 0x00)) &&
      ((VAR_WIRE_BYTE(VAR_SERVER, 3) == 0xFF) || (VAR_WIRE_BYTE(VAR_SERVER, 3) == 0x00)) &&
      ((VAR_WIRE_BYTE(VAR_SERVER, 4) == 0xFF) || (VAR_WIRE_BYTE(VAR_SERVER, 4) == 0x00)) &&
      ((VAR_WIRE_BYTE(VAR_SERVER, 5) == 0xFF) || (VAR_WIRE_BYTE(VAR_SERVER, 5) == 0x00)) &&
      ((VAR_WIRE_BYTE(VAR_SERVER, 6) == 0xFF) || (VAR_WIRE_BYTE(VAR_SERVER, 6) == 0x00)) &&
      ((VAR_WIRE_BYTE(VAR_SERVER, 7) == 0xFF) || (VAR_WIRE_BYTE(VAR_SERVER, 7) == 0x00)) &&
      ((VAR_WIRE_BYTE(VAR_SERVER, 8) == 0xFF) || (VAR_WIRE_BYTE(VAR_SERVER, 8) == 0x00)) &&
      ((VAR_WIRE_BYTE(VAR_SERVER, 9) == 0xFF) || (VAR_WIRE_BYTE(VAR_SERVER, 9) == 0x00))) {
    app.comm.sendUnicast = FALSE;
  } else {
    app.comm.sendUnicast = TRUE;
  }
  if (VAR_WIRE_BYTE(VAR_PROFILE, 2) == 0xFF) {
    VAR_WIRE_BYTE(VAR_PROFILE, 2) = 0x57;
  }
  if (Events_ArduinoUNO) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARM_MASK, 3), 3, 1);
  }
  if (VAR_WIRE_BYTE(VAR_ACS_TYPE, 2) == 0xFF) VAR_WIRE_BYTE(VAR_ACS_TYPE, 2) = 0;
  if (VAR_WIRE_BYTE(VAR_PROT_X1, 2) == 0xFF) VAR_WIRE_BYTE(VAR_PROT_X1, 2) = 1;
  if (VAR_WIRE_BYTE(VAR_PROT_X2, 2) == 0xFF) VAR_WIRE_BYTE(VAR_PROT_X2, 2) = 10;
  if (VAR_WIRE_BYTE(VAR_PROT_X3, 2) == 0xFF) VAR_WIRE_BYTE(VAR_PROT_X3, 2) = 1;
  if (VAR_WIRE_BYTE(VAR_DISCONNECTED, 2) == 0xFF) VAR_WIRE_BYTE(VAR_DISCONNECTED, 2) = 14;
  if (VAR_WIRE_BYTE(VAR_X3_WEAR_PCT, 2) == 0xFF) VAR_WIRE_BYTE(VAR_X3_WEAR_PCT, 2) = 30;
  if (VAR_WIRE_BYTE(VAR_X3_WEAR_EMA_ALPHA, 2) == 0xFF) VAR_WIRE_BYTE(VAR_X3_WEAR_EMA_ALPHA, 2) = 2;
  if (VAR_WIRE_BYTE(VAR_X2_WEAR_PCT, 2) == 0xFF) VAR_WIRE_BYTE(VAR_X2_WEAR_PCT, 2) = 30;
  if (VAR_WIRE_BYTE(VAR_X2_WEAR_EMA_ALPHA, 2) == 0xFF) VAR_WIRE_BYTE(VAR_X2_WEAR_EMA_ALPHA, 2) = 2;
  if (VAR_WIRE_BYTE(VAR_X2_BLADE_AMP_PCT, 2) == 0xFF) VAR_WIRE_BYTE(VAR_X2_BLADE_AMP_PCT, 2) = 25;
  if (VAR_WIRE_BYTE(VAR_X2_BLADE_MIN_SWING, 2) == 0xFF) VAR_WIRE_BYTE(VAR_X2_BLADE_MIN_SWING, 2) = 15;
  if ((VAR_WIRE_BYTE(VAR_EMPTY_HOPPER, 2) == 0xFF) && (VAR_WIRE_BYTE(VAR_EMPTY_HOPPER, 3) == 0xFF)) {
    VAR_WIRE_BYTE(VAR_EMPTY_HOPPER, 2) = 0x01;
    VAR_WIRE_BYTE(VAR_EMPTY_HOPPER, 3) = 0x36;
  }
  if ((VAR_WIRE_BYTE(VAR_LOW_BATTERY, 2) == 0xFF) && (VAR_WIRE_BYTE(VAR_LOW_BATTERY, 3) == 0xFF)) {
    VAR_WIRE_BYTE(VAR_LOW_BATTERY, 2) = 0x2C;
    VAR_WIRE_BYTE(VAR_LOW_BATTERY, 3) = 0xEC;
  }
  {
    uint8_t resetCount = 0;
    EEPROM.get(varEepromAddr(VAR_RESET_COUNT), resetCount);
    Serial.println(resetCount);
    if (resetCount == 0xFF) {
      resetCount = 0;
    }
    resetCount++;
    VAR_WIRE_BYTE(VAR_RESET_COUNT, 2) = resetCount;
    EEPROM.put(varEepromAddr(VAR_RESET_COUNT), resetCount);
  }
  EEPROM.get(EEPROM_ADDR_COMPARING_DAY, rtc.dailyCompareDay);
  EEPROM.get(EEPROM_ADDR_AUX_DAY, rtc.persistedDay);
  {
    uint16_t dosingId = 0;
    EEPROM.get(varEepromAddr(VAR_ID_DOSAGE), dosingId);
    if (dosingId == 0xFFFF) {
      dosingId = 0;
    }
    varSetDosingId(dosingId);
  }
  {
    uint16_t timeAlarmEeprom = 0;
    EEPROM.get(varEepromAddr(VAR_TIME_ALARM), timeAlarmEeprom);
    if ((timeAlarmEeprom != 0x0000) && (timeAlarmEeprom != 0xFFFF)) {
      uint16_t timeAlarmAddr = varEepromAddr(VAR_TIME_ALARM);
      for (uint8_t i = 0; i <= 1; i++) {
        VAR_WIRE_BYTE(VAR_TIME_ALARM, 2 + i) = EEPROM.read(timeAlarmAddr + i);
      }
    }
  }
  {
    uint32_t accGrams = 0;
    EEPROM.get(varEepromAddr(VAR_ACCUMULATED_GRAMS), accGrams);
    if (accGrams > 540000) {
      accGrams = 0;
    }
    varSetAccumulatedGrams(accGrams);
  }
  if ((VAR_WIRE_BYTE(VAR_PERCENT_MOTOR, 2) <= 0) || (VAR_WIRE_BYTE(VAR_PERCENT_MOTOR, 2) > 100)) {
    VAR_WIRE_BYTE(VAR_PERCENT_MOTOR, 2) = 100;
    saveInEeprom(VAR_PERCENT_MOTOR);
  }
  for (uint8_t i = 0; i < 4; i++) {
    xbee.comm.alarmsSnapshot[i] = VAR_WIRE_BYTE(VAR_ALARMS, 2 + i);
  }
  motorWearInitAll();
  x2BladeInit();
}
