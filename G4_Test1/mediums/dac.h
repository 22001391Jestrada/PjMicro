#ifndef DAC_H
#define DAC_H

#include <stdint.h>

void dac_config(void);
void vout(float voltage, uint32_t res);

#endif
