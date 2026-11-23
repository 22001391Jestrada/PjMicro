#ifndef ADC_H
#define ADC_H

#include <stdint.h>

void adc_config(void);
uint32_t adc_read(uint32_t res);

#endif
