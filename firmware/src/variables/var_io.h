#ifndef AQUASONIC_VAR_IO_H_
#define AQUASONIC_VAR_IO_H_
#include <stdint.h>
void appendVarWire(int row, uint8_t* buf, int& counter);
/* Copia valor wire desde trama XBee a la matriz. Retorna bytes de valor escritos. */
uint8_t copyVarWireFromFrame(int row, uint8_t posLSB, const uint8_t* frame);
/*
 * Escribe variable identificada por fila (lookup previo con findRowById).
 * Retorna true si se procesÃ³; countByte = tamaÃ±o del valor en wire (SIZE).
 */
bool applyVarWriteFromXbee(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte);
#endif /* AQUASONIC_VAR_IO_H_ */
