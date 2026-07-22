#ifndef AQUASONIC_VAR_CALLBACKS_H_
#define AQUASONIC_VAR_CALLBACKS_H_
#include <stdint.h>
/*
 * Tabla de callbacks por fila (estilo hub).
 * NULL en kVarWriteHandlers[row] => copy + saveInEeprom si WR/EEPROM lo permiten.
 */
typedef bool (*VarWriteHandlerFn)(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte);
extern const VarWriteHandlerFn kVarWriteHandlers[];
bool varWriteCopySave(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte);
bool varWriteCopyOnly(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte);
bool varWriteAmTable(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte);
bool varWritePmTable(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte);
bool varWriteAlarmMask(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte);
bool varWritePercentMotor(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte);
bool varWriteRtc(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte);
bool varWritePanId(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte);
bool varWriteCycleGrams(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte);
#endif /* AQUASONIC_VAR_CALLBACKS_H_ */
