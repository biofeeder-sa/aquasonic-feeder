#ifndef AQUASONIC_EEPROM_CONFIG_H_
#define AQUASONIC_EEPROM_CONFIG_H_
#include <stdint.h>
/* Direcciones EEPROM auxiliares (no estan en transferVar) */
#define EEPROM_ADDR_AUX_DAY       0x4Cu
#define EEPROM_ADDR_COMPARING_DAY 0xA3u
void initConfig(void);
void saveInEeprom(int row);
void seedFactoryDefaults(void);
void postInitConfig(void);
#endif
