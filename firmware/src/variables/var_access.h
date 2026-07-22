#ifndef AQUASONIC_VAR_ACCESS_H_
#define AQUASONIC_VAR_ACCESS_H_

#include "variables/GlobalVariables.h"

/* Acceso estilo wire legacy: [0,1]=ID, [2+]=valor (mapeado a VAR_VALUE) */
#define VAR_WIRE_BYTE(row, idx) \
  ((idx) < 2 ? transferVar[(row)][(idx)] : transferVar[(row)][VAR_VALUE + ((uint8_t)(idx) - 2)])

#define VAR_WIRE_LEN(row) (2u + transferVar[(row)][VAR_SIZE])

#define VAR_VAL(row, n) transferVar[(row)][VAR_VALUE + (n)]

static inline uint16_t varId16(int row) {
  return ((uint16_t)transferVar[row][0] << 8) | transferVar[row][1];
}

static inline uint16_t varEepromAddr(int row) {
  return ((uint16_t)transferVar[row][VAR_EEPROM_ADDRESS] << 8)
       | transferVar[row][VAR_EEPROM_ADDRESS + 1];
}

static inline int findRowById(uint8_t msb, uint8_t lsb) {
  uint16_t id = ((uint16_t)msb << 8) | lsb;
  for (int j = 0; j < ROW; j++) {
    if (varId16(j) == id) {
      return j;
    }
  }
  return -1;
}

static inline uint16_t varU16Wire(int row, uint8_t wireIdx2) {
  return ((uint16_t)VAR_WIRE_BYTE(row, wireIdx2) << 8)
       | VAR_WIRE_BYTE(row, wireIdx2 + 1);
}

static inline void varSetU16Wire(int row, uint8_t wireIdx2, uint16_t value) {
  VAR_WIRE_BYTE(row, wireIdx2) = (uint8_t)(value >> 8);
  VAR_WIRE_BYTE(row, wireIdx2 + 1) = (uint8_t)(value & 0xFF);
}

static inline uint16_t varCycleGrams(void) {
  return varU16Wire(VAR_CYCLE_GRAMS, 2);
}

static inline void varSetCycleGrams(uint16_t grams) {
  varSetU16Wire(VAR_CYCLE_GRAMS, 2, grams);
}

static inline uint8_t varCycleTime(void) {
  return VAR_WIRE_BYTE(VAR_CYCLE_TIME, 2);
}

static inline void varSetCycleTime(uint8_t minutes) {
  VAR_WIRE_BYTE(VAR_CYCLE_TIME, 2) = minutes;
}

static inline float varFeedRateGramsPerTurn(void) {
  return varU16Wire(VAR_FEED_RATE, 2) / 100.0f;
}

static inline uint32_t varAccumulatedGramsGet(void) {
  return ((uint32_t)VAR_VAL(VAR_ACCUMULATED_GRAMS, 0) << 24)
       | ((uint32_t)VAR_VAL(VAR_ACCUMULATED_GRAMS, 1) << 16)
       | ((uint32_t)VAR_VAL(VAR_ACCUMULATED_GRAMS, 2) << 8)
       | VAR_VAL(VAR_ACCUMULATED_GRAMS, 3);
}

static inline uint16_t varDosingIdGet(void) {
  return varU16Wire(VAR_ID_DOSAGE, 2);
}

/* Tabla AM/PM: slot = [cycle_time, grams_MSB, grams_LSB] */
static inline uint8_t varTableSlotOffset(uint8_t hourIdx) {
  return (uint8_t)(VAR_VALUE + hourIdx * 3u);
}

static inline uint16_t varTableGrams(int tableRow, uint8_t hourIdx) {
  uint8_t off = varTableSlotOffset(hourIdx);
  return ((uint16_t)transferVar[tableRow][off + 1] << 8)
       | transferVar[tableRow][off + 2];
}

static inline uint8_t varTableCycleTime(int tableRow, uint8_t hourIdx) {
  return transferVar[tableRow][varTableSlotOffset(hourIdx)];
}

/* Perfiles de dosificacion */
#define Events_ArduinoUNO     (VAR_WIRE_BYTE(VAR_PROFILE, 2) == 0x57)
#define Table_ArduinoUNO      (VAR_WIRE_BYTE(VAR_PROFILE, 2) == 0x07)
#define FeedRate_Dosage       (Table_ArduinoUNO || Events_ArduinoUNO)

void varSetAccumulatedGrams(uint32_t grams);
void varSetDosingId(uint16_t id);
int32_t varBatteryTaraMicrovolts(void);
uint32_t varReadBatteryMicrovolts(void);
void varPackBatteryValue(uint8_t* value4, uint32_t microvolts);
void varPackBatteryWire(uint8_t* wire6, uint32_t microvolts);

#endif /* AQUASONIC_VAR_ACCESS_H_ */
