#ifndef RGB_H
#define RGB_H

#include <stdint.h>

void rgb_config(void);
void rgb_set(uint32_t color);
int rgb_isValid(char *str);

#endif
