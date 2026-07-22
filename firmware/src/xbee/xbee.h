#ifndef XBEE_H
#define XBEE_H
#include <stdint.h>
void create_frame(unsigned char frame[], unsigned char data[], unsigned char data_size, unsigned char frame_id);
void set_checksum(unsigned char frame[], int size);
unsigned char checksum(unsigned char frame[], int size);
void print_frame(const char *header, unsigned char frame[], unsigned char data_size, const char *footer);
void send_frame(unsigned char frame[], int size);
void printDigits(int digits);
void create_init_broadcast(unsigned char frame[], unsigned char data[], unsigned char data_size);
void create_data_broadcast();
void broadcast();
void xbee_communication();
void xbeeProcessApiFrame(void);
void create_init_Alarms(unsigned char frame[], unsigned char data[], unsigned char data_size);
void create_data_Alarms(void);
void Alarms(void);
void Send_Sequence_Alarms(void);
void Send_Sequence_Broadcast(void);
void NewServer(void);
void serialEventRun(void);
void create_init_unicast(unsigned char frame[], unsigned char data[], unsigned char data_size);
void create_data_unicast();
void unicast();
void cleanAlarms();
#endif
