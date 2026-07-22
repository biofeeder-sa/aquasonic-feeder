#ifndef AQUASONIC_APP_STATE_H_
#define AQUASONIC_APP_STATE_H_
#include <stdint.h>

typedef struct {
  unsigned char dosingActive   :1;
  unsigned char changeTableAm  :1;
  unsigned char changeTablePm  :1;
  unsigned char rtcSynced      :1;
  unsigned char calibrating    :1;
} DosageFlags;

typedef struct {
  uint32_t minBatteryInFeed;
  int calibGrams;
  unsigned long calibStartMs;
  unsigned long calibDurationMs;
  uint16_t targetRevolutions;
  float revolutionFraction;
  unsigned long sprayerStartMs;
  unsigned long dosingStartMs;
  unsigned long dosingTotalMs;
  unsigned long motorX1StartMs;
} DosageRuntime;

typedef struct {
  unsigned char deliveryAck     :1;
  unsigned char answerBroadcast :1;
  unsigned char initAlarms      :1;
  unsigned char initBroadcast   :1;
  unsigned char sendUnicast     :1;
} CommFlags;

typedef struct {
  unsigned char alarmLowVoltage :1;
} SensorFlags;

typedef struct {
  unsigned char ready      :1;
  unsigned char hourLoaded :1;
} EepromFlags;

typedef struct {
  DosageFlags    dosage;
  DosageRuntime  dosageRt;
  CommFlags      comm;
  SensorFlags    sensor;
  EepromFlags    eeprom;
} AppState;

extern AppState app;

void appStateInit(void);

#endif /* AQUASONIC_APP_STATE_H_ */
