#ifndef LCD_H
#define LCD_H

#include <stdint.h>

void lcd_config(void);
void lcd_clear(void);
void lcd_goto(uint8_t x, uint8_t fila);
void lcd_putc(char c);
void lcd_print(char *s);
uint8_t lcd_font_col(char c, int k);
void lcd_send_buffer(const uint8_t *buf);

#endif
