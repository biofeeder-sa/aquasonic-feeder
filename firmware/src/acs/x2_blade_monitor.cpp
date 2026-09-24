#include "acs/x2_blade_monitor.h"
#include "acs/acs_sensor.h"
#include "core/vars.h"
#include "eeprom/eeprom_config.h"

static uint8_t suspectStreak = 0;
static bool bladeLatched = false;

static void setBladeAlarmBit(bool active) {
  if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, X2_BLADE_MASK_BYTE), X2_BLADE_MASK_BIT) == 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, X2_BLADE_ALARM_BYTE), X2_BLADE_ALARM_BIT, 0);
    return;
  }
  bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, X2_BLADE_ALARM_BYTE), X2_BLADE_ALARM_BIT, active ? 1 : 0);
}

static void clearBladeAlarmState(void) {
  suspectStreak = 0;
  bladeLatched = false;
  setBladeAlarmBit(false);
}

void x2BladeInit(void) {
  suspectStreak = 0;
  bladeLatched = false;

  if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, X2_BLADE_MASK_BYTE), X2_BLADE_MASK_BIT) == 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARM_MASK, X2_BLADE_MASK_BYTE), X2_BLADE_MASK_BIT, 1);
    saveInEeprom(VAR_ALARM_MASK);
  }
}

void x2BladeApplyAlarmMask(void) {
  if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, X2_BLADE_MASK_BYTE), X2_BLADE_MASK_BIT) == 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, X2_BLADE_ALARM_BYTE), X2_BLADE_ALARM_BIT, 0);
  }
}

void x2BladeResetStreak(void) {
  suspectStreak = 0;
}

void x2BladeOnSample(uint16_t ampX2Ca, uint16_t ampX3Ca, bool x3Settled) {
  if (bladeLatched || !x3Settled) {
    return;
  }

  const uint16_t disconnected = VAR_WIRE_BYTE(VAR_DISCONNECTED, 2);
  if (ampX2Ca <= disconnected || ampX2Ca >= X2_BLADE_MAX_CA) {
    suspectStreak = 0;
    return;
  }

  /* Dosificador en X2 y aspersor en X3: motores invertidos, no aspa suelta. */
  if (ampX3Ca >= MOTOR_SWAP_SPRAYER_MIN_CA) {
    suspectStreak = 0;
    return;
  }

  if (suspectStreak < 255) {
    suspectStreak++;
  }
  if (suspectStreak < X2_BLADE_CONSECUTIVE) {
    return;
  }

  bladeLatched = true;
  setBladeAlarmBit(true);
#if MOTOR_WEAR_DEBUG
  Serial.print(F("[X2BLADE] latched X2="));
  Serial.print(ampX2Ca);
  Serial.print(F("cA X3="));
  Serial.println(ampX3Ca);
#endif
}

bool x2BladeHandleAckWrite(uint8_t command) {
  if (command == X2_BLADE_CMD_ACK) {
    clearBladeAlarmState();
    return true;
  }
  return false;
}

bool x2BladeAlarmActive(void) {
  return bladeLatched;
}
