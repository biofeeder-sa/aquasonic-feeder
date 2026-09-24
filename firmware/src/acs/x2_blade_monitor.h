#ifndef AQUASONIC_X2_BLADE_MONITOR_H_
#define AQUASONIC_X2_BLADE_MONITOR_H_

#include <Arduino.h>
#include <stdint.h>

/* VAR_WIRE_BYTE índice 2 = primer byte de valor (el 0 y el 1 son el ID). */
#define X2_BLADE_ALARM_BYTE      2
#define X2_BLADE_ALARM_BIT       4
#define X2_BLADE_MASK_BYTE       2
#define X2_BLADE_MASK_BIT        4

/* Aspersor con aspa ≥ 4 A; sin aspa ≈ 0.5–1.0 A. Umbral fijo, en centiamperios. */
#define X2_BLADE_MAX_CA          200u
#define X2_BLADE_CONSECUTIVE     3u

void x2BladeInit(void);
void x2BladeApplyAlarmMask(void);
void x2BladeResetStreak(void);
void x2BladeOnSample(uint16_t ampX2Ca, uint16_t ampX3Ca, bool x3Settled);
bool x2BladeAlarmActive(void);

#endif
