#include <string.h>
#include "iomap.h"
#include "i2c.h"
#include "timebase.h"

// U2 MCP23017: GPA0..7 = LEDs (activos en bajo), GPB0..7 = switches
// el switch en OFF va a GND y en ON queda abierto -> necesita pull-up interno (si no, flota y parpadea)
// in n = switch en GPBn, out n = LED en GPAn
#define IODIRA 0x00
#define IODIRB 0x01
#define GPPUB  0x0D
#define GPIOB_ 0x13
#define OLATA  0x14

#define IOMAP_PERIODO_MS 10
#define IOMAP_LECTURAS   3      // lecturas iguales seguidas para aceptar un cambio (antirrebote)
#define IOMAP_REFRESCO   25     // cada 25 periodos (250 ms) se reescriben IODIRA y OLATA

static int8_t map[8];        // entrada asignada a cada salida, -1 = sin mapa
static uint8_t inv[8];       // 1 = not
static uint8_t leds_ultimo = 0xFF;
static uint32_t iomap_ultimo = 0;
static uint8_t sw_previo = 0, sw_estable = 0, sw_iguales = 0;
static uint8_t refresco = 0;

void iomap_config(void){
    i2c_write(U2_ADDR, IODIRA, 0x00);
    i2c_write(U2_ADDR, IODIRB, 0xFF);
    i2c_write(U2_ADDR, GPPUB, 0xFF);
    iomap_clear();
}

void iomap_clear(void){
    for(int k = 0; k < 8; k++){
        map[k] = -1;
        inv[k] = 0;
    }
    leds_ultimo = 0xFF;
    i2c_write(U2_ADDR, OLATA, 0xFF);
}

static int es_canal(char *s){
    return s[0] >= '0' && s[0] <= '7' && s[1] == '\0';
}

// IOMAP in out [not] [all]; regresa 0 si el comando es valido
int iomap_cmd(char **tokens, int token_cont){
    int all = 0, no = 0, nums = 0;
    int val[2];
    for(int k = 1; k < token_cont; k++){
        if(strcmp(tokens[k], "all") == 0 || strcmp(tokens[k], "ALL") == 0){
            all = 1;
        }else if(strcmp(tokens[k], "not") == 0 || strcmp(tokens[k], "NOT") == 0){
            no = 1;
        }else if(es_canal(tokens[k]) && nums < 2){
            val[nums++] = tokens[k][0] - '0';
        }else{
            return -1;
        }
    }
    if(all){
        for(int k = 0; k < 8; k++){
            map[k] = k;
            inv[k] = no;
        }
        return 0;
    }
    if(nums != 2){
        return -1;
    }
    map[val[1]] = val[0];
    inv[val[1]] = no;
    return 0;
}

// cada 10 ms lee los switches y actualiza los LEDs mapeados, no bloquea
void iomap_task(void){
    if((ms_ticks - iomap_ultimo) < IOMAP_PERIODO_MS){
        return;
    }
    iomap_ultimo = ms_ticks;

    int hay = 0;
    for(int k = 0; k < 8; k++){
        if(map[k] >= 0) hay = 1;
    }
    if(!hay) return;

    uint8_t sw;
    if(i2c_read(U2_ADDR, GPIOB_, &sw) != 0) return;

    // antirrebote: el cambio cuenta solo si se lee igual varias veces seguidas
    if(sw == sw_previo){
        if(sw_iguales < IOMAP_LECTURAS) sw_iguales++;
    }else{
        sw_iguales = 1;
        sw_previo = sw;
    }
    if(sw_iguales >= IOMAP_LECTURAS){
        sw_estable = sw;
    }

    uint8_t leds = 0;     // 1 = LED encendido
    for(int k = 0; k < 8; k++){
        if(map[k] >= 0){
            uint8_t bit = (sw_estable >> map[k]) & 1;
            if(inv[k]) bit ^= 1;
            leds |= (bit << k);
        }
    }
    leds = ~leds;         // activos en bajo

    // si el MCP se reinicio o perdio una escritura, el refresco lo corrige
    refresco++;
    if(refresco >= IOMAP_REFRESCO){
        refresco = 0;
        i2c_write(U2_ADDR, IODIRA, 0x00);
        i2c_write(U2_ADDR, GPPUB, 0xFF);
        leds_ultimo = ~leds;
    }
    if(leds != leds_ultimo){
        leds_ultimo = leds;
        i2c_write(U2_ADDR, OLATA, leds);
    }
}
