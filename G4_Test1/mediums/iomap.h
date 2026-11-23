#ifndef IOMAP_H
#define IOMAP_H

#include <stdint.h>

#define U2_ADDR 0x24

void iomap_config(void);
int iomap_cmd(char **tokens, int token_cont);
void iomap_clear(void);
void iomap_task(void);

#endif
