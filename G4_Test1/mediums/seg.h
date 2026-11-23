#ifndef SEG_H
#define SEG_H

#include <stdint.h>

#define U4_ADDR 0x20

void seg_config(void);
void seg_show_dec(uint32_t num);
void seg_show_hex(uint32_t num);
void seg_off(void);
void seg_task(void);

#endif
