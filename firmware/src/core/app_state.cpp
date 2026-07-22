#include "core/app_state.h"
#include "comm/xbee_parser.h"
#include "comm/xbee_buffers.h"
#include "acs/acs_sensor.h"

AppState app;

void appStateInit(void) {
  app.comm.answerBroadcast = 0;
  app.dosage.dosingActive = 0;
  app.dosage.changeTableAm = 0;
  app.dosage.changeTablePm = 0;
  app.dosage.rtcSynced = 0;
  app.dosage.calibrating = 0;
  app.dosageRt.minBatteryInFeed = 0xFFFFFFFF;
  app.sensor.alarmLowVoltage = 0;
  app.comm.deliveryAck = 0;
  app.eeprom.hourLoaded = 0;
  app.eeprom.ready = 0;
  app.comm.sendUnicast = 0;
  app.comm.initBroadcast = 0;
  app.comm.initAlarms = 0;
  xbee.comm.ackMultiplier = 1;
  xbee.comm.broadcastMultiplier = 1;
  acsInit();
  xbeeParserReset();
}
