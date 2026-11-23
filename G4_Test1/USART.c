#include "USART.h"

char rx_data[64];
char rx_line[64];    // copia de la linea completa (strtok parte rx_data)
uint8_t n;
uint8_t cmd_ready = 0;
char uart_data;
uint8_t i = 0;

// USART1: PA9 = TX (D1), PA10 = RX (D0) -> FTDI
void USART_config(uint32_t baudrate){
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    // PA9 y PA10 en modo alterno
    GPIOA->MODER &= ~(GPIO_MODER_MODE9 | GPIO_MODER_MODE10);
    GPIOA->MODER |=  (GPIO_MODER_MODE9_1 | GPIO_MODER_MODE10_1);

    // AF7 = USART1
    GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL9 | GPIO_AFRH_AFSEL10);
    GPIOA->AFR[1] |=  (7U << GPIO_AFRH_AFSEL9_Pos) | (7U << GPIO_AFRH_AFSEL10_Pos);

    // Pull-up en RX para que no flote si el FTDI esta desconectado
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD10;
    GPIOA->PUPDR |=  GPIO_PUPDR_PUPD10_0;

    USART1->BRR = (uint16_t)(SystemCoreClock / baudrate);
    USART1->CR1 |= USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE;
    USART1->CR1 |= USART_CR1_UE;

    NVIC_EnableIRQ(USART1_IRQn);
}

void USART_send(char c){
    while(!(USART1->ISR & USART_ISR_TXE)){}
    USART1->TDR = c;
}

void USART_putString(char * string){
    while(*string){
        USART_send(*string);
        string++;
    }
}

void USART1_IRQHandler(void){
    if(USART1->ISR & USART_ISR_ORE){
        USART1->ICR |= USART_ICR_ORECF;
    }
    if(USART1->ISR & USART_ISR_RXNE){
        uart_data = USART1->RDR;
        if(uart_data == '\r'){
            rx_data[i] = '\0';
            for(uint8_t k = 0; k <= i; k++){
                rx_line[k] = rx_data[k];
            }
            cmd_ready = 1;
            i = 0;
        }else if(uart_data == '\n'){
            // CoolTerm manda CR+LF, el LF se ignora
        }else if(uart_data == '\b'){
            if(i >= 1){
                i--;
                rx_data[i] = '\0';
            }
        }else if(i < 63){
            rx_data[i] = uart_data;
            i++;
        }
    }
}
