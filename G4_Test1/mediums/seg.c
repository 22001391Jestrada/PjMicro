#include "seg.h"
#include "i2c.h"
#include "timebase.h"

// U4 MCP23017: GPA0..GPA7 = segmentos A..G, DP (activos en alto)
// GPB3 = DIGIT1 (izquierda) ... GPB0 = DIGIT4 (derecha), GPB4..7 = botones
#define IODIRA 0x00
#define IODIRB 0x01
#define OLATA  0x14
#define OLATB  0x15

#define SEG_PERIODO_MS 3

// 0-9, A-F
static const uint8_t seg_tabla[16] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
    0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71
};

static uint8_t seg_digitos[4];   // patron de segmentos de cada digito, [0] = izquierda
static uint8_t seg_on = 0;
static uint8_t seg_actual = 0;
static uint32_t seg_ultimo = 0;

void seg_config(void){
    i2c_write(U4_ADDR, IODIRA, 0x00);
    i2c_write(U4_ADDR, IODIRB, 0xF0);
    i2c_write(U4_ADDR, OLATA, 0x00);
    i2c_write(U4_ADDR, OLATB, 0x00);
}

// separa el numero en 4 digitos en la base dada, sin ceros a la izquierda
static void seg_cargar(uint32_t num, uint32_t base){
    for(int k = 3; k >= 0; k--){
        seg_digitos[k] = seg_tabla[num % base];
        num /= base;
        if(num == 0){
            for(int j = k - 1; j >= 0; j--){
                seg_digitos[j] = 0x00;
            }
            break;
        }
    }
    seg_on = 1;
}

void seg_show_dec(uint32_t num){
    seg_cargar(num % 10000, 10);
}

void seg_show_hex(uint32_t num){
    seg_cargar(num & 0xFFFF, 16);
}

void seg_off(void){
    seg_on = 0;
    i2c_write(U4_ADDR, OLATB, 0x00);
}

// multiplexado: un digito a la vez cada SEG_PERIODO_MS, no bloquea
void seg_task(void){
    if(!seg_on || (ms_ticks - seg_ultimo) < SEG_PERIODO_MS){
        return;
    }
    seg_ultimo = ms_ticks;
    seg_actual = (seg_actual + 1) % 4;

    i2c_write(U4_ADDR, OLATB, 0x00);                         // apagar digitos (evita fantasmas)
    i2c_write(U4_ADDR, OLATA, seg_digitos[seg_actual]);      // segmentos
    i2c_write(U4_ADDR, OLATB, 1U << (3 - seg_actual));       // prender el digito
}
