#ifndef LCDWATCH_H
#define LCDWATCH_H

int lcdwatch_cmd(char **tokens, int token_cont);
void lcdwatch_stop(void);
void lcdwatch_task(void);

#endif
