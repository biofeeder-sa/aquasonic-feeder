#include <Arduino.h>
#include "core/config.h"
#include "comm/xbee_buffers.h"
#include "comm/xbee_parser.h"
#include "xbee/xbee.h"
typedef enum {
  XBEE_PARSE_WAIT_START = 0,
  XBEE_PARSE_LEN_MSB,
  XBEE_PARSE_LEN_LSB,
  XBEE_PARSE_BODY
} XbeeParseState;
static XbeeParseState s_parseState = XBEE_PARSE_WAIT_START;
static uint16_t s_parseApiLen = 0;
static uint16_t s_parseIndex = 0;
void xbeeParserReset(void) {
  s_parseState = XBEE_PARSE_WAIT_START;
  s_parseApiLen = 0;
  s_parseIndex = 0;
}
bool xbeeValidateApiFrame(const uint8_t* frame, uint16_t len) {
  if (len < 4 || frame[0] != STARTDELIMITER) {
    return false;
  }
  uint16_t apiLen = ((uint16_t)frame[1] << 8) | frame[2];
  uint16_t totalLen = (uint16_t)(4 + apiLen);
  if (apiLen == 0 || totalLen > len) {
    return false;
  }
  uint8_t sum = 0;
  for (uint16_t i = 3; i < 3 + apiLen + 1; i++) {
    sum += frame[i];
  }
  return sum == 0xFF;
}
static void xbeeParserFeed(uint8_t byte) {
  switch (s_parseState) {
    case XBEE_PARSE_WAIT_START:
      if (byte == STARTDELIMITER) {
        xbee.txFrame[0] = byte;
        s_parseIndex = 1;
        s_parseState = XBEE_PARSE_LEN_MSB;
      }
      break;
    case XBEE_PARSE_LEN_MSB:
      xbee.txFrame[1] = byte;
      s_parseIndex = 2;
      s_parseState = XBEE_PARSE_LEN_LSB;
      break;
    case XBEE_PARSE_LEN_LSB:
      xbee.txFrame[2] = byte;
      s_parseApiLen = ((uint16_t)xbee.txFrame[1] << 8) | byte;
      if (s_parseApiLen == 0 || s_parseApiLen > (sizeof(xbee.txFrame) - 4)) {
        xbeeParserReset();
        break;
      }
      s_parseIndex = 3;
      s_parseState = XBEE_PARSE_BODY;
      break;
    case XBEE_PARSE_BODY:
      if (s_parseIndex >= sizeof(xbee.txFrame)) {
        xbee.stats.frameOverflow++;
        xbeeParserReset();
        break;
      }
      xbee.txFrame[s_parseIndex++] = byte;
      if (s_parseIndex >= (uint16_t)(3 + s_parseApiLen + 1)) {
        xbee.rxIndex = (uint8_t)s_parseIndex;
        if (xbeeValidateApiFrame(xbee.txFrame, s_parseIndex)) {
          xbeeProcessApiFrame();
        } else {
          xbee.stats.checksumErrors++;
          Serial.println(F("XBee: checksum RX invalido"));
        }
        xbeeParserReset();
      }
      break;
  }
}
void xbeeParserPoll(void) {
  while (Serial3.available() > 0) {
    xbeeParserFeed((uint8_t)Serial3.read());
  }
}
