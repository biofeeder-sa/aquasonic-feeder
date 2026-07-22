#ifndef ACTUATORS_H
#define ACTUATORS_H
#include <stdint.h>
void activarPWM(int valorPWM, int output);
void InitLed(void);
void motorStarting(int _delay, int _interval, int _motor, uint8_t percent);
#endif
