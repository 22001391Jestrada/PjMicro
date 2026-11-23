#include "RTE_Components.h"
#include CMSIS_device_header
#include "buzzer.h"

// Buzzer en D12 = PB4, TIM16_CH1 (AF1)
void buzzer_config(void){
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM16EN;

    GPIOB->MODER &= ~(3U << (4*2));
    GPIOB->MODER |=  (2U << (4*2));
    GPIOB->AFR[0] &= ~(0xFU << (4*4));
    GPIOB->AFR[0] |=  (1U << (4*4));

    // salida forzada en bajo mientras no suena
    TIM16->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM16->CCMR1 |=  (4U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    TIM16->CCER  |= TIM_CCER_CC1E;
    TIM16->BDTR  |= TIM_BDTR_MOE;
    TIM16->CR1   |= TIM_CR1_ARPE;
}

void sound(uint32_t freq){
    if(freq == 0){
        TIM16->CR1 &= ~TIM_CR1_CEN;
        TIM16->CCMR1 &= ~TIM_CCMR1_OC1M;
        TIM16->CCMR1 |=  (4U << TIM_CCMR1_OC1M_Pos);
        return;
    }
    uint32_t ticks = SystemCoreClock / freq;
    uint32_t psc = ticks / 65536;
    TIM16->PSC  = psc;
    TIM16->ARR  = ticks / (psc + 1) - 1;
    TIM16->CCR1 = (TIM16->ARR + 1) / 2;

    // PWM modo 1, 50%
    TIM16->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM16->CCMR1 |=  (6U << TIM_CCMR1_OC1M_Pos);
    TIM16->EGR = TIM_EGR_UG;
    TIM16->CR1 |= TIM_CR1_CEN;
}
