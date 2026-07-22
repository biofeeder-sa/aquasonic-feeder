#ifndef AQUASONIC_RTC_CACHE_H_
#define AQUASONIC_RTC_CACHE_H_
#include <stdint.h>
/*
 * Cache unificada de tiempo.
 *
 * - hour..year: reloj binario (TimeLib), fuente para logica local.
 * - shadow*: ultimo valor visto; detectar cambio de unidad sin re-leer EEPROM.
 * - persistedDay / dailyCompareDay: EEPROM auxiliar (fuera de transferVar).
 * - breakCycle / feedMinuteLatch: estado de dosificacion por tabla horaria.
 */
typedef struct {
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t day;
  uint8_t month;
  uint8_t year;  /* offset desde 2000 */
  uint8_t shadowSecond;
  uint8_t shadowMinute;
  uint8_t shadowHour;
  uint8_t shadowDay;
  uint8_t shadowMonth;
  uint8_t shadowYear;
  uint8_t persistedDay;
  uint8_t dailyCompareDay;
  uint8_t breakCycle;
  uint8_t feedMinuteLatch;
  uint32_t ledTimerStart;
  uint32_t ledTimerNow;
} RtcCache;
extern RtcCache rtc;
void rtcSyncFromTimeLib(void);
void rtcPushCacheToWire(void);
void rtcApplyRemoteFromWire(void);
void rtcApplyEventsDefault(void);
void rtcApplyWrittenRemote(void);
bool rtcBootstrapUntilSynced(void);
#endif /* AQUASONIC_RTC_CACHE_H_ */
