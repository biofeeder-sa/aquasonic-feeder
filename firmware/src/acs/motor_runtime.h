#ifndef AQUASONIC_MOTOR_RUNTIME_H_
#define AQUASONIC_MOTOR_RUNTIME_H_

#include <Arduino.h>
#include <stdint.h>

void motorRuntimeInit(void);
void motorRuntimeOnCycleEnd(uint32_t x2RunMs, uint32_t x3RunMs, bool emptyHopper);
void motorRuntimeResetX2(void);
void motorRuntimeResetX3(void);

uint32_t motorRuntimeX2Seconds(void);
uint32_t motorRuntimeX3Seconds(void);
void motorRuntimeWriteX2(uint32_t seconds);
void motorRuntimeWriteX3(uint32_t seconds);

#endif
