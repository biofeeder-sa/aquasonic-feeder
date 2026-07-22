/*
  GlobalVariables.h - Registro central de variables (generado)
  Alimentador Aquasonic — formato compatible con proyecto hub.

  NO EDITAR A MANO: usar firmware/tools/gen_var_registry.py

  Columnas:
    [0-1] ID MSB/LSB
    [2]   SIZE (bytes del valor, sin ID)
    [3]   FORMAT
    [4]   EEPROM (1=persiste en EEPROM interna ATmega)
    [5-6] EEPROM ADDRESS MSB/LSB
    [7]   WR (1=RW remoto XBee, 0=solo lectura remota)
    [8]   LOG_TIME (minutos, 0=sin log periodico)
    [9 .. 9+SIZE-1] VALUE (exactamente SIZE bytes; resto de COL se inicializa en 0)

*/
#ifndef AQUASONIC_GLOBAL_VARIABLES_H_
#define AQUASONIC_GLOBAL_VARIABLES_H_

#include <Arduino.h>
#include <stdint.h>

/* Indices de fila */
#define VAR_SETTING_BYTES      0
#define VAR_SOFTWARE_VERSION   1
#define VAR_FEED_RATE          2
#define VAR_CYCLE_TIME         3
#define VAR_CYCLE_GRAMS        4
#define VAR_PROFILE            5
#define VAR_HARDWARE_VERSION   6
#define VAR_PROJECT_NAME       7
#define VAR_AM_TABLE           8
#define VAR_PM_TABLE           9
#define VAR_ALARMS             10
#define VAR_ALARM_MASK         11
#define VAR_RTC                12
#define VAR_RTC_RESET          13
#define VAR_RESET_COUNT        14
#define VAR_SERVER             15
#define VAR_BATTERY            16
#define VAR_PANNEL             17
#define VAR_BATTERY_TARA       18
#define VAR_PANNEL_TARA        19
#define VAR_LOW_BATTERY        20
#define VAR_ID_DOSAGE          21
#define VAR_ACCUMULATED_GRAMS  22
#define VAR_DOSED_GRAMS        23
#define VAR_TIME_ALARM         24
#define VAR_PERCENT_MOTOR      25
#define VAR_N_VUELTAS          26
#define VAR_T_VUELTAS          27
#define VAR_PAN_ID             28
#define VAR_ACS_TYPE           29
#define VAR_AMP_X1             30
#define VAR_AMP_X2             31
#define VAR_AMP_X3             32
#define VAR_PROT_X1            33
#define VAR_PROT_X2            34
#define VAR_PROT_X3            35
#define VAR_DISCONNECTED       36
#define VAR_EMPTY_HOPPER       37
#define VAR_AMP_WARN_X2        38
#define VAR_AMP_WARN_X3        39
#define VAR_AMP_MAX_X2         40
#define VAR_AMP_MIN_X2         41
#define VAR_AMP_MAX_X3         42
#define VAR_AMP_MIN_X3         43
#define VAR_RESET_EEPROM       44

/* Columnas (igual que hub) */
#define VAR_ID              0
#define VAR_SIZE            2
#define VAR_FORMAT          3
#define VAR_EEPROM          4
#define VAR_EEPROM_ADDRESS  5
#define VAR_WR              7
#define VAR_LOG_TIME        8
#define VAR_VALUE           9

#define ROW 45
#define COL 45   /* 9 meta + hasta 36 bytes VALUE por fila */

extern uint8_t transferVar[ROW][COL];

#endif /* AQUASONIC_GLOBAL_VARIABLES_H_ */
