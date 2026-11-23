#include "RTE_Components.h"
#include CMSIS_device_header
#include "adc.h"

// POT en A7 = PA2 = ADC1_IN3
void adc_config(void){
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN | RCC_AHB2ENR_GPIOAEN;
    // reloj sincrono HCLK/1
    ADC12_COMMON->CCR &= ~ADC_CCR_CKMODE;
    ADC12_COMMON->CCR |=  (1U << ADC_CCR_CKMODE_Pos);

    GPIOA->MODER |= (3U << (2*2));

    // salir de deep power down y encender el regulador (>= 20 us)
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |= ADC_CR_ADVREGEN;
    for(volatile uint32_t k = 0; k < 1000; k++){}

    // calibracion single-ended
    ADC1->CR &= ~ADC_CR_ADCALDIF;
    ADC1->CR |= ADC_CR_ADCAL;
    while(ADC1->CR & ADC_CR_ADCAL){}

    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while(!(ADC1->ISR & ADC_ISR_ADRDY)){}

    // una conversion, canal 3, muestreo largo
    ADC1->SQR1 = (3U << ADC_SQR1_SQ1_Pos);
    ADC1->SMPR1 &= ~ADC_SMPR1_SMP3;
    ADC1->SMPR1 |=  (7U << ADC_SMPR1_SMP3_Pos);
}

// res: 12, 10, 8 o 6 bits
uint32_t adc_read(uint32_t res){
    uint32_t bits = (12 - res) / 2;
    ADC1->CFGR &= ~ADC_CFGR_RES;
    ADC1->CFGR |=  (bits << ADC_CFGR_RES_Pos);

    ADC1->CR |= ADC_CR_ADSTART;
    while(!(ADC1->ISR & ADC_ISR_EOC)){}
    return ADC1->DR;
}
