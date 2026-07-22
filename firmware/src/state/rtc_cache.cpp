#include <Arduino.h>
#include <TimeLib.h>

#include "state/rtc_cache.h"
#include "core/config.h"
#include "core/app_state.h"
#include "variables/var_access.h"
#include "rtc/rtc.h"

RtcCache rtc = {
  0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0,
  0, 1, 0, 0xFF,
  0, 0
};

void rtcSyncFromTimeLib(void) {
  if (app.eeprom.hourLoaded == FALSE) {
    return;
  }
  rtc.hour = hour();
  rtc.minute = minute();
  rtc.second = second();
  rtc.day = day();
  rtc.month = month();
  rtc.year = year() - 2000;
}

void rtcPushCacheToWire(void) {
  VAR_WIRE_BYTE(VAR_RTC, 2) = HEXA_TO_BCD(rtc.hour);
  VAR_WIRE_BYTE(VAR_RTC, 3) = HEXA_TO_BCD(rtc.minute);
  VAR_WIRE_BYTE(VAR_RTC, 4) = HEXA_TO_BCD(rtc.second);
  VAR_WIRE_BYTE(VAR_RTC, 5) = HEXA_TO_BCD(rtc.day);
  VAR_WIRE_BYTE(VAR_RTC, 6) = HEXA_TO_BCD(rtc.month);
  VAR_WIRE_BYTE(VAR_RTC, 7) = HEXA_TO_BCD(rtc.year);
}

void rtcApplyRemoteFromWire(void) {
  uint8_t h = BCD_TO_HEXA(VAR_WIRE_BYTE(VAR_RTC, 2));
  uint8_t mi = BCD_TO_HEXA(VAR_WIRE_BYTE(VAR_RTC, 3));
  uint8_t s = BCD_TO_HEXA(VAR_WIRE_BYTE(VAR_RTC, 4));
  uint8_t d = BCD_TO_HEXA(VAR_WIRE_BYTE(VAR_RTC, 5));
  uint8_t mo = BCD_TO_HEXA(VAR_WIRE_BYTE(VAR_RTC, 6));
  uint8_t y = BCD_TO_HEXA(VAR_WIRE_BYTE(VAR_RTC, 7));

  setTime(h, mi, s, d, mo, y);
  app.eeprom.hourLoaded = TRUE;
  rtcSyncFromTimeLib();
}

static void rtcSyncShadowsFromCache(void) {
  rtc.shadowSecond = rtc.second;
  rtc.shadowMinute = rtc.minute;
  rtc.shadowHour = rtc.hour;
  rtc.shadowDay = rtc.day;
  rtc.shadowMonth = rtc.month;
  rtc.shadowYear = rtc.year;
}

static bool rtcWireHasValidYear(void) {
  return BCD_TO_HEXA(VAR_WIRE_BYTE(VAR_RTC, 7)) != 0;
}

void rtcApplyWrittenRemote(void) {
  rtcApplyRemoteFromWire();
  rtcSyncShadowsFromCache();
  app.dosage.rtcSynced = TRUE;
}

void rtcApplyEventsDefault(void) {
  setTime(0, 0, 0, 1, 1, 1);
  app.eeprom.hourLoaded = TRUE;
  rtcSyncFromTimeLib();
}

bool rtcBootstrapUntilSynced(void) {
  if (Events_ArduinoUNO && !rtcWireHasValidYear()) {
    rtcApplyEventsDefault();
  } else {
    rtcApplyRemoteFromWire();
  }

  rtcSyncShadowsFromCache();
  ChangeGramsAndTime();

  if (Events_ArduinoUNO || rtc.year != 0) {
    app.dosage.rtcSynced = TRUE;
    return true;
  }
  return false;
}
