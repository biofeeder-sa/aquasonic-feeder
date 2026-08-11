#ifndef AQUASONIC_MOTOR_WEAR_MONITOR_H_
#define AQUASONIC_MOTOR_WEAR_MONITOR_H_

#include <Arduino.h>
#include <stdint.h>

#ifndef MOTOR_WEAR_DEBUG
#define MOTOR_WEAR_DEBUG 1
#endif

/* VAR_ALARMS valor byte índice 2: desgaste (0-1) y cambio de motor (2-3) */
#define X2_WEAR_ALARM_BYTE       2
#define X2_WEAR_ALARM_BIT        0
#define X2_WEAR_MASK_BYTE        0
#define X2_WEAR_MASK_BIT         0

#define X3_WEAR_ALARM_BYTE       2
#define X3_WEAR_ALARM_BIT        1
#define X3_WEAR_MASK_BYTE        0
#define X3_WEAR_MASK_BIT         1

#define X2_MOTOR_CHANGE_ALARM_BIT  2
#define X2_MOTOR_CHANGE_MASK_BIT   2

#define X3_MOTOR_CHANGE_ALARM_BIT  3
#define X3_MOTOR_CHANGE_MASK_BIT   3

#define MOTOR_WEAR_CMD_RESET     0x01u
#define MOTOR_WEAR_CLEAR_STREAK  2u

void motorWearInitAll(void);
void motorWearApplyAlarmMask(void);

void x2WearOnCycleEnd(uint16_t cycleImax, uint32_t dosingMs, bool hadProtection,
                      bool hadX2Disconnected, bool hadX3Disconnected, bool hadEmptyHopper);
void x3WearOnCycleEnd(uint16_t cycleImax, uint32_t dosingMs, bool hadProtection,
                      bool hadX2Disconnected, bool hadX3Disconnected, bool hadEmptyHopper);
void x2WearSyncAlarm(bool hadProtection);
void x3WearSyncAlarm(bool hadProtection);

bool x2WearHandleMotorResetWrite(uint8_t command);
bool x3WearHandleMotorResetWrite(uint8_t command);

uint16_t x2WearBaselineRef(void);
uint16_t x2WearBaselineSlow(void);
uint8_t x2WearHealthyCount(void);
bool x2WearAlarmActive(void);

uint16_t x3WearBaselineRef(void);
uint16_t x3WearBaselineSlow(void);
uint8_t x3WearHealthyCount(void);
bool x3WearAlarmActive(void);

/* Compatibilidad con codigo existente */
#define x3WearInit            motorWearInitAll
#define x3WearApplyAlarmMask  motorWearApplyAlarmMask
#define x3WearHandleMotorResetWrite x3WearHandleMotorResetWrite
#define x3UpdateDoserAlarms   x3WearSyncAlarm
#define X3_WEAR_CMD_RESET_MOTOR MOTOR_WEAR_CMD_RESET
#define X3_WEAR_CLEAR_CYCLE_STREAK MOTOR_WEAR_CLEAR_STREAK
#define X3_WEAR_DEBUG MOTOR_WEAR_DEBUG

#endif
