#include "RTE_Components.h"
#include CMSIS_device_header
#include "keypad.h"

// Solo se usa la fila 4 del teclado (* 0 #):
// fila 4 = KEY4 = A1 = PA1 (salida en 0)
// *  = columna 1 = KEY3 = D3 = PB0 (entrada con pull-up)
// #  = columna 3 = KEY5 = D8 = PF1 (entrada con pull-up)
void keypad_config(void){
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOFEN;

    GPIOA->BSRR = (1U << (1+16));
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (1*2))) | (1U << (1*2));

    GPIOB->PUPDR = (GPIOB->PUPDR & ~(3U << (0*2))) | (1U << (0*2));
    GPIOB->MODER &= ~(3U << (0*2));

    GPIOF->PUPDR = (GPIOF->PUPDR & ~(3U << (1*2))) | (1U << (1*2));
    GPIOF->MODER &= ~(3U << (1*2));
}

// 1 si * o # estan presionados (la columna se lee en 0)
int keypad_star_hash(void){
    return !(GPIOB->IDR & (1U << 0)) || !(GPIOF->IDR & (1U << 1));
}
