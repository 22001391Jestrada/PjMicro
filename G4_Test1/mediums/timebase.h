#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <stdint.h>

extern volatile uint32_t ms_ticks;

void timebase_config(void);
void delay_ms(uint32_t ms);

#endif
