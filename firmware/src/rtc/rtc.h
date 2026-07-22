#ifndef RTC_H
#define RTC_H
#include <stdint.h>
void Internal_Clock(void);
void Print_Hour(void);
uint8_t BCD_TO_HEXA(uint8_t num);
uint8_t HEXA_TO_BCD(uint8_t byte);
void ChangeGramsAndTime(void);
void forTimeAlam(void);
uint8_t calculatePercentX(void);
#endif
