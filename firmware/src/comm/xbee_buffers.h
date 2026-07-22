#ifndef AQUASONIC_XBEE_BUFFERS_H_
#define AQUASONIC_XBEE_BUFFERS_H_
#include <stdint.h>

typedef struct {
  uint8_t checksumErrors;
  uint8_t frameOverflow;
  uint8_t txDeliveryErrors;
  uint8_t lastTxDeliveryStatus;
  uint8_t lastAppError;
} XbeeStats;

typedef struct {
  uint32_t ackTimeA;
  uint32_t ackTimeB;
  uint8_t ackMultiplier;
  uint32_t broadcastTimeA;
  uint32_t broadcastTimeB;
  uint8_t broadcastMultiplier;
  uint32_t lastActivityMs;
  uint8_t alarmsSnapshot[4];
} XbeeCommState;

typedef struct {
  uint8_t txFrame[100];
  uint8_t readRequest[100];
  uint8_t writeResponse[4];
  uint8_t broadcast[33];
  uint8_t unicast[33];
  uint8_t alarms[100];
  uint8_t replyAddr[8];
  uint8_t battBeforeFeed[6];
  uint8_t battAfterFeed[6];
  XbeeStats stats;
  XbeeCommState comm;
  uint8_t payloadSize;
  uint8_t rxIndex;
  uint8_t alarmsVarCount;
} XbeeBuffers;

extern XbeeBuffers xbee;

#endif /* AQUASONIC_XBEE_BUFFERS_H_ */
