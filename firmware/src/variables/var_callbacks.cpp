#include <Arduino.h>
#include <EEPROM.h>
#include "core/vars.h"
#include "variables/var_callbacks.h"
#include "variables/var_io.h"
#include "eeprom/eeprom_config.h"
#include "state/rtc_cache.h"
#include "xbee/xbee.h"
#include "acs/motor_wear_monitor.h"
#include "acs/x2_blade_monitor.h"
bool varWriteCopySave(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  copyVarWireFromFrame(row, posLSB, frame);
  saveInEeprom(row);
  countByte = transferVar[row][VAR_SIZE];
  return true;
}
bool varWriteCopyOnly(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  copyVarWireFromFrame(row, posLSB, frame);
  countByte = transferVar[row][VAR_SIZE];
  return true;
}
bool varWriteAmTable(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  if (!varWriteCopySave(row, posLSB, frame, countByte)) {
    return false;
  }
  app.dosage.changeTableAm = TRUE;
  return true;
}
bool varWritePmTable(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  if (!varWriteCopySave(row, posLSB, frame, countByte)) {
    return false;
  }
  app.dosage.changeTablePm = TRUE;
  return true;
}
bool varWriteAlarmMask(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  copyVarWireFromFrame(row, posLSB, frame);
  if (Events_ArduinoUNO) {
    bitWrite(VAR_WIRE_BYTE(VAR_ALARM_MASK, 3), 3, 1);
  }
  saveInEeprom(row);
  motorWearApplyAlarmMask();
  x2BladeApplyAlarmMask();
  countByte = transferVar[row][VAR_SIZE];
  return true;
}
bool varWritePercentMotor(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  countByte = transferVar[row][VAR_SIZE];
  if ((frame[posLSB + 2] > 0) && (frame[posLSB + 2] <= 100)) {
    copyVarWireFromFrame(row, posLSB, frame);
    saveInEeprom(row);
  }
  return true;
}
bool varWriteRtc(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  countByte = transferVar[row][VAR_SIZE];
  if (bitRead(VAR_WIRE_BYTE(VAR_SETTING_BYTES, 2), 7) == FALSE) {
    copyVarWireFromFrame(row, posLSB, frame);
    saveInEeprom(VAR_RTC);
    if (rtc.persistedDay != VAR_WIRE_BYTE(VAR_RTC, 5)) {
      VAR_WIRE_BYTE(VAR_RESET_COUNT, 2) = 0;
      EEPROM.put(varEepromAddr(VAR_RESET_COUNT), VAR_WIRE_BYTE(VAR_RESET_COUNT, 2));
      rtc.persistedDay = VAR_WIRE_BYTE(VAR_RTC, 5);
      EEPROM.put(EEPROM_ADDR_AUX_DAY, rtc.persistedDay);
    }
    rtcApplyWrittenRemote();
    ChangeGramsAndTime();
  }
  return true;
}
static void writePanIdAtCommands(void) {
  unsigned char atPanID[] = {0x7E, 0x00, 0x06, 0x09, 0x6C, 0x49, 0x44, 0x00, 0x00, 0x00};
  unsigned char atWR[] = {0x7E, 0x00, 0x04, 0x09, 0x6D, 0x57, 0x52, 0xE0};
  unsigned char atAC[] = {0x7E, 0x00, 0x04, 0x09, 0x6E, 0x41, 0x43, 0x04};
  atPanID[7] = VAR_WIRE_BYTE(VAR_PAN_ID, 2);
  atPanID[8] = VAR_WIRE_BYTE(VAR_PAN_ID, 3);
  set_checksum(atPanID, 10);
  for (uint8_t i = 0; i < sizeof(atPanID); i++) {
    Serial3.write(atPanID[i]);
  }
  delay(50);
  for (uint8_t i = 0; i < sizeof(atWR); i++) {
    Serial3.write(atWR[i]);
  }
  delay(50);
  for (uint8_t i = 0; i < sizeof(atAC); i++) {
    Serial3.write(atAC[i]);
  }
  delay(50);
}
bool varWritePanId(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  copyVarWireFromFrame(row, posLSB, frame);
  writePanIdAtCommands();
  countByte = transferVar[row][VAR_SIZE];
  return true;
}
bool varWriteCycleGrams(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  (void)row;
  countByte = transferVar[VAR_CYCLE_GRAMS][VAR_SIZE];
  if (bitRead(VAR_WIRE_BYTE(VAR_ALARMS, 3), 3) != 0) {
    return false;
  }
  for (uint8_t i = 4; i <= 11; i++) {
    if (frame[i] != VAR_WIRE_BYTE(VAR_SERVER, i - 2)) {
      Serial.println(F("EMISOR DEL MENSAJE NO ES EL SERVIDOR..!!"));
      return false;
    }
    if (i == 11) {
      copyVarWireFromFrame(VAR_CYCLE_GRAMS, posLSB, frame);
      copyVarWireFromFrame(VAR_DOSED_GRAMS, posLSB, frame);
      VAR_WIRE_BYTE(VAR_ID_DOSAGE, 2) = frame[16];
      VAR_WIRE_BYTE(VAR_ID_DOSAGE, 3) = frame[17];
    }
  }
  return true;
}
bool varWriteX2BladeAck(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  copyVarWireFromFrame(row, posLSB, frame);
  x2BladeHandleAckWrite(VAR_WIRE_BYTE(row, 2));
  VAR_WIRE_BYTE(row, 2) = 0;
  countByte = transferVar[row][VAR_SIZE];
  return true;
}
bool varWriteX2BladeAmpPct(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  countByte = transferVar[row][VAR_SIZE];
  const uint8_t pct = frame[posLSB + 2];
  if (pct >= 10 && pct <= 45) {
    copyVarWireFromFrame(row, posLSB, frame);
    saveInEeprom(row);
  }
  return true;
}
bool varWriteX2BladeMinSwing(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  countByte = transferVar[row][VAR_SIZE];
  const uint8_t swing = frame[posLSB + 2];
  if (swing >= 1 && swing <= 100) {
    copyVarWireFromFrame(row, posLSB, frame);
    saveInEeprom(row);
  }
  return true;
}
bool varWriteX2MotorReset(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  copyVarWireFromFrame(row, posLSB, frame);
  x2WearHandleMotorResetWrite(VAR_WIRE_BYTE(row, 2));
  VAR_WIRE_BYTE(row, 2) = 0;
  countByte = transferVar[row][VAR_SIZE];
  return true;
}
bool varWriteX3MotorReset(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  copyVarWireFromFrame(row, posLSB, frame);
  x3WearHandleMotorResetWrite(VAR_WIRE_BYTE(row, 2));
  VAR_WIRE_BYTE(row, 2) = 0;
  countByte = transferVar[row][VAR_SIZE];
  return true;
}
bool varWriteX2WearPct(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  countByte = transferVar[row][VAR_SIZE];
  const uint8_t pct = frame[posLSB + 2];
  if (pct >= 1) {
    copyVarWireFromFrame(row, posLSB, frame);
    saveInEeprom(row);
  }
  return true;
}
bool varWriteX2WearEmaAlpha(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  countByte = transferVar[row][VAR_SIZE];
  const uint8_t alpha = frame[posLSB + 2];
  if (alpha >= 1 && alpha <= 100) {
    copyVarWireFromFrame(row, posLSB, frame);
    saveInEeprom(row);
  }
  return true;
}
bool varWriteMotorRefSamples(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  countByte = transferVar[row][VAR_SIZE];
  const uint8_t n = frame[posLSB + 2];
  if (n >= 1) {
    copyVarWireFromFrame(row, posLSB, frame);
    saveInEeprom(row);
  }
  return true;
}
bool varWriteMotorChangePct(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  countByte = transferVar[row][VAR_SIZE];
  const uint8_t pct = frame[posLSB + 2];
  if (pct >= 1) {
    copyVarWireFromFrame(row, posLSB, frame);
    saveInEeprom(row);
  }
  return true;
}
bool varWriteX3WearPct(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  countByte = transferVar[row][VAR_SIZE];
  const uint8_t pct = frame[posLSB + 2];
  if (pct >= 1) {
    copyVarWireFromFrame(row, posLSB, frame);
    saveInEeprom(row);
  }
  return true;
}
bool varWriteX3WearEmaAlpha(int row, uint8_t posLSB, const uint8_t* frame, uint8_t& countByte) {
  countByte = transferVar[row][VAR_SIZE];
  const uint8_t alpha = frame[posLSB + 2];
  if (alpha >= 1 && alpha <= 100) {
    copyVarWireFromFrame(row, posLSB, frame);
    saveInEeprom(row);
  }
  return true;
}
const VarWriteHandlerFn kVarWriteHandlers[ROW] = {
  /* VAR_SETTING_BYTES      */ varWriteCopySave,
  /* VAR_SOFTWARE_VERSION   */ NULL,
  /* VAR_FEED_RATE          */ varWriteCopySave,
  /* VAR_CYCLE_TIME         */ NULL,
  /* VAR_CYCLE_GRAMS        */ varWriteCycleGrams,
  /* VAR_PROFILE            */ NULL,
  /* VAR_HARDWARE_VERSION   */ NULL,
  /* VAR_PROJECT_NAME       */ NULL,
  /* VAR_AM_TABLE           */ varWriteAmTable,
  /* VAR_PM_TABLE           */ varWritePmTable,
  /* VAR_ALARMS             */ varWriteCopyOnly,
  /* VAR_ALARM_MASK         */ varWriteAlarmMask,
  /* VAR_RTC                */ varWriteRtc,
  /* VAR_RTC_RESET          */ NULL,
  /* VAR_RESET_COUNT        */ NULL,
  /* VAR_SERVER             */ varWriteCopySave,
  /* VAR_BATTERY            */ NULL,
  /* VAR_PANNEL             */ NULL,
  /* VAR_BATTERY_TARA       */ varWriteCopySave,
  /* VAR_PANNEL_TARA        */ varWriteCopySave,
  /* VAR_LOW_BATTERY        */ varWriteCopySave,
  /* VAR_ID_DOSAGE          */ NULL,
  /* VAR_ACCUMULATED_GRAMS  */ NULL,
  /* VAR_DOSED_GRAMS        */ varWriteCopyOnly,
  /* VAR_TIME_ALARM         */ varWriteCopySave,
  /* VAR_PERCENT_MOTOR      */ varWritePercentMotor,
  /* VAR_N_VUELTAS          */ NULL,
  /* VAR_T_VUELTAS          */ NULL,
  /* VAR_PAN_ID             */ varWritePanId,
  /* VAR_ACS_TYPE           */ varWriteCopySave,
  /* VAR_AMP_X1             */ NULL,
  /* VAR_AMP_X2             */ NULL,
  /* VAR_AMP_X3             */ NULL,
  /* VAR_PROT_X1            */ varWriteCopySave,
  /* VAR_PROT_X2            */ varWriteCopySave,
  /* VAR_PROT_X3            */ varWriteCopySave,
  /* VAR_DISCONNECTED       */ varWriteCopySave,
  /* VAR_EMPTY_HOPPER       */ varWriteCopySave,
  /* VAR_MOTOR_CHANGE_PCT   */ varWriteMotorChangePct,
  /* VAR_MOTOR_REF_SAMPLES  */ varWriteMotorRefSamples,
  /* VAR_X3_WEAR_PCT        */ varWriteX3WearPct,
  /* VAR_X3_WEAR_EMA_ALPHA  */ varWriteX3WearEmaAlpha,
  /* VAR_AMP_MAX_X2         */ NULL,
  /* VAR_AMP_MIN_X2         */ NULL,
  /* VAR_AMP_MAX_X3         */ NULL,
  /* VAR_AMP_MIN_X3         */ NULL,
  /* VAR_X3_BASELINE_REF    */ NULL,
  /* VAR_X3_BASELINE_SLOW   */ NULL,
  /* VAR_X3_MOTOR_RESET     */ varWriteX3MotorReset,
  /* VAR_X2_WEAR_PCT        */ varWriteX2WearPct,
  /* VAR_X2_WEAR_EMA_ALPHA  */ varWriteX2WearEmaAlpha,
  /* VAR_X2_BASELINE_REF    */ NULL,
  /* VAR_X2_BASELINE_SLOW   */ NULL,
  /* VAR_X2_MOTOR_RESET     */ varWriteX2MotorReset,
  /* VAR_X2_BLADE_AMP_PCT   */ varWriteX2BladeAmpPct,
  /* VAR_X2_BLADE_MIN_SWING */ varWriteX2BladeMinSwing,
  /* VAR_X2_BLADE_ACK       */ varWriteX2BladeAck,
  /* VAR_X2_MOTOR_RUNTIME   */ NULL,
  /* VAR_X3_MOTOR_RUNTIME   */ NULL,
  /* VAR_RESET_EEPROM       */ varWriteCopySave,
};
