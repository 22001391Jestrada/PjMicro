#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "lcdwatch.h"
#include "lcd.h"
#include "timebase.h"

#define WATCH_MAX        4
#define WATCH_PERIODO_MS 100

typedef struct {
    uint32_t addr;
    uint8_t hex;
    uint8_t size;     // 1, 2 o 4 bytes
} watch_t;

static watch_t watches[WATCH_MAX];
static uint8_t n_watch = 0;
static uint8_t next_watch = 0;
static uint32_t watch_ultimo = 0;

static int es(char *s, char *min, char *may){
    return strcmp(s, min) == 0 || strcmp(s, may) == 0;
}

// solo RAM, CCM, flash y perifericos; leer otra cosa da HardFault
static int addr_valida(uint32_t a, uint8_t size){
    if(a % size != 0) return 0;
    if(a >= 0x20000000 && a + size <= 0x20008000) return 1;
    if(a >= 0x10000000 && a + size <= 0x10002800) return 1;
    if(a >= 0x08000000 && a + size <= 0x08020000) return 1;
    if(a >= 0x40000000 && a + size <= 0x60000000) return 1;
    return 0;
}

// LCDWATCH addr [dec|hex] [b|hw|w]  |  LCDWATCH off
int lcdwatch_cmd(char **tokens, int token_cont){
    if(token_cont < 2) return -1;
    if(es(tokens[1], "off", "OFF")){
        lcdwatch_stop();
        lcd_clear();
        return 0;
    }

    watch_t w;
    w.addr = strtoul(tokens[1], NULL, 16);
    w.hex = 0;
    w.size = 1;
    for(int k = 2; k < token_cont; k++){
        if(es(tokens[k], "dec", "DEC"))      w.hex = 0;
        else if(es(tokens[k], "hex", "HEX")) w.hex = 1;
        else if(es(tokens[k], "b", "B"))     w.size = 1;
        else if(es(tokens[k], "hw", "HW"))   w.size = 2;
        else if(es(tokens[k], "w", "W"))     w.size = 4;
        else return -1;
    }
    if(!addr_valida(w.addr, w.size)) return -2;

    if(n_watch == 0) lcd_clear();
    watches[next_watch] = w;
    next_watch = (next_watch + 1) % WATCH_MAX;
    if(n_watch < WATCH_MAX) n_watch++;
    watch_ultimo = ms_ticks - WATCH_PERIODO_MS;   // que se vea de una vez
    return 0;
}

void lcdwatch_stop(void){
    n_watch = 0;
    next_watch = 0;
}

// cada 100 ms lee cada direccion y la escribe en su fila (0-3), no bloquea
void lcdwatch_task(void){
    if(n_watch == 0 || (ms_ticks - watch_ultimo) < WATCH_PERIODO_MS){
        return;
    }
    watch_ultimo = ms_ticks;

    char buf[16];
    for(int k = 0; k < n_watch; k++){
        watch_t *w = &watches[k];
        uint32_t val;
        if(w->size == 1)      val = *(volatile uint8_t *)w->addr;
        else if(w->size == 2) val = *(volatile uint16_t *)w->addr;
        else                  val = *(volatile uint32_t *)w->addr;

        // 14 caracteres: 4 digitos bajos de la direccion + valor alineado a la derecha
        uint32_t a = w->addr & 0xFFFF;
        if(w->hex){
            if(w->size == 1)      snprintf(buf, sizeof(buf), "%04lX:     0x%02lX", a, val);
            else if(w->size == 2) snprintf(buf, sizeof(buf), "%04lX:   0x%04lX", a, val);
            else                  snprintf(buf, sizeof(buf), "%04lX: %08lX", a, val);
        }else{
            if(val > 999999999)   snprintf(buf, sizeof(buf), "%04lX%10lu", a, val);
            else                  snprintf(buf, sizeof(buf), "%04lX:%9lu", a, val);
        }

        lcd_goto(0, k);
        for(int c = 0; c < 14 && buf[c]; c++){
            lcd_putc(buf[c]);
        }
    }
}
