#ifndef AQUASONIC_XBEE_PARSER_H_
#define AQUASONIC_XBEE_PARSER_H_
#include <stdint.h>
void xbeeParserReset(void);
void xbeeParserPoll(void);
bool xbeeValidateApiFrame(const uint8_t* frame, uint16_t len);
#endif /* AQUASONIC_XBEE_PARSER_H_ */
