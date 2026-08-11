#include "acs/x2_blade_monitor.h"
#include "acs/motor_wear_monitor.h"
#include "core/vars.h"
#include "eeprom/eeprom_config.h"
#include "variables/var_io.h"

/* Calibrado con corriente_aspersor_80792: full~576 cA, empty+aspa~345 cA, aspa off~66 cA */
#define BLADE_AMP_PCT_DEFAULT      25u
#define BLADE_AMP_PCT_MIN          10u
#define BLADE_AMP_PCT_MAX          45u
#define BLADE_MIN_SWING_CA_DEFAULT 15u
#define BLADE_MIN_SWING_CA_MAX     100u
#define BLADE_CONSECUTIVE          2u
#define BLADE_CLEAR_STREAK         2u
#define BLADE_CLEAR_PCT            50u

static uint8_t suspectStreak = 0;
static uint8_t clearStreak = 0;
static bool bladeLatched = false;

static void setBladeAlarmBit(bool active) {
  if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, X2_BLADE_MASK_BYTE), X2_BLADE_MASK_BIT) == 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, X2_BLADE_ALARM_BYTE), X2_BLADE_ALARM_BIT, 0);
    return;
  }
  bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, X2_BLADE_ALARM_BYTE), X2_BLADE_ALARM_BIT, active ? 1 : 0);
}

static uint8_t bladeAmpPct(void) {
  uint8_t pct = VAR_WIRE_BYTE(VAR_X2_BLADE_AMP_PCT, 2);
  if (pct == 0xFF || pct < BLADE_AMP_PCT_MIN) {
    pct = BLADE_AMP_PCT_DEFAULT;
  }
  if (pct > BLADE_AMP_PCT_MAX) {
    pct = BLADE_AMP_PCT_MAX;
  }
  return pct;
}

static uint8_t bladeMinSwingCa(void) {
  uint8_t swing = VAR_WIRE_BYTE(VAR_X2_BLADE_MIN_SWING, 2);
  if (swing == 0xFF || swing == 0) {
    swing = BLADE_MIN_SWING_CA_DEFAULT;
  }
  if (swing > BLADE_MIN_SWING_CA_MAX) {
    swing = BLADE_MIN_SWING_CA_MAX;
  }
  return swing;
}

static uint32_t bladeDropThresholdCa(uint16_t ref) {
  return ((uint32_t)ref * bladeAmpPct()) / 100u;
}

static void clearBladeAlarmState(void) {
  suspectStreak = 0;
  clearStreak = 0;
  bladeLatched = false;
  setBladeAlarmBit(false);
}

void x2BladeInit(void) {
  suspectStreak = 0;
  clearStreak = 0;
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

void x2BladeOnCycleEnd(uint16_t cycleImax, uint8_t nPicos, bool inactivity, bool revolutionMode,
                       float currentSwingA, bool hadProtection, bool hadX2Disconnected) {
  if (!revolutionMode) {
    return;
  }

  const uint16_t ref = x2WearBaselineRef();
  if (ref == 0) {
    return;
  }

  if (hadProtection || hadX2Disconnected) {
    suspectStreak = 0;
    return;
  }

  const uint32_t dropThreshold = bladeDropThresholdCa(ref);
  const uint32_t clearThreshold = ((uint32_t)ref * BLADE_CLEAR_PCT) / 100u;
  const uint16_t swingCa = (uint16_t)(currentSwingA * 100.0f);
  const bool noRevolutions = (nPicos == 0) && inactivity;
  const bool ampDrop = ((uint32_t)cycleImax < dropThreshold);
  const bool flatCurrent = (swingCa <= bladeMinSwingCa());
  const bool suspect = noRevolutions && ampDrop && flatCurrent;

#if MOTOR_WEAR_DEBUG
  Serial.print(F("[X2BLADE] imax="));
  Serial.print(cycleImax);
  Serial.print(F(" ref="));
  Serial.print(ref);
  Serial.print(F(" thr="));
  Serial.print(dropThreshold);
  Serial.print(F(" rev="));
  Serial.print(nPicos);
  Serial.print(F(" swing="));
  Serial.print(swingCa);
  Serial.print(F(" sus="));
  Serial.print(suspect ? 1 : 0);
  Serial.print(F(" latch="));
  Serial.println(bladeLatched ? 1 : 0);
#endif

  if (bladeLatched) {
    if ((nPicos > 0) && ((uint32_t)cycleImax >= clearThreshold)) {
      if (clearStreak < 255) {
        clearStreak++;
      }
    } else {
      clearStreak = 0;
    }
    if (clearStreak >= BLADE_CLEAR_STREAK) {
      clearBladeAlarmState();
#if MOTOR_WEAR_DEBUG
      Serial.println(F("[X2BLADE] cleared"));
#endif
    }
    return;
  }

  if (suspect) {
    if (suspectStreak < 255) {
      suspectStreak++;
    }
    if (suspectStreak >= BLADE_CONSECUTIVE) {
      bladeLatched = true;
      setBladeAlarmBit(true);
#if MOTOR_WEAR_DEBUG
      Serial.println(F("[X2BLADE] latched"));
#endif
    }
  } else {
    suspectStreak = 0;
  }
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
