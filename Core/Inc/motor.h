#ifndef MOTOR_H
#define MOTOR_H

#include "stm32f0xx.h"
#include <stdint.h>

void motor_open(void);
void motor_close(void);
void motor_stop(void);
void motor_set_speed(uint8_t duty);

#endif