#include "acs/motor_wear_monitor.h"
#include "acs/motor_runtime.h"
#include "core/vars.h"
#include "eeprom/eeprom_config.h"
#include "variables/var_io.h"
#include <EEPROM.h>

#define MOTOR_REF_SAMPLES_DEFAULT  20u
#define MOTOR_REF_SAMPLES_MIN      1u
#define MOTOR_REF_SAMPLES_MAX      255u
#define WEAR_EMA_ALPHA_DEFAULT     2u
#define WEAR_EMA_ALPHA_MIN         1u
#define WEAR_EMA_ALPHA_MAX         100u
#define WEAR_AUTO_MIN_CYCLES       20u
#define WEAR_AUTO_DROP_NUM         80u
#define WEAR_AUTO_DROP_DEN         100u
#define WEAR_AUTO_ELEVATED_NUM     115u
#define WEAR_AUTO_ELEVATED_DEN     100u
#define WEAR_AUTO_CONSECUTIVE      5u
#define WEAR_PROFILE_DEV_HIGH_NUM  140u
#define WEAR_PROFILE_DEV_LOW_NUM   60u
#define MOTOR_CHANGE_PCT_DEFAULT   40u
#define MOTOR_CHANGE_PCT_MIN       1u
#define MOTOR_CHANGE_PCT_MAX       255u
#define WEAR_PROFILE_TRACKING_NUM  10u
#define WEAR_PROFILE_CONSECUTIVE   5u
#define WEAR_PROFILE_MIN_CYCLES    10u
#define WEAR_I_MIN_FLOOR_CA        3u
#define WEAR_I_MAX_CA              2500u
#define WEAR_MIN_DOSING_MS         2000u
#define WEAR_PCT_DEFAULT           30u
#define WEAR_PCT_MIN               1u
#define WEAR_PCT_MAX               255u

#define MOTOR_BACKUP_FLAG_WEAR_LATCHED  0x01u
#define MOTOR_BACKUP_FLAG_WEAR_SLOW_ARM 0x02u
#define MOTOR_CHANGE_PENDING_X2_BIT     0x01u
#define MOTOR_CHANGE_PENDING_X3_BIT     0x02u

#define MOTOR_CHANGE_CONFIRM_ACCEPT  0u
#define MOTOR_CHANGE_CONFIRM_REVERT  1u
#define MOTOR_CHANGE_X2              0u
#define MOTOR_CHANGE_X3              1u

typedef struct {
  uint8_t alarmByte;
  uint8_t alarmBit;
  uint8_t maskByte;
  uint8_t maskBit;
  uint8_t motorChangeAlarmBit;
  uint8_t motorChangeMaskBit;
  int varPct;
  int varAlpha;
  int varRef;
  int varSlow;
  uint8_t eepromHealthy;
  char motorId;
} MotorWearConfig;

typedef struct {
  uint32_t commissioningSum;
  uint8_t commissioningCount;
  uint8_t autoDetectStreak;
  uint8_t profileStreak;
  uint8_t wearUnderStreak;
  uint8_t healthyCount;
  bool wearLatched;
  bool wearSlowArm;
  bool changePending;
} MotorWearState;

static MotorWearState x2State;
static MotorWearState x3State;

static const MotorWearConfig x2Cfg = {
  X2_WEAR_ALARM_BYTE, X2_WEAR_ALARM_BIT, X2_WEAR_MASK_BYTE, X2_WEAR_MASK_BIT,
  X2_MOTOR_CHANGE_ALARM_BIT, X2_MOTOR_CHANGE_MASK_BIT,
  VAR_X2_WEAR_PCT, VAR_X2_WEAR_EMA_ALPHA, VAR_X2_BASELINE_REF, VAR_X2_BASELINE_SLOW,
  EEPROM_ADDR_X2_WEAR_HEALTHY, '2'
};

static const MotorWearConfig x3Cfg = {
  X3_WEAR_ALARM_BYTE, X3_WEAR_ALARM_BIT, X3_WEAR_MASK_BYTE, X3_WEAR_MASK_BIT,
  X3_MOTOR_CHANGE_ALARM_BIT, X3_MOTOR_CHANGE_MASK_BIT,
  VAR_X3_WEAR_PCT, VAR_X3_WEAR_EMA_ALPHA, VAR_X3_BASELINE_REF, VAR_X3_BASELINE_SLOW,
  EEPROM_ADDR_X3_WEAR_HEALTHY, '3'
};

static void printWearTag(const MotorWearConfig* cfg) {
#if MOTOR_WEAR_DEBUG
  Serial.print(F("[X"));
  Serial.print(cfg->motorId);
  Serial.print(F("WEAR] "));
#endif
}

static uint16_t readVarU16(const MotorWearConfig* cfg, int row) {
  (void)cfg;
  return ((uint16_t)VAR_WIRE_BYTE(row, 2) << 8) | VAR_WIRE_BYTE(row, 3);
}

static void writeVarU16(const MotorWearConfig* cfg, int row, uint16_t value) {
  (void)cfg;
  VAR_WIRE_BYTE(row, 2) = highByte(value);
  VAR_WIRE_BYTE(row, 3) = lowByte(value);
}

static void persistHealthyCount(const MotorWearConfig* cfg, MotorWearState* st) {
  EEPROM.update(cfg->eepromHealthy, st->healthyCount);
}

static void persistBaselines(const MotorWearConfig* cfg) {
  saveInEeprom(cfg->varRef);
  saveInEeprom(cfg->varSlow);
}

static void setWearAlarmBit(const MotorWearConfig* cfg, bool active) {
  if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, cfg->maskByte), cfg->maskBit) == 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, cfg->alarmByte), cfg->alarmBit, 0);
    return;
  }
  bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, cfg->alarmByte), cfg->alarmBit, active ? 1 : 0);
}

static void setMotorChangeAlarmBit(const MotorWearConfig* cfg, bool active) {
  if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, cfg->maskByte), cfg->motorChangeMaskBit) == 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, cfg->alarmByte), cfg->motorChangeAlarmBit, 0);
    return;
  }
  bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, cfg->alarmByte), cfg->motorChangeAlarmBit, active ? 1 : 0);
}

static bool isCommissioned(const MotorWearConfig* cfg) {
  return readVarU16(cfg, cfg->varRef) > 0;
}

static uint8_t wearThresholdPct(const MotorWearConfig* cfg) {
  uint8_t pct = VAR_WIRE_BYTE(cfg->varPct, 2);
  if (pct == 0xFF || pct < WEAR_PCT_MIN) {
    pct = WEAR_PCT_DEFAULT;
  }
  if (pct > WEAR_PCT_MAX) {
    pct = WEAR_PCT_MAX;
  }
  return pct;
}

static uint8_t motorRefSampleCount(void) {
  uint8_t n = VAR_WIRE_BYTE(VAR_MOTOR_REF_SAMPLES, 2);
  if (n == 0xFF || n < MOTOR_REF_SAMPLES_MIN) {
    n = MOTOR_REF_SAMPLES_DEFAULT;
  }
  if (n > MOTOR_REF_SAMPLES_MAX) {
    n = MOTOR_REF_SAMPLES_MAX;
  }
  return n;
}

static uint8_t motorChangeStepPct(void) {
  uint8_t pct = VAR_WIRE_BYTE(VAR_MOTOR_CHANGE_PCT, 2);
  if (pct == 0xFF || pct < MOTOR_CHANGE_PCT_MIN) {
    pct = MOTOR_CHANGE_PCT_DEFAULT;
  }
  if (pct > MOTOR_CHANGE_PCT_MAX) {
    pct = MOTOR_CHANGE_PCT_MAX;
  }
  return pct;
}

static uint8_t wearEmaAlphaPct(const MotorWearConfig* cfg) {
  uint8_t alpha = VAR_WIRE_BYTE(cfg->varAlpha, 2);
  if (alpha == 0xFF || alpha < WEAR_EMA_ALPHA_MIN) {
    alpha = WEAR_EMA_ALPHA_DEFAULT;
  }
  if (alpha > WEAR_EMA_ALPHA_MAX) {
    alpha = WEAR_EMA_ALPHA_MAX;
  }
  return alpha;
}

static uint32_t wearActivateThresholdCa(const MotorWearConfig* cfg, uint16_t baselineRef) {
  const uint8_t pct = wearThresholdPct(cfg);
  return ((uint32_t)baselineRef * (100u + pct)) / 100u;
}

static uint16_t wearMinCurrentCa(void) {
  uint16_t disconnected = VAR_WIRE_BYTE(VAR_DISCONNECTED, 2);
  if (disconnected < WEAR_I_MIN_FLOOR_CA) {
    disconnected = WEAR_I_MIN_FLOOR_CA;
  }
  return disconnected + 1u;
}

static uint16_t absDiffU16(uint16_t a, uint16_t b) {
  return (a > b) ? (a - b) : (b - a);
}

static uint16_t backupEepromAddr(const MotorWearConfig* cfg) {
  return (cfg->motorId == '2') ? EEPROM_ADDR_X2_MOTOR_BACKUP : EEPROM_ADDR_X3_MOTOR_BACKUP;
}

static uint8_t pendingBitForMotor(const MotorWearConfig* cfg) {
  return (cfg->motorId == '2') ? MOTOR_CHANGE_PENDING_X2_BIT : MOTOR_CHANGE_PENDING_X3_BIT;
}

static bool isMotorChangePending(const MotorWearConfig* cfg, const MotorWearState* st) {
  (void)cfg;
  return st->changePending;
}

static void setMotorChangePending(const MotorWearConfig* cfg, MotorWearState* st, bool pending) {
  st->changePending = pending;
  uint8_t flags = EEPROM.read(EEPROM_ADDR_MOTOR_CHANGE_PENDING);
  if (flags == 0xFF) {
    flags = 0;
  }
  if (pending) {
    flags |= pendingBitForMotor(cfg);
  } else {
    flags &= (uint8_t)~pendingBitForMotor(cfg);
  }
  EEPROM.update(EEPROM_ADDR_MOTOR_CHANGE_PENDING, flags);
}

static uint32_t runtimeSecondsForMotor(const MotorWearConfig* cfg) {
  return (cfg->motorId == '2') ? motorRuntimeX2Seconds() : motorRuntimeX3Seconds();
}

static void writeRuntimeSecondsForMotor(const MotorWearConfig* cfg, uint32_t seconds) {
  if (cfg->motorId == '2') {
    motorRuntimeWriteX2(seconds);
  } else {
    motorRuntimeWriteX3(seconds);
  }
}

static void resetRuntimeForMotor(const MotorWearConfig* cfg) {
  if (cfg->motorId == '2') {
    motorRuntimeResetX2();
  } else {
    motorRuntimeResetX3();
  }
}

static void saveMotorBackup(const MotorWearConfig* cfg, const MotorWearState* st) {
  const uint16_t addr = backupEepromAddr(cfg);
  const uint16_t ref = readVarU16(cfg, cfg->varRef);
  const uint16_t slow = readVarU16(cfg, cfg->varSlow);
  const uint32_t runtime = runtimeSecondsForMotor(cfg);
  uint8_t flags = 0;
  if (st->wearLatched) {
    flags |= MOTOR_BACKUP_FLAG_WEAR_LATCHED;
  }
  if (st->wearSlowArm) {
    flags |= MOTOR_BACKUP_FLAG_WEAR_SLOW_ARM;
  }

  EEPROM.update(addr + 0, highByte(ref));
  EEPROM.update(addr + 1, lowByte(ref));
  EEPROM.update(addr + 2, highByte(slow));
  EEPROM.update(addr + 3, lowByte(slow));
  EEPROM.update(addr + 4, st->healthyCount);
  EEPROM.update(addr + 5, (uint8_t)(runtime >> 24));
  EEPROM.update(addr + 6, (uint8_t)(runtime >> 16));
  EEPROM.update(addr + 7, (uint8_t)(runtime >> 8));
  EEPROM.update(addr + 8, (uint8_t)(runtime));
  EEPROM.update(addr + 9, flags);

#if MOTOR_WEAR_DEBUG
  printWearTag(cfg);
  Serial.print(F("backup saved ref="));
  Serial.print(ref);
  Serial.print(F(" slow="));
  Serial.print(slow);
  Serial.print(F(" runtime="));
  Serial.println(runtime);
#endif
}

static bool loadMotorBackup(const MotorWearConfig* cfg, uint16_t* ref, uint16_t* slow,
                            uint8_t* healthyCount, uint32_t* runtime, bool* wearLatched,
                            bool* wearSlowArm) {
  const uint16_t addr = backupEepromAddr(cfg);
  const uint8_t refMsb = EEPROM.read(addr + 0);
  const uint8_t refLsb = EEPROM.read(addr + 1);
  if (refMsb == 0xFF && refLsb == 0xFF) {
    return false;
  }

  *ref = ((uint16_t)refMsb << 8) | refLsb;
  *slow = ((uint16_t)EEPROM.read(addr + 2) << 8) | EEPROM.read(addr + 3);
  *healthyCount = EEPROM.read(addr + 4);
  if (*healthyCount == 0xFF) {
    *healthyCount = 0;
  }
  *runtime = ((uint32_t)EEPROM.read(addr + 5) << 24) |
             ((uint32_t)EEPROM.read(addr + 6) << 16) |
             ((uint32_t)EEPROM.read(addr + 7) << 8) |
             EEPROM.read(addr + 8);
  const uint8_t flags = EEPROM.read(addr + 9);
  *wearLatched = (flags & MOTOR_BACKUP_FLAG_WEAR_LATCHED) != 0;
  *wearSlowArm = (flags & MOTOR_BACKUP_FLAG_WEAR_SLOW_ARM) != 0;
  return (*ref > 0);
}

static void clearMotorBackup(const MotorWearConfig* cfg) {
  const uint16_t addr = backupEepromAddr(cfg);
  for (uint8_t i = 0; i < MOTOR_WEAR_BACKUP_SIZE; i++) {
    EEPROM.update(addr + i, 0xFF);
  }
}

static void resetMotorActiveState(const MotorWearConfig* cfg, MotorWearState* st) {
  st->commissioningSum = 0;
  st->commissioningCount = 0;
  st->autoDetectStreak = 0;
  st->profileStreak = 0;
  st->wearUnderStreak = 0;
  st->healthyCount = 0;
  st->wearLatched = false;
  st->wearSlowArm = true;

  writeVarU16(cfg, cfg->varRef, 0);
  writeVarU16(cfg, cfg->varSlow, 0);
  persistBaselines(cfg);
  persistHealthyCount(cfg, st);
  setWearAlarmBit(cfg, false);
  resetRuntimeForMotor(cfg);
}

static void debugLog(const MotorWearConfig* cfg, MotorWearState* st, const __FlashStringHelper* tag) {
#if MOTOR_WEAR_DEBUG
  const uint16_t baselineRef = readVarU16(cfg, cfg->varRef);
  printWearTag(cfg);
  Serial.print(tag);
  Serial.print(F(" ref="));
  Serial.print(baselineRef);
  Serial.print(F(" slow="));
  Serial.print(readVarU16(cfg, cfg->varSlow));
  Serial.print(F(" thr="));
  Serial.print(wearActivateThresholdCa(cfg, baselineRef));
  Serial.print(F(" alpha="));
  Serial.print(wearEmaAlphaPct(cfg));
  Serial.print(F(" healthy="));
  Serial.print(st->healthyCount);
  Serial.print(F(" wear="));
  Serial.print(st->wearLatched ? 1 : 0);
  Serial.print(F(" alarm="));
  Serial.println(bitRead(VAR_WIRE_BYTE(VAR_ALARMS, cfg->alarmByte), cfg->alarmBit));
#else
  (void)cfg;
  (void)st;
  (void)tag;
#endif
}

static void evaluateWearFromSlow(const MotorWearConfig* cfg, MotorWearState* st) {
  if (!isCommissioned(cfg) || st->wearLatched) {
    return;
  }

  const uint16_t baselineRef = readVarU16(cfg, cfg->varRef);
  const uint16_t baselineSlow = readVarU16(cfg, cfg->varSlow);
  if (baselineRef == 0) {
    return;
  }

  const uint32_t activateThreshold = wearActivateThresholdCa(cfg, baselineRef);

  if (!st->wearSlowArm) {
    if ((uint32_t)baselineSlow < activateThreshold) {
      st->wearSlowArm = true;
#if MOTOR_WEAR_DEBUG
      printWearTag(cfg);
      Serial.println(F("slow re-armed"));
#endif
    }
    return;
  }

  if ((uint32_t)baselineSlow >= activateThreshold) {
    st->wearLatched = true;
    st->wearUnderStreak = 0;
    setWearAlarmBit(cfg, true);
    debugLog(cfg, st, F("wear latched slow"));
  }
}

static void evaluateWearCycleEnd(const MotorWearConfig* cfg, MotorWearState* st,
                                 uint16_t cycleImax, uint32_t dosingMs, bool hadProtection) {
  if (dosingMs < WEAR_MIN_DOSING_MS) {
    return;
  }
  if (!isCommissioned(cfg) || hadProtection) {
    if (hadProtection) {
      st->wearUnderStreak = 0;
    }
    return;
  }
  if (cycleImax < wearMinCurrentCa() || cycleImax > WEAR_I_MAX_CA) {
    return;
  }
  if (!st->wearLatched) {
    return;
  }

  const uint16_t baselineRef = readVarU16(cfg, cfg->varRef);
  if (baselineRef == 0) {
    return;
  }

  const uint32_t activateThreshold = wearActivateThresholdCa(cfg, baselineRef);

  if ((uint32_t)cycleImax <= activateThreshold) {
    if (st->wearUnderStreak < 255) {
      st->wearUnderStreak++;
    }
#if MOTOR_WEAR_DEBUG
    printWearTag(cfg);
    Serial.print(F("clear streak "));
    Serial.print(st->wearUnderStreak);
    Serial.print(F("/"));
    Serial.print(MOTOR_WEAR_CLEAR_STREAK);
    Serial.print(F(" imax="));
    Serial.print(cycleImax);
    Serial.print(F(" thr="));
    Serial.println(activateThreshold);
#endif
    if (st->wearUnderStreak >= MOTOR_WEAR_CLEAR_STREAK) {
      st->wearLatched = false;
      st->wearUnderStreak = 0;
      st->wearSlowArm = false;
      setWearAlarmBit(cfg, false);
      debugLog(cfg, st, F("wear cleared"));
    }
  } else {
    st->wearUnderStreak = 0;
  }
}

static void finalizeCommissioning(const MotorWearConfig* cfg, MotorWearState* st) {
  if (st->commissioningCount == 0) {
    return;
  }
  const uint16_t baselineRef = (uint16_t)(st->commissioningSum / st->commissioningCount);
  writeVarU16(cfg, cfg->varRef, baselineRef);
  writeVarU16(cfg, cfg->varSlow, baselineRef);
  st->healthyCount = st->commissioningCount;
  persistBaselines(cfg);
  persistHealthyCount(cfg, st);
  st->commissioningSum = 0;
  st->commissioningCount = 0;
  debugLog(cfg, st, F("commissioned"));
}

static void updateBaselineSlow(const MotorWearConfig* cfg, uint16_t cycleImax) {
  uint16_t slow = readVarU16(cfg, cfg->varSlow);
  if (slow == 0) {
    slow = cycleImax;
  } else {
    const uint8_t alpha = wearEmaAlphaPct(cfg);
    const uint8_t inv = (uint8_t)(100u - alpha);
    slow = (uint16_t)(((uint32_t)slow * inv) + ((uint32_t)cycleImax * alpha)) / 100u;
  }
  writeVarU16(cfg, cfg->varSlow, slow);
  persistBaselines(cfg);
}

static bool isDeviatedHighFromSlow(uint16_t cycleImax, uint16_t baselineSlow, uint16_t baselineRef) {
  const uint16_t baseline = (baselineSlow > 0u) ? baselineSlow : baselineRef;
  if (baseline == 0u) {
    return false;
  }
  return ((uint32_t)cycleImax * 100u) > ((uint32_t)baseline * WEAR_PROFILE_DEV_HIGH_NUM);
}

static bool isDeviatedLowFromRef(uint16_t cycleImax, uint16_t baselineRef) {
  if (baselineRef == 0u) {
    return false;
  }
  return ((uint32_t)cycleImax * 100u) < ((uint32_t)baselineRef * WEAR_PROFILE_DEV_LOW_NUM);
}

static bool isDeviatedForProfileChange(uint16_t cycleImax, uint16_t baselineRef, uint16_t baselineSlow) {
  return isDeviatedHighFromSlow(cycleImax, baselineSlow, baselineRef) ||
         isDeviatedLowFromRef(cycleImax, baselineRef);
}

static bool isTrackingSlow(uint16_t cycleImax, uint16_t baselineRef, uint16_t baselineSlow) {
  return ((uint32_t)absDiffU16(cycleImax, baselineSlow) * 100u) <=
         ((uint32_t)baselineRef * WEAR_PROFILE_TRACKING_NUM);
}

static bool isStepFromSlow(uint16_t cycleImax, uint16_t baselineRef, uint16_t baselineSlow) {
  const uint8_t stepPct = motorChangeStepPct();
  return ((uint32_t)absDiffU16(cycleImax, baselineSlow) * 100u) >
         ((uint32_t)baselineRef * stepPct);
}

static bool tryAutoDetectProfileChange(const MotorWearConfig* cfg, MotorWearState* st, uint16_t cycleImax) {
  if (!isCommissioned(cfg) || st->healthyCount < WEAR_PROFILE_MIN_CYCLES) {
    st->profileStreak = 0;
    return false;
  }

  const uint16_t baselineRef = readVarU16(cfg, cfg->varRef);
  const uint16_t baselineSlow = readVarU16(cfg, cfg->varSlow);
  if (baselineRef == 0) {
    st->profileStreak = 0;
    return false;
  }

  if (isTrackingSlow(cycleImax, baselineRef, baselineSlow)) {
    st->profileStreak = 0;
    return false;
  }

  if (isDeviatedForProfileChange(cycleImax, baselineRef, baselineSlow) &&
      isStepFromSlow(cycleImax, baselineRef, baselineSlow)) {
    st->profileStreak++;
#if MOTOR_WEAR_DEBUG
    printWearTag(cfg);
    Serial.print(F("profile streak "));
    Serial.print(st->profileStreak);
    Serial.print(F("/"));
    Serial.println(WEAR_PROFILE_CONSECUTIVE);
#endif
  } else {
    st->profileStreak = 0;
  }

  if (st->profileStreak >= WEAR_PROFILE_CONSECUTIVE) {
    st->profileStreak = 0;
    return true;
  }
  return false;
}

static bool tryAutoDetectMotorDrop(const MotorWearConfig* cfg, MotorWearState* st, uint16_t cycleImax) {
  if (!isCommissioned(cfg) || st->healthyCount < WEAR_AUTO_MIN_CYCLES) {
    st->autoDetectStreak = 0;
    return false;
  }

  const uint16_t baselineRef = readVarU16(cfg, cfg->varRef);
  const uint16_t baselineSlow = readVarU16(cfg, cfg->varSlow);
  const bool elevated = st->wearLatched ||
      (((uint32_t)baselineSlow * WEAR_AUTO_ELEVATED_DEN) >
       ((uint32_t)baselineRef * WEAR_AUTO_ELEVATED_NUM));

  if (!elevated) {
    st->autoDetectStreak = 0;
    return false;
  }

  if (((uint32_t)cycleImax * WEAR_AUTO_DROP_DEN) <
      ((uint32_t)baselineRef * WEAR_AUTO_DROP_NUM)) {
    st->autoDetectStreak++;
  } else {
    st->autoDetectStreak = 0;
  }

  if (st->autoDetectStreak >= WEAR_AUTO_CONSECUTIVE) {
    st->autoDetectStreak = 0;
    return true;
  }
  return false;
}

static bool isHealthyCycle(uint16_t cycleImax, uint32_t dosingMs, bool hadProtection,
                           bool hadX2Disconnected, bool hadX3Disconnected, bool hadEmptyHopper) {
  if (hadProtection || hadX2Disconnected || hadX3Disconnected || hadEmptyHopper) {
    return false;
  }
  if (dosingMs < WEAR_MIN_DOSING_MS) {
    return false;
  }
  if (cycleImax < wearMinCurrentCa() || cycleImax > WEAR_I_MAX_CA) {
    return false;
  }
  return true;
}

static void beginAutoMotorChange(const MotorWearConfig* cfg, MotorWearState* st) {
  saveMotorBackup(cfg, st);
  resetMotorActiveState(cfg, st);
  setMotorChangePending(cfg, st, true);
  setMotorChangeAlarmBit(cfg, true);

#if MOTOR_WEAR_DEBUG
  printWearTag(cfg);
  Serial.println(F("auto motor change pending"));
#endif
}

static void acceptMotorChange(const MotorWearConfig* cfg, MotorWearState* st) {
  if (!isMotorChangePending(cfg, st)) {
    return;
  }
  clearMotorBackup(cfg);
  setMotorChangePending(cfg, st, false);
  setMotorChangeAlarmBit(cfg, false);

#if MOTOR_WEAR_DEBUG
  printWearTag(cfg);
  Serial.println(F("motor change accepted"));
#endif
}

static void revertMotorChange(const MotorWearConfig* cfg, MotorWearState* st) {
  if (!isMotorChangePending(cfg, st)) {
    return;
  }

  uint16_t ref = 0;
  uint16_t slow = 0;
  uint8_t healthyCount = 0;
  uint32_t runtime = 0;
  bool wearLatched = false;
  bool wearSlowArm = true;

  if (!loadMotorBackup(cfg, &ref, &slow, &healthyCount, &runtime, &wearLatched, &wearSlowArm)) {
    clearMotorBackup(cfg);
    setMotorChangePending(cfg, st, false);
    setMotorChangeAlarmBit(cfg, false);
    return;
  }

  st->commissioningSum = 0;
  st->commissioningCount = 0;
  st->autoDetectStreak = 0;
  st->profileStreak = 0;
  st->wearUnderStreak = 0;
  st->healthyCount = healthyCount;
  st->wearLatched = wearLatched;
  st->wearSlowArm = wearSlowArm;

  writeVarU16(cfg, cfg->varRef, ref);
  writeVarU16(cfg, cfg->varSlow, slow);
  persistBaselines(cfg);
  persistHealthyCount(cfg, st);
  writeRuntimeSecondsForMotor(cfg, runtime);
  setWearAlarmBit(cfg, wearLatched);

  clearMotorBackup(cfg);
  setMotorChangePending(cfg, st, false);
  setMotorChangeAlarmBit(cfg, false);

#if MOTOR_WEAR_DEBUG
  printWearTag(cfg);
  Serial.print(F("motor change reverted ref="));
  Serial.print(ref);
  Serial.print(F(" runtime="));
  Serial.println(runtime);
#endif
}

static void resetMotor(const MotorWearConfig* cfg, MotorWearState* st, bool autoDetected) {
  if (autoDetected) {
    beginAutoMotorChange(cfg, st);
    return;
  }

  clearMotorBackup(cfg);
  setMotorChangePending(cfg, st, false);
  resetMotorActiveState(cfg, st);
  setMotorChangeAlarmBit(cfg, false);

#if MOTOR_WEAR_DEBUG
  printWearTag(cfg);
  Serial.println(F("reset motor manual"));
#endif
}

static void initOne(const MotorWearConfig* cfg, MotorWearState* st) {
  st->commissioningSum = 0;
  st->commissioningCount = 0;
  st->autoDetectStreak = 0;
  st->profileStreak = 0;
  st->wearUnderStreak = 0;
  st->wearLatched = false;
  st->wearSlowArm = true;
  st->changePending = false;

  uint8_t storedHealthy = 0xFF;
  EEPROM.get(cfg->eepromHealthy, storedHealthy);
  if (storedHealthy == 0xFF) {
    storedHealthy = 0;
    EEPROM.update(cfg->eepromHealthy, storedHealthy);
  }
  st->healthyCount = storedHealthy;

  if (readVarU16(cfg, cfg->varRef) == 0xFFFF) {
    writeVarU16(cfg, cfg->varRef, 0);
    writeVarU16(cfg, cfg->varSlow, 0);
    persistBaselines(cfg);
  }
  if (readVarU16(cfg, cfg->varSlow) == 0xFFFF) {
    writeVarU16(cfg, cfg->varSlow, 0);
    persistBaselines(cfg);
  }

  if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, cfg->maskByte), cfg->maskBit) == 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARM_MASK, cfg->maskByte), cfg->maskBit, 1);
    saveInEeprom(VAR_ALARM_MASK);
  }
  if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, cfg->maskByte), cfg->motorChangeMaskBit) == 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARM_MASK, cfg->maskByte), cfg->motorChangeMaskBit, 1);
    saveInEeprom(VAR_ALARM_MASK);
  }

  {
    uint8_t pendingFlags = EEPROM.read(EEPROM_ADDR_MOTOR_CHANGE_PENDING);
    if (pendingFlags == 0xFF) {
      pendingFlags = 0;
      EEPROM.update(EEPROM_ADDR_MOTOR_CHANGE_PENDING, pendingFlags);
    }
    st->changePending = (pendingFlags & pendingBitForMotor(cfg)) != 0;
    if (st->changePending) {
      setMotorChangeAlarmBit(cfg, true);
    }
  }

  if (isCommissioned(cfg) && st->healthyCount >= motorRefSampleCount()) {
    evaluateWearFromSlow(cfg, st);
  } else {
    setWearAlarmBit(cfg, false);
  }

  debugLog(cfg, st, F("init"));
}

static void onCycleEnd(const MotorWearConfig* cfg, MotorWearState* st,
                       uint16_t cycleImax, uint32_t dosingMs, bool hadProtection,
                       bool hadX2Disconnected, bool hadX3Disconnected, bool hadEmptyHopper) {
  if (dosingMs == 0 && cycleImax == 0) {
    return;
  }

#if MOTOR_WEAR_DEBUG
  printWearTag(cfg);
  Serial.print(F("cycle imax="));
  Serial.print(cycleImax);
  Serial.print(F(" ms="));
  Serial.print(dosingMs);
  Serial.print(F(" prot="));
  Serial.print(hadProtection ? 1 : 0);
  Serial.print(F(" discX2="));
  Serial.print(hadX2Disconnected ? 1 : 0);
  Serial.print(F(" discX3="));
  Serial.print(hadX3Disconnected ? 1 : 0);
  Serial.print(F(" hopper="));
  Serial.println(hadEmptyHopper ? 1 : 0);
#endif

  if (!hadX2Disconnected && !hadX3Disconnected && !hadEmptyHopper) {
    evaluateWearCycleEnd(cfg, st, cycleImax, dosingMs, hadProtection);
  }

  if (!isHealthyCycle(cycleImax, dosingMs, hadProtection, hadX2Disconnected, hadX3Disconnected,
                      hadEmptyHopper)) {
#if MOTOR_WEAR_DEBUG
    printWearTag(cfg);
    Serial.println(F("ciclo descartado"));
#endif
    st->profileStreak = 0;
    st->autoDetectStreak = 0;
    return;
  }

  if (tryAutoDetectProfileChange(cfg, st, cycleImax)) {
#if MOTOR_WEAR_DEBUG
    printWearTag(cfg);
    Serial.println(F("auto-reset: perfil"));
#endif
    resetMotor(cfg, st, true);
  } else if (tryAutoDetectMotorDrop(cfg, st, cycleImax)) {
#if MOTOR_WEAR_DEBUG
    printWearTag(cfg);
    Serial.println(F("auto-reset: caida post-desgaste"));
#endif
    resetMotor(cfg, st, true);
  }

  if (!isCommissioned(cfg)) {
    st->commissioningSum += cycleImax;
    st->commissioningCount++;
    st->healthyCount = st->commissioningCount;
    persistHealthyCount(cfg, st);

#if MOTOR_WEAR_DEBUG
    printWearTag(cfg);
    Serial.print(F("commissioning "));
    Serial.print(st->commissioningCount);
    Serial.print(F("/"));
    Serial.println(motorRefSampleCount());
#endif

    if (st->commissioningCount >= motorRefSampleCount()) {
      finalizeCommissioning(cfg, st);
      evaluateWearFromSlow(cfg, st);
    }
    return;
  }

  updateBaselineSlow(cfg, cycleImax);
  if (st->healthyCount < 255) {
    st->healthyCount++;
    persistHealthyCount(cfg, st);
  }

  evaluateWearFromSlow(cfg, st);
  debugLog(cfg, st, F("cycle ok"));
}

static void syncAlarm(const MotorWearConfig* cfg, MotorWearState* st, bool hadProtection) {
  if (hadProtection) {
    setWearAlarmBit(cfg, false);
    return;
  }
  setWearAlarmBit(cfg, st->wearLatched);
}

void motorWearInitAll(void) {
  bool maskChanged = false;
  if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 2), 1) != 0 ||
      bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, 2), 2) != 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARM_MASK, 0), 0, 1);
    bitWrite(VAR_WIRE_BYTE(VAR_ALARM_MASK, 0), 1, 1);
    bitWrite(VAR_WIRE_BYTE(VAR_ALARM_MASK, 2), 1, 0);
    bitWrite(VAR_WIRE_BYTE(VAR_ALARM_MASK, 2), 2, 0);
    maskChanged = true;
  }
  /* Migrar desgaste X3 legado (bit 2) al bit 1 */
  if (bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 2), 2) != 0 &&
      bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 2), 1) == 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 2), 1, 1);
    bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, 2), 2, 0);
  }
  if (maskChanged) {
    saveInEeprom(VAR_ALARM_MASK);
  }
  initOne(&x2Cfg, &x2State);
  initOne(&x3Cfg, &x3State);
}

void motorWearApplyAlarmMask(void) {
  if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, X2_WEAR_MASK_BYTE), X2_WEAR_MASK_BIT) == 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, X2_WEAR_ALARM_BYTE), X2_WEAR_ALARM_BIT, 0);
  }
  if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, X3_WEAR_MASK_BYTE), X3_WEAR_MASK_BIT) == 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, X3_WEAR_ALARM_BYTE), X3_WEAR_ALARM_BIT, 0);
  }
  if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, X2_WEAR_MASK_BYTE), X2_MOTOR_CHANGE_MASK_BIT) == 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, X2_WEAR_ALARM_BYTE), X2_MOTOR_CHANGE_ALARM_BIT, 0);
  }
  if (bitRead(VAR_WIRE_BYTE(VAR_ALARM_MASK, X3_WEAR_MASK_BYTE), X3_MOTOR_CHANGE_MASK_BIT) == 0) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARMS, X3_WEAR_ALARM_BYTE), X3_MOTOR_CHANGE_ALARM_BIT, 0);
  }
}

void x2WearOnCycleEnd(uint16_t cycleImax, uint32_t dosingMs, bool hadProtection,
                      bool hadX2Disconnected, bool hadX3Disconnected, bool hadEmptyHopper) {
  onCycleEnd(&x2Cfg, &x2State, cycleImax, dosingMs, hadProtection, hadX2Disconnected,
             hadX3Disconnected, hadEmptyHopper);
}

void x3WearOnCycleEnd(uint16_t cycleImax, uint32_t dosingMs, bool hadProtection,
                      bool hadX2Disconnected, bool hadX3Disconnected, bool hadEmptyHopper) {
  onCycleEnd(&x3Cfg, &x3State, cycleImax, dosingMs, hadProtection, hadX2Disconnected,
             hadX3Disconnected, hadEmptyHopper);
}

void x2WearSyncAlarm(bool hadProtection) {
  syncAlarm(&x2Cfg, &x2State, hadProtection);
}

void x3WearSyncAlarm(bool hadProtection) {
  syncAlarm(&x3Cfg, &x3State, hadProtection);
}

bool x2WearHandleMotorResetWrite(uint8_t command) {
  if (command == MOTOR_WEAR_CMD_RESET) {
    resetMotor(&x2Cfg, &x2State, false);
    return true;
  }
  return false;
}

bool x3WearHandleMotorResetWrite(uint8_t command) {
  if (command == MOTOR_WEAR_CMD_RESET) {
    resetMotor(&x3Cfg, &x3State, false);
    return true;
  }
  return false;
}

bool motorWearHandleChangeConfirm(uint8_t motor, uint8_t action) {
  const MotorWearConfig* cfg = NULL;
  MotorWearState* st = NULL;

  if (motor == MOTOR_CHANGE_X2) {
    cfg = &x2Cfg;
    st = &x2State;
  } else if (motor == MOTOR_CHANGE_X3) {
    cfg = &x3Cfg;
    st = &x3State;
  } else {
    return false;
  }

  if (!isMotorChangePending(cfg, st)) {
    return false;
  }

  if (action == MOTOR_CHANGE_CONFIRM_ACCEPT) {
    acceptMotorChange(cfg, st);
    return true;
  }
  if (action == MOTOR_CHANGE_CONFIRM_REVERT) {
    revertMotorChange(cfg, st);
    return true;
  }
  return false;
}

bool motorWearChangePendingX2(void) {
  return x2State.changePending;
}

bool motorWearChangePendingX3(void) {
  return x3State.changePending;
}

uint16_t x2WearBaselineRef(void) { return readVarU16(&x2Cfg, x2Cfg.varRef); }
uint16_t x2WearBaselineSlow(void) { return readVarU16(&x2Cfg, x2Cfg.varSlow); }
uint8_t x2WearHealthyCount(void) { return x2State.healthyCount; }
bool x2WearAlarmActive(void) { return x2State.wearLatched; }

uint16_t x3WearBaselineRef(void) { return readVarU16(&x3Cfg, x3Cfg.varRef); }
uint16_t x3WearBaselineSlow(void) { return readVarU16(&x3Cfg, x3Cfg.varSlow); }
uint8_t x3WearHealthyCount(void) { return x3State.healthyCount; }
bool x3WearAlarmActive(void) { return x3State.wearLatched; }
