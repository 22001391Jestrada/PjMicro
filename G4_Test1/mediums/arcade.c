#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "arcade.h"
#include "lcd.h"
#include "lcdwatch.h"
#include "adc.h"
#include "keypad.h"
#include "timebase.h"

// Breakout en la LCD 84x48. Barra con el POT, * o # lanza cada bola.
#define ANCHO     84
#define ALTO      48
#define FRAME_MS  30
#define BARRA_W   14
#define BARRA_Y   45
#define BOLA      2
#define LAD_FILAS 4
#define LAD_COLS  12
#define LAD_W     7          // 6 pixeles + 1 de espacio
#define LAD_H     4          // 3 pixeles + 1 de espacio
#define LAD_Y0    11
#define TECHO     9          // debajo de la barra de puntos
#define ESCALA    16         // posicion de la bola en 1/16 de pixel
#define VEL_Y     12         // 0.75 pixeles por frame (la LCD es lenta, mas rapido no se ve)
#define VEL_X     6          // velocidad horizontal al lanzar
#define VEL_GIRO  2          // cuanto cambia vx segun donde pega en la barra

enum { APAGADO, ESPERA, JUEGO, PERDIO, GANO };

static uint8_t fb[504];      // pantalla en RAM: 6 bancos de 84 columnas
static uint8_t estado = APAGADO;
static uint16_t ladrillos[LAD_FILAS];
static int32_t bx, by, vx, vy;
static int barra_x;
static uint8_t vidas;
static uint16_t puntos;
static uint8_t tecla_antes = 0;
static uint32_t ultimo = 0;

static int es(char *s, char *min, char *may){
    return strcmp(s, min) == 0 || strcmp(s, may) == 0;
}

static void rect(int x, int y, int w, int h){
    for(int i = x; i < x + w; i++){
        for(int j = y; j < y + h; j++){
            if(i >= 0 && i < ANCHO && j >= 0 && j < ALTO){
                fb[(j >> 3) * ANCHO + i] |= (1U << (j & 7));
            }
        }
    }
}

static void texto(int x, int banco, const char *s){
    while(*s && x <= ANCHO - 6){
        for(int k = 0; k < 5; k++){
            fb[banco * ANCHO + x + k] = lcd_font_col(*s, k);
        }
        x += 6;
        s++;
    }
}

static void juego_nuevo(void){
    for(int f = 0; f < LAD_FILAS; f++){
        ladrillos[f] = (1U << LAD_COLS) - 1;
    }
    vidas = 3;
    puntos = 0;
    estado = ESPERA;
}

// ARCADE breakout | ARCADE off
int arcade_cmd(char **tokens, int token_cont){
    if(token_cont != 2) return -1;
    if(es(tokens[1], "off", "OFF")){
        arcade_stop();
        return 0;
    }
    if(!es(tokens[1], "breakout", "BREAKOUT")) return -1;
    lcdwatch_stop();
    juego_nuevo();
    tecla_antes = 1;          // que no lance si la tecla ya estaba presionada
    ultimo = ms_ticks;
    return 0;
}

void arcade_stop(void){
    if(estado != APAGADO){
        estado = APAGADO;
        lcd_clear();
    }
}

static void mover(void){
    bx += vx;
    by += vy;

    // paredes y techo
    if(bx < 0){ bx = 0; vx = -vx; }
    if(bx > (ANCHO - BOLA) * ESCALA){ bx = (ANCHO - BOLA) * ESCALA; vx = -vx; }
    if(by < TECHO * ESCALA){ by = TECHO * ESCALA; vy = -vy; }

    int x = bx / ESCALA;
    int y = by / ESCALA;

    // barra: el angulo depende de donde pega
    if(vy > 0 && y + BOLA >= BARRA_Y && y + BOLA <= BARRA_Y + 2 && x + BOLA > barra_x && x < barra_x + BARRA_W){
        by = (BARRA_Y - BOLA) * ESCALA;
        vy = -vy;
        int off = (x + 1) - (barra_x + BARRA_W / 2);   // -7 .. 7
        vx = off * VEL_GIRO;
        if(vx == 0) vx = (ms_ticks & 1) ? VEL_GIRO : -VEL_GIRO;
    }

    // ladrillos: se revisa el centro de la bola
    int cx = x + 1, cy = y + 1;
    if(cy >= LAD_Y0 && cy < LAD_Y0 + LAD_FILAS * LAD_H){
        int f = (cy - LAD_Y0) / LAD_H;
        int c = cx / LAD_W;
        if(c < LAD_COLS && (ladrillos[f] & (1U << c))){
            ladrillos[f] &= ~(1U << c);
            vy = -vy;
            puntos += 10;
            int quedan = 0;
            for(int k = 0; k < LAD_FILAS; k++) quedan |= ladrillos[k];
            if(!quedan) estado = GANO;
        }
    }

    // se cayo la bola
    if(y >= ALTO){
        vidas--;
        estado = vidas ? ESPERA : PERDIO;
    }
}

static void dibujar(void){
    char buf[16];
    memset(fb, 0, sizeof(fb));

    snprintf(buf, sizeof(buf), "P:%u", puntos);
    texto(0, 0, buf);
    snprintf(buf, sizeof(buf), "V:%u", vidas);
    texto(ANCHO - 18, 0, buf);
    rect(0, 8, ANCHO, 1);

    if(estado == PERDIO || estado == GANO){
        texto(estado == PERDIO ? 15 : 18, 3, estado == PERDIO ? "GAME OVER" : "GANASTE!");
        texto(3, 4, "* = de nuevo");
        return;
    }

    for(int f = 0; f < LAD_FILAS; f++){
        for(int c = 0; c < LAD_COLS; c++){
            if(ladrillos[f] & (1U << c)){
                rect(c * LAD_W, LAD_Y0 + f * LAD_H, LAD_W - 1, LAD_H - 1);
            }
        }
    }
    rect(barra_x, BARRA_Y, BARRA_W, 2);
    rect(bx / ESCALA, by / ESCALA, BOLA, BOLA);

    if(estado == ESPERA){
        texto(9, 4, "* = lanzar");
    }
}

// un frame cada 30 ms, no bloquea al monitor
void arcade_task(void){
    if(estado == APAGADO || (ms_ticks - ultimo) < FRAME_MS){
        return;
    }
    ultimo = ms_ticks;

    // barra: posicion absoluta con el POT
    uint32_t suma = 0;
    for(int k = 0; k < 4; k++){
        suma += adc_read(12);
    }
    barra_x = (suma / 4) * (ANCHO - BARRA_W) / 4095;

    // * o #, solo al presionar (flanco)
    uint8_t tecla = keypad_star_hash();
    uint8_t presiono = tecla && !tecla_antes;
    tecla_antes = tecla;

    if(estado == ESPERA){
        bx = (barra_x + BARRA_W / 2 - 1) * ESCALA;
        by = (BARRA_Y - BOLA) * ESCALA;
        if(presiono){
            vy = -VEL_Y;
            vx = (ms_ticks & 1) ? VEL_X : -VEL_X;
            estado = JUEGO;
        }
    }else if(estado == JUEGO){
        mover();
    }else if(presiono){
        juego_nuevo();
    }

    dibujar();
    lcd_send_buffer(fb);
}
