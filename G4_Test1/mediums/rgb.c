#include "RTE_Components.h"
#include CMSIS_device_header
#include <ctype.h>
#include "rgb.h"

// R = D9 = PA8 (TIM1_CH1 AF6), G = D10 = PA11 (TIM1_CH4 AF11), B = D11 = PB5 (TIM3_CH2 AF2)
// estos pines los comparte la LCD (jumpers J2/J3/J4), por eso se reconfiguran en cada rgb_set
static void rgb_pins(void){
    GPIOA->MODER &= ~((3U << (8*2)) | (3U << (11*2)));
    GPIOA->MODER |=  (2U << (8*2)) | (2U << (11*2));
    GPIOA->AFR[1] &= ~((0xFU << ((8-8)*4)) | (0xFU << ((11-8)*4)));
    GPIOA->AFR[1] |=  (6U << ((8-8)*4)) | (11U << ((11-8)*4));

    GPIOB->MODER &= ~(3U << (5*2));
    GPIOB->MODER |=  (2U << (5*2));
    GPIOB->AFR[0] &= ~(0xFU << (5*4));
    GPIOB->AFR[0] |=  (2U << (5*4));
}

void rgb_config(void){
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;
    RCC->APB2ENR  |= RCC_APB2ENR_TIM1EN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;

    rgb_pins();

    // 1 MHz, periodo de 255 cuentas: CCR = 0 apagado, CCR = 255 siempre encendido
    TIM1->PSC = 16 - 1;
    TIM1->ARR = 254;
    TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM1->CCMR1 |=  (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    TIM1->CCMR2 &= ~TIM_CCMR2_OC4M;
    TIM1->CCMR2 |=  (6U << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE;
    TIM1->CCR1 = 0;
    TIM1->CCR4 = 0;
    TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC4E;
    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM1->EGR = TIM_EGR_UG;
    TIM1->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;

    TIM3->PSC = 16 - 1;
    TIM3->ARR = 254;
    TIM3->CCMR1 &= ~TIM_CCMR1_OC2M;
    TIM3->CCMR1 |=  (6U << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE;
    TIM3->CCR2 = 0;
    TIM3->CCER |= TIM_CCER_CC2E;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;
}

void rgb_set(uint32_t color){
    rgb_pins();
    TIM1->CCR1 = (color >> 16) & 0xFF;
    TIM1->CCR4 = (color >> 8) & 0xFF;
    TIM3->CCR2 = color & 0xFF;
}

// acepta RRGGBB o 0xRRGGBB, hasta 6 digitos hex
int rgb_isValid(char *str){
    int i = 0;
    if(str[0] == '0' && (str[1] == 'x' || str[1] == 'X')){
        i = 2;
    }
    int digits = 0;
    while(str[i] != '\0'){
        if(!isxdigit((unsigned char)str[i])){
            return 0;
        }
        i++;
        digits++;
    }
    return (digits >= 1 && digits <= 6);
}
