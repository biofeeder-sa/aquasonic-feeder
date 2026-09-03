#ifndef AQUASONIC_EEPROM_CONFIG_H_
#define AQUASONIC_EEPROM_CONFIG_H_
#include <stdint.h>
/* Direcciones EEPROM auxiliares (no estan en transferVar) */
#define EEPROM_ADDR_AUX_DAY       0x4Cu
#define EEPROM_ADDR_COMPARING_DAY 0xA3u
#define EEPROM_ADDR_X2_WEAR_HEALTHY 0xEBu
#define EEPROM_ADDR_X3_WEAR_HEALTHY 0xE8u
#define EEPROM_ADDR_MOTOR_CHANGE_PENDING 0xF8u
#define EEPROM_ADDR_X2_MOTOR_BACKUP      0xF9u
#define EEPROM_ADDR_X3_MOTOR_BACKUP      0x103u
#define MOTOR_WEAR_BACKUP_SIZE           10u
void initConfig(void);
void saveInEeprom(int row);
void seedFactoryDefaults(void);
void postInitConfig(void);
#endif
