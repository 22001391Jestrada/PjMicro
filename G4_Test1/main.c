#include "RTE_Components.h"
#include <stdint.h>
#include CMSIS_device_header
#include "USART.h"
#include "mediums/buzzer.h"
#include "mediums/rgb.h"
#include "mediums/dac.h"
#include "mediums/adc.h"
#include "mediums/lcd.h"
#include "mediums/i2c.h"
#include "mediums/timebase.h"
#include "mediums/seg.h"
#include "mediums/tasks.h"
#include "mediums/iomap.h"
#include "mediums/pot2seg.h"
#include "mediums/lcdwatch.h"
#include "mediums/keypad.h"
#include "mediums/arcade.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


extern char *register_display(void);									//trabajar en ensamblador
extern void register_modify(int reg_num, uint32_t data);    	        //trabajar en ensamblador
extern void run(uint32_t addr);										    //trabajar en ensamblador
extern void call(uint32_t addr);										//trabajar en ensamblador

int register_isValid(char *str){
    if(str == NULL || (str[0] != 'R' && str[0] != 'r')){
        USART_putString("Formato de registro invalido, el formato debe ser: R{X} o r{x}\r\n");
        return 0;
    }
    int i = 1;
    while(str[i] != '\0'){
        if (!isdigit((unsigned char)str[i])){
            USART_putString("R{X} X debe ser un numero [0,14]\r\n");
            return 0;
        }
        i++;
    }
    int num = atoi(&str[1]);
    if(num >= 0 && num <= 14){
        return 1;
    }else{
        USART_putString("Numero de registro invalido, se acepta del [0,14]\r\n");
    }
    return 0;
}

int data_isValid(char *str, char *type){
    if(str == NULL || str[0] != '0' || (str[1] != 'x' && str[1] != 'X')){
        USART_putString("Formato de data invalido, el formato debe ser: 0x...\r\n");
        return 0;
    }
    int i = 2;
    if (str[i] == '\0') {
        return 0;
    }

    while(str[i] != '\0'){
        char c = str[i];
        if (!isdigit((unsigned char)c) && 
            !(c >= 'A' && c <= 'F') && 
            !(c >= 'a' && c <= 'f')) {
            USART_putString("Caracter invalido en Data (escribir en HEX)\r\n");
            return 0; // Encontró un carácter no válido
        }
        i++;
    }
    if(i > 10){
        if(strcmp(type, "data") == 0){
            USART_putString("Tamano de data invalido maximo 8 digitos hex\r\n");
        }else{
            USART_putString("Direccion invalida, maximo 8 digitos hex\r\n");
        }
        return 0;
    }
    return 1;
}

void memory_display(char *start_str, char *end_str){
    uint32_t start = strtoul(start_str, NULL, 16);
    uint32_t end = strtoul(end_str, NULL, 16);
    char buf[24];
    for(uint32_t addr = start; addr<=end; addr+=4){
        uint32_t word = *(volatile uint32_t *)addr;
        snprintf(buf, sizeof(buf), "%08lX: %08lX\r\n", (unsigned long)addr, (unsigned long)word);
        USART_putString(buf);
    }
}

void memory_modify(char *start_addr, char *data_str, char *size_str){
    uint32_t start = strtoul(start_addr, NULL, 16);
    uint32_t data = strtoul(data_str, NULL, 16);
    int size = (size_str != NULL) ? atoi(size_str) : 1;
    uint32_t max_value;
    switch (size) {
        case 1: max_value = 0xFF;       break;
        case 2: max_value = 0xFFFF;     break;
        case 4: max_value = 0xFFFFFFFF; break;
        default:
            USART_putString("Tamano invalido, usa 1, 2 o 4\r\n");
            return;
    }

    if (data > max_value) {
        USART_putString("El dato no cabe en el tamano indicado\r\n");
        return;
    }
    
    switch(size){
        case 1:
            *(volatile uint8_t *)start = (uint8_t) data;
            break;
        case 2:
            *(volatile uint16_t *)start = (uint16_t) data;
            break;
        case 4:
            *(volatile uint32_t *)start = (uint32_t) data;
            break;
        default:
            *(volatile uint8_t *)start = (uint8_t) data;
            break;
    }
}

void block_fill(char *start_addr, char *end_addr, char *data_str, char *size_str){
    uint32_t start = strtoul(start_addr, NULL, 16);
    uint32_t end = strtoul(end_addr, NULL, 16);
    uint32_t data  = strtoul(data_str, NULL, 16);
    uint8_t size = (size_str != NULL)? atoi(size_str) : 1;
    uint32_t max_value;
    switch (size) {
        case 1: max_value = 0xFF;       break;
        case 2: max_value = 0xFFFF;     break;
        case 4: max_value = 0xFFFFFFFF; break;
        default:
            USART_putString("Tamano invalido, usa 1, 2 o 4\r\n");
            return;
    }

    if (data > max_value) {
        USART_putString("El dato no cabe en el tamano indicado\r\n");
        return;
    }

    if (end < start) {
        USART_putString("Rango invalido: end < start\r\n");
        return;
    }

    switch (size) {
        case 1:
            for (uint32_t addr = start; addr <= end; addr += 1)
                *(volatile uint8_t *)addr = (uint8_t)data;
            break;
        case 2:
            for (uint32_t addr = start; addr <= end; addr += 2)
                *(volatile uint16_t *)addr = (uint16_t)data;
            break;
        case 4:
            for (uint32_t addr = start; addr <= end; addr += 4)
                *(volatile uint32_t *)addr = (uint32_t)data;
            break;
    }
}

int main(void){
	char *tokens[10];
	USART_config(115200);
	buzzer_config();
	lcd_config();
	rgb_config();
	dac_config();
	adc_config();
	timebase_config();
	i2c_config();
	seg_config();
	iomap_config();
	keypad_config();

	USART_putString("\n\r         Proyecto Final Microprocesadores 2026       \n\r");
	USART_putString("                    Programa Monitor                 \n\r");
	USART_putString("                      Jose Estrada                  \n\r\n\r");
	USART_putString(">> ");
	
	while(1){
        if(cmd_ready){
			int token_cont = 0;
            char *token = strtok(rx_data, " \r\n");
			while(token != NULL && token_cont < 10){
				tokens[token_cont++] = token;
				token = strtok(NULL, " \r\n");
			}
			if (token_cont > 0) {
                if (strcmp(tokens[0], "RD") == 0) {
                    uint32_t *regs = (uint32_t *)register_display(); 
                    char buffer[400];
                    snprintf(buffer, sizeof(buffer), 
                    "R0: %08X \r\n R1: %08X \r\n R2: %08X \r\n R3: %08X\r\n R4: %08X\r\n R5: %08X\r\n R6: %08X\r\n R7: %08X \r\n R8: %08X \r\n R9: %08X \r\n R10: %08X\r\n R11: %08X\r\n R12: %08X\r\n R13: %08X\r\n R14: %08X\r\n R15: %08X\r\n", 
                    regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7], regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15]);
                    USART_putString(buffer);
                } 
                else if (strcmp(tokens[0], "RM") == 0) {
                    if(token_cont >= 3 && register_isValid(tokens[1]) && data_isValid(tokens[2], "data")){
                        int num = atoi(&tokens[1][1]);
                        uint32_t data = strtoul(tokens[2], NULL, 16);
                        register_modify(num, data);
                    }
                } 
                else if (strcmp(tokens[0], "MD") == 0) {
                    char *default_addr = "0x20002424"; // Reemplaza por tu valor por defecto
                    char *default_addr2 = "0x20002440";   // Reemplaza por tu valor por defecto

                    if (token_cont < 2) {
                        tokens[1] = default_addr;
                        tokens[2] = default_addr2;
                    } else if (token_cont < 3) {
                        tokens[2] = tokens[1];
                    }

                    if(data_isValid(tokens[1], "addr") && data_isValid(tokens[2], "addr")){
                        memory_display(tokens[1], tokens[2]);
                    }
                } 
                else if (strcmp(tokens[0], "MM") == 0) {
                    if (token_cont < 4){
                        tokens[3] = "1";
                    }
                    if(token_cont >= 3 && data_isValid(tokens[1], "addr") && data_isValid(tokens[2], "data")){
                        memory_modify(tokens[1], tokens[2], tokens[3]);
                    }
                } 
                else if (strcmp(tokens[0], "BF") == 0) {
                    if(token_cont >= 4 && data_isValid(tokens[1], "addr") && data_isValid(tokens[2], "addr") && data_isValid(tokens[3],"data")){
                        block_fill(tokens[1], tokens[2], tokens[3], (token_cont >= 5) ? tokens[4] : NULL);
                    }
                } 
                else if (strcmp(tokens[0], "RUN") == 0) {
                    if(token_cont >= 2 && data_isValid(tokens[1], "addr")){
                        uint32_t addr = strtoul(tokens[1], NULL, 16);
                        run(addr);
                    }else{
                        USART_putString("\n\r data no valida o direccion no especificada");
                    }
                } 
                else if (strcmp(tokens[0], "CALL") == 0) {
                    if(token_cont >= 2 && data_isValid(tokens[1], "addr")){
                        uint32_t addr = strtoul(tokens[1], NULL, 16);
                        call(addr);
                    }else{
                        USART_putString("\n\r data no valida o direccion no especificada");
                    }
                }
                else if (strcmp(tokens[0], "SOUND") == 0) {
                    char *end;
                    uint32_t freq = (token_cont >= 2) ? strtoul(tokens[1], &end, 10) : 0;
                    if(token_cont >= 2 && *end == '\0' && freq <= 100000){
                        sound(freq);
                    }else{
                        USART_putString("Uso: SOUND <freq> (0 a 100000 Hz)\r\n");
                    }
                }
                else if (strcmp(tokens[0], "MUTE") == 0) {
                    sound(0);
                }
                else if (strcmp(tokens[0], "RGBOUT") == 0) {
                    if(token_cont >= 2 && rgb_isValid(tokens[1])){
                        rgb_set(strtoul(tokens[1], NULL, 16));
                    }else{
                        USART_putString("Uso: RGBOUT <RRGGBB> en hex, ej. FF0000\r\n");
                    }
                }
                else if (strcmp(tokens[0], "VOUT") == 0) {
                    char *end1 = "";
                    char *end2 = "";
                    float volt = (token_cont >= 2) ? strtof(tokens[1], &end1) : -1.0f;
                    uint32_t res = (token_cont >= 3) ? strtoul(tokens[2], &end2, 10) : 8;
                    if(token_cont >= 2 && *end1 == '\0' && volt >= 0.0f && volt <= 3.3f && *end2 == '\0' && res >= 1 && res <= 12){
                        vout(volt, res);
                    }else{
                        USART_putString("Uso: VOUT <voltaje 0-3.3> [res 1-12, default 8]\r\n");
                    }
                }
                else if (strcmp(tokens[0], "VIN") == 0) {
                    uint32_t res = (token_cont >= 2) ? atoi(tokens[1]) : 8;
                    if(res == 6 || res == 8 || res == 10 || res == 12){
                        uint32_t valor = adc_read(res);
                        float volt = valor * 3.3f / ((1U << res) - 1);
                        char buf[48];
                        snprintf(buf, sizeof(buf), "%.4f V  (%lu)\r\n", volt, (unsigned long)valor);
                        USART_putString(buf);
                    }else{
                        USART_putString("Uso: VIN [res] (6, 8, 10 o 12 bits, default 8)\r\n");
                    }
                }
                else if (strcmp(tokens[0], "LCD") == 0) {
                    // el texto trae espacios, se toma de la copia de la linea
                    char *txt = rx_line;
                    while(isspace((unsigned char)*txt)) txt++;
                    txt += 3;
                    while(isspace((unsigned char)*txt)) txt++;
                    lcdwatch_stop();
                    arcade_stop();
                    lcd_print(txt);
                }
                else if (strcmp(tokens[0], "SEGMENT") == 0) {
                    char *end;
                    uint32_t num = (token_cont >= 2) ? strtoul(tokens[1], &end, 10) : 0;
                    if(token_cont >= 2 && isdigit((unsigned char)tokens[1][0]) && *end == '\0' && num <= 9999){
                        pot2seg_stop();
                        seg_show_dec(num);
                    }else{
                        USART_putString("Uso: SEGMENT <num> (0 a 9999)\r\n");
                    }
                }
                else if (strcmp(tokens[0], "IOMAP") == 0) {
                    if(iomap_cmd(tokens, token_cont) != 0){
                        USART_putString("Uso: IOMAP <in 0-7> <out 0-7> [not] | IOMAP all [not]\r\n");
                    }
                }
                else if (strcmp(tokens[0], "IOUNMAP") == 0) {
                    iomap_clear();
                }
                else if (strcmp(tokens[0], "POT2SEG") == 0) {
                    if(pot2seg_cmd(tokens, token_cont) != 0){
                        USART_putString("Uso: POT2SEG {on|off} [{dec|hex}]\r\n");
                    }
                }
                else if (strcmp(tokens[0], "LCDWATCH") == 0) {
                    int r = -1;
                    if(token_cont >= 2 && (strcmp(tokens[1], "off") == 0 || strcmp(tokens[1], "OFF") == 0 || data_isValid(tokens[1], "addr"))){
                        r = lcdwatch_cmd(tokens, token_cont);
                    }
                    if(r == -2){
                        USART_putString("Direccion no permitida o no alineada (RAM, flash o perifericos)\r\n");
                    }else if(r != 0){
                        USART_putString("Uso: LCDWATCH <0xADDR> [dec|hex] [b|hw|w] | LCDWATCH off\r\n");
                    }else{
                        arcade_stop();
                    }
                }
                else if (strcmp(tokens[0], "ARCADE") == 0) {
                    if(arcade_cmd(tokens, token_cont) != 0){
                        USART_putString("Uso: ARCADE breakout | ARCADE off\r\n");
                    }
                }else if (strcmp(tokens[0], "HELP") == 0) {
                    USART_putString("\r\n--- COMANDOS DISPONIBLES ---\r\n");
                    USART_putString("RD                        - Muestra los registros (R0-R15)\r\n");
                    USART_putString("RM <Rx> <0xDATA>          - Modifica el registro Rx con el valor dado\r\n");
                    USART_putString("MD <0xSTART> <0xEND>      - Muestra el contenido de la memoria en ese rango\r\n");
                    USART_putString("MM <0xADDR> <0xDATA> [sz] - Modifica memoria (sz: 1, 2 o 4 bytes)\r\n");
                    USART_putString("BF <0xSTA> <0xEND> <DATA> [sz] - Llena un bloque de memoria con datos\r\n");
                    USART_putString("RUN <0xADDR>              - Ejecuta el programa saltando a la direccion\r\n");
                    USART_putString("CALL <0xADDR>             - Llama a una subrutina en la direccion\r\n");
                    USART_putString("SOUND <freq>              - Senal cuadrada de freq Hz al buzzer (0 = silencio)\r\n");
                    USART_putString("MUTE                      - Silencia el buzzer\r\n");
                    USART_putString("RGBOUT <RRGGBB>           - Color en el LED RGB (hex 24 bits)\r\n");
                    USART_putString("VOUT <volt> [res]         - Saca el voltaje por el DAC (A4), res 1-12 bits\r\n");
                    USART_putString("VIN [res]                 - Lee el voltaje del POT (A7), res 6/8/10/12 bits\r\n");
                    USART_putString("LCD <texto>               - Escribe el texto en la LCD (sin texto la borra)\r\n");
                    USART_putString("SEGMENT <num>             - Muestra num (0-9999) en el display de 7 segmentos\r\n");
                    USART_putString("IOMAP <in> <out> [not] [all] - Switch in (0-7) -> LED out (0-7), all = todos\r\n");
                    USART_putString("IOUNMAP                   - Quita todos los mapeos de IOMAP\r\n");
                    USART_putString("POT2SEG {on|off} [dec|hex] - Muestra el POT en el display en tiempo real\r\n");
                    USART_putString("LCDWATCH <0xADDR> [dec|hex] [b|hw|w] - Watch de memoria en la LCD (max 4)\r\n");
                    USART_putString("LCDWATCH off              - Quita todos los watches\r\n");
                    USART_putString("ARCADE breakout           - Juego en la LCD: barra con el POT, * o # lanza\r\n");
                    USART_putString("ARCADE off                - Sale del juego\r\n");
                    USART_putString("HELP                      - Muestra esta lista de ayuda\r\n");
                }
                else {
                    USART_putString("\n\rComando no valido\n\r");
                }
            }
			USART_putString("\n\r>> ");
			cmd_ready = 0;
        }
        tasks_update();
    }
}