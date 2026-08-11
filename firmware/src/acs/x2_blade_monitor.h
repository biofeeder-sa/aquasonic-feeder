#ifndef AQUASONIC_X2_BLADE_MONITOR_H_
#define AQUASONIC_X2_BLADE_MONITOR_H_

#include <Arduino.h>
#include <stdint.h>

/* VAR_ALARMS valor byte índice 2, bit 4 */
#define X2_BLADE_ALARM_BYTE      2
#define X2_BLADE_ALARM_BIT       4
#define X2_BLADE_MASK_BYTE       0
#define X2_BLADE_MASK_BIT        4

#define X2_BLADE_CMD_ACK         0x01u

void x2BladeInit(void);
void x2BladeApplyAlarmMask(void);
void x2BladeOnCycleEnd(uint16_t cycleImax, uint8_t nPicos, bool inactivity, bool revolutionMode,
                       float currentSwingA, bool hadProtection, bool hadX2Disconnected);
bool x2BladeHandleAckWrite(uint8_t command);
bool x2BladeAlarmActive(void);

#endif
