#ifndef AQUASONIC_CONFIG_H_
#define AQUASONIC_CONFIG_H_
#include <stdint.h>
/*****  ESTADOS LOGICOS *****/
#define TRUE        1
#define FALSE       0
#define ON          1
#define OFF         0
#define LED_ON      0
#define LED_OFF     1
/*****  DEFINICION DE SALIDAS DIGITALES *****/
#define X3          5
#define X2          3
#define X1          6
#define X0_1        7
#define X0_2        2
#define LED_ACTIVE  3
#define LED_STANDBY 6
#define LED         13
#define ADDO        8
#define ADSK        9
#define PIN_RESET   25
#define OE_DRIVER   26
/*****  DEFINICION DE PINES ANALOGICOS  *****/
#define CH_BATTERY  0
#define CH_PANNEL   1
/*****  COMUNICACION XBEE  *****/
#define XBEE_RX     11
#define XBEE_TX     10
#define STARTDELIMITER              0x7E
#define FRAME_HEADER                18
#define FRAME_HEADER_BROADCAST      22
#define FRAME_HEADER_UNICAST        22
#define FRAME_HEADER_ALARMS         22
#define CMD_TRANSMIT_FRAME          0x10
#define CMD_RECEIVE_PACKET          0x90
#define CMD_READ_REQUEST            0x19
#define CMD_READ_RESPONSE           0x23
#define CMD_LOGS_REQUEST            0x20
#define CMD_LOGS_RESPONSE           0x24
#define CMD_WRITE_REQUEST           0x21
#define CMD_WRITE_RESPONSE          0x05
#define CMD_INIT                    0x23
#define CMD_INIT_ALARM              0x23
#define CMD_ERROR                   0xEE
#define CMD_ACK                     0x41
#define CMD_CALIBRATION             0xCA
#define CMD_API_TX_STATUS           0x89
#define CMD_API_MODEM_STATUS        0x8A
#define BYTE_ADDRESS                2
#endif
