#include <Arduino.h>
#include <EEPROM.h>
#include "core/config.h"
#include "core/vars.h"
#include "variables/var_access.h"
#include "variables/var_callbacks.h"
#include "variables/var_io.h"
#include "eeprom/eeprom_config.h"
#include "rtc/rtc.h"
void appendVarWire(int row, uint8_t* buf, int& counter) {
  for (uint8_t j = 0; j < VAR_WIRE_LEN(row); j++) {
    buf[counter++] = VAR_WIRE_BYTE(row, j);
  }
}
uint8_t copyVarWireFromFrame(int row, uint8_t posLSB, const uint8_t* frame) {
  for (int i = 2; i < (int)VAR_WIRE_LEN(row); ++i) {
    VAR_WIRE_BYTE(row, i) = frame[posLSB + i];
  }
  return transferVar[row][VAR_SIZE];
}
bool applyVarWriteFromXbee(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  if (row < 0 || row >= ROW) {
    return false;
  }
  VarWriteHandlerFn handler = kVarWriteHandlers[row];
  if (handler != NULL) {
    return handler(row, posLSB, frame, countByte);
  }
  if (transferVar[row][VAR_WR] == 0) {
    return false;
  }
  copyVarWireFromFrame(row, posLSB, frame);
  if (transferVar[row][VAR_EEPROM] != 0) {
    saveInEeprom(row);
  }
  countByte = transferVar[row][VAR_SIZE];
  return true;
}
void varSetAccumulatedGrams(uint32_t grams) {
  VAR_VAL(VAR_ACCUMULATED_GRAMS, 0) = (uint8_t)(grams >> 24);
  VAR_VAL(VAR_ACCUMULATED_GRAMS, 1) = (uint8_t)(grams >> 16);
  VAR_VAL(VAR_ACCUMULATED_GRAMS, 2) = (uint8_t)(grams >> 8);
  VAR_VAL(VAR_ACCUMULATED_GRAMS, 3) = (uint8_t)(grams);
  EEPROM.put(varEepromAddr(VAR_ACCUMULATED_GRAMS), grams);
}
void varSetDosingId(uint16_t id) {
  varSetU16Wire(VAR_ID_DOSAGE, 2, id);
  EEPROM.put(varEepromAddr(VAR_ID_DOSAGE), id);
}
int32_t varBatteryTaraMicrovolts(void) {
  return (int32_t)(
      ((int32_t)BCD_TO_HEXA(VAR_WIRE_BYTE(VAR_BATTERY_TARA, 2)) * 1000000L)
    + ((int32_t)BCD_TO_HEXA(VAR_WIRE_BYTE(VAR_BATTERY_TARA, 3)) * 10000L)
    + ((int32_t)BCD_TO_HEXA(VAR_WIRE_BYTE(VAR_BATTERY_TARA, 4)) * 100L)
    + (int32_t)BCD_TO_HEXA(VAR_WIRE_BYTE(VAR_BATTERY_TARA, 5)));
}
uint32_t varReadBatteryMicrovolts(void) {
  uint32_t raw = (uint32_t)map(analogRead(CH_BATTERY), 0, 1023, 0, 200000);
  int32_t tara = varBatteryTaraMicrovolts();
  if ((int32_t)raw <= tara) {
    return 0;
  }
  return raw - (uint32_t)tara;
}
void varPackBatteryValue(uint8_t* value4, uint32_t microvolts) {
  uint32_t aux;
  aux = microvolts / 1000000UL;
  value4[0] = HEXA_TO_BCD((uint8_t)aux);
  aux = (microvolts / 10000UL) % 100UL;
  value4[1] = HEXA_TO_BCD((uint8_t)aux);
  aux = (microvolts % 10000UL) / 100UL;
  value4[2] = HEXA_TO_BCD((uint8_t)aux);
  aux = microvolts % 100UL;
  value4[3] = HEXA_TO_BCD((uint8_t)aux);
}
void varPackBatteryWire(uint8_t* wire6, uint32_t microvolts) {
  wire6[0] = 0x0D;
  wire6[1] = 0x86;
  varPackBatteryValue(wire6 + 2, microvolts);
}
