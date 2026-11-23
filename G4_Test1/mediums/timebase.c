#include "RTE_Components.h"
#include CMSIS_device_header
#include "timebase.h"

volatile uint32_t ms_ticks = 0;

// SysTick cada 1 ms
void timebase_config(void){
    SysTick_Config(SystemCoreClock / 1000);
}

void SysTick_Handler(void){
    ms_ticks++;
}

void delay_ms(uint32_t ms){
    uint32_t inicio = ms_ticks;
    while(ms_ticks - inicio < ms){}
}
