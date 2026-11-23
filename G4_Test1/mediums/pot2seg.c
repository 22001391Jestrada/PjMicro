#include <string.h>
#include <stdint.h>
#include "pot2seg.h"
#include "adc.h"
#include "seg.h"
#include "timebase.h"

#define POT2SEG_PERIODO_MS 50
#define POT2SEG_MUESTRAS   16      // promedio para que el ultimo digito no brinque

static uint8_t pot2seg_on = 0;
static uint8_t pot2seg_hex = 0;
static uint32_t pot2seg_ultimo = 0;

static int es(char *s, char *min, char *may){
    return strcmp(s, min) == 0 || strcmp(s, may) == 0;
}

// POT2SEG {on|off} [{dec|hex}]; regresa 0 si el comando es valido
int pot2seg_cmd(char **tokens, int token_cont){
    if(token_cont < 2 || token_cont > 3){
        return -1;
    }
    if(es(tokens[1], "off", "OFF")){
        pot2seg_on = 0;
        seg_off();
        return 0;
    }
    if(!es(tokens[1], "on", "ON")){
        return -1;
    }
    pot2seg_hex = 0;
    if(token_cont == 3){
        if(es(tokens[2], "hex", "HEX")){
            pot2seg_hex = 1;
        }else if(!es(tokens[2], "dec", "DEC")){
            return -1;
        }
    }
    pot2seg_on = 1;
    pot2seg_ultimo = ms_ticks - POT2SEG_PERIODO_MS;   // que muestre de una vez
    return 0;
}

// lo usa SEGMENT para quedarse con el display
void pot2seg_stop(void){
    pot2seg_on = 0;
}

// cada 50 ms convierte el pot a 12 bits y lo manda al display, no bloquea
void pot2seg_task(void){
    if(!pot2seg_on || (ms_ticks - pot2seg_ultimo) < POT2SEG_PERIODO_MS){
        return;
    }
    pot2seg_ultimo = ms_ticks;

    uint32_t suma = 0;
    for(int k = 0; k < POT2SEG_MUESTRAS; k++){
        suma += adc_read(12);
    }
    uint32_t valor = suma / POT2SEG_MUESTRAS;

    if(pot2seg_hex){
        seg_show_hex(valor);
    }else{
        seg_show_dec(valor);
    }
}
