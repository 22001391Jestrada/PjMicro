#include "RTE_Components.h"
#include CMSIS_device_header
#include "dac.h"

#define VREF 3.3f

// DAC1 canal 2 en A4 = PA5
void dac_config(void){
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_DAC1EN;

    GPIOA->MODER |= (3U << (5*2));

    // modo normal, buffer encendido, salida al pin
    DAC1->CR &= ~DAC_CR_EN2;
    DAC1->MCR &= ~(DAC_MCR_MODE2 | DAC_MCR_HFSEL);
    DAC1->DHR12R2 = 0;
    DAC1->CR |= DAC_CR_EN2;
    while(!(DAC1->SR & DAC_SR_DAC2RDY)){}
}

void vout(float voltage, uint32_t res){
    uint32_t max = (1U << res) - 1;
    uint32_t code = (uint32_t)(voltage / VREF * max + 0.5f);
    if(code > max){
        code = max;
    }
    // el codigo de res bits se pasa a 12 bits
    DAC1->DHR12R2 = code << (12 - res);
}
