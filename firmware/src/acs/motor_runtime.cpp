#include "acs/motor_runtime.h"
#include "core/vars.h"
#include "eeprom/eeprom_config.h"
#include "variables/var_io.h"
#include <EEPROM.h>

static uint32_t readRuntimeSec(int row) {
  return ((uint32_t)VAR_WIRE_BYTE(row, 2) << 24) |
         ((uint32_t)VAR_WIRE_BYTE(row, 3) << 16) |
         ((uint32_t)VAR_WIRE_BYTE(row, 4) << 8) |
         (uint32_t)VAR_WIRE_BYTE(row, 5);
}

static void writeRuntimeSec(int row, uint32_t seconds) {
  VAR_WIRE_BYTE(row, 2) = (uint8_t)(seconds >> 24);
  VAR_WIRE_BYTE(row, 3) = (uint8_t)(seconds >> 16);
  VAR_WIRE_BYTE(row, 4) = (uint8_t)(seconds >> 8);
  VAR_WIRE_BYTE(row, 5) = (uint8_t)(seconds);
  saveInEeprom(row);
}

static void addRuntimeMs(int row, uint32_t runMs) {
  if (runMs == 0) {
    return;
  }
  const uint32_t addSec = runMs / 1000u;
  if (addSec == 0) {
    return;
  }
  writeRuntimeSec(row, readRuntimeSec(row) + addSec);
}

static void initRuntimeRow(int row) {
  uint32_t stored = 0xFFFFFFFFu;
  EEPROM.get(varEepromAddr(row), stored);
  if (stored == 0xFFFFFFFFu) {
    stored = 0;
    writeRuntimeSec(row, stored);
  } else {
    writeRuntimeSec(row, stored);
  }
}

void motorRuntimeInit(void) {
  initRuntimeRow(VAR_X2_MOTOR_RUNTIME);
  initRuntimeRow(VAR_X3_MOTOR_RUNTIME);
}

void motorRuntimeOnCycleEnd(uint32_t x2RunMs, uint32_t x3RunMs, bool emptyHopper) {
  if (emptyHopper) {
    return;
  }
  addRuntimeMs(VAR_X2_MOTOR_RUNTIME, x2RunMs);
  addRuntimeMs(VAR_X3_MOTOR_RUNTIME, x3RunMs);
}

void motorRuntimeResetX2(void) {
  writeRuntimeSec(VAR_X2_MOTOR_RUNTIME, 0);
}

void motorRuntimeResetX3(void) {
  writeRuntimeSec(VAR_X3_MOTOR_RUNTIME, 0);
}

uint32_t motorRuntimeX2Seconds(void) {
  return readRuntimeSec(VAR_X2_MOTOR_RUNTIME);
}

uint32_t motorRuntimeX3Seconds(void) {
  return readRuntimeSec(VAR_X3_MOTOR_RUNTIME);
}

void motorRuntimeWriteX2(uint32_t seconds) {
  writeRuntimeSec(VAR_X2_MOTOR_RUNTIME, seconds);
}

void motorRuntimeWriteX3(uint32_t seconds) {
  writeRuntimeSec(VAR_X3_MOTOR_RUNTIME, seconds);
}
