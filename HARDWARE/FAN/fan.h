#ifndef __FAN_H
#define __FAN_H
#include "sys.h"

#define FAN_OFF             0
#define FAN_LOW_SPEED       1
#define FAN_HIGH_SPEED      2

//控制风扇的电磁继电器，低电平有效
void FAN_Init(void);

void fan_set(u8 fan_status);

#endif
