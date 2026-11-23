#include "RTE_Components.h"
#include CMSIS_device_header
#include "i2c.h"

#define I2C_TIMEOUT 50000

// I2C1: SDA = D4 = PB7 (AF4), SCL = D5 = PA15 (AF4), open-drain, pull-ups en el circuito
void i2c_config(void){
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;

    GPIOB->OTYPER |= (1U << 7);
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(0xFU << (7*4))) | (4U << (7*4));
    GPIOB->MODER  = (GPIOB->MODER & ~(3U << (7*2))) | (2U << (7*2));

    GPIOA->OTYPER |= (1U << 15);
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(0xFU << ((15-8)*4))) | (4U << ((15-8)*4));
    GPIOA->MODER  = (GPIOA->MODER & ~(3U << (15*2))) | (2U << (15*2));

    // 100 kHz con 16 MHz (RM0440 tabla 384)
    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->TIMINGR = 0x30420F13;
    I2C1->CR1 |= I2C_CR1_PE;
}

// si algo se traba, apagar y prender el periferico lo resetea
static void i2c_reset(void){
    I2C1->CR1 &= ~I2C_CR1_PE;
    while(I2C1->CR1 & I2C_CR1_PE){}
    I2C1->CR1 |= I2C_CR1_PE;
}

// espera una bandera; si llega NACK o se acaba el tiempo regresa -1
static int i2c_wait(uint32_t flag){
    uint32_t t = I2C_TIMEOUT;
    while(!(I2C1->ISR & flag)){
        if(I2C1->ISR & I2C_ISR_NACKF){
            // con NACK el maestro manda STOP solo
            t = I2C_TIMEOUT;
            while(!(I2C1->ISR & I2C_ISR_STOPF) && --t){}
            I2C1->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
            return -1;
        }
        if(--t == 0){
            i2c_reset();
            return -1;
        }
    }
    return 0;
}

static int i2c_wait_free(void){
    uint32_t t = I2C_TIMEOUT;
    while(I2C1->ISR & I2C_ISR_BUSY){
        if(--t == 0){
            i2c_reset();
            return -1;
        }
    }
    return 0;
}

int i2c_write(uint8_t addr, uint8_t reg, uint8_t data){
    if(i2c_wait_free()) return -1;
    I2C1->CR2 = (addr << 1) | (2U << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_START;
    if(i2c_wait(I2C_ISR_TXIS)) return -1;
    I2C1->TXDR = reg;
    if(i2c_wait(I2C_ISR_TXIS)) return -1;
    I2C1->TXDR = data;
    if(i2c_wait(I2C_ISR_STOPF)) return -1;
    I2C1->ICR = I2C_ICR_STOPCF;
    return 0;
}

int i2c_read(uint8_t addr, uint8_t reg, uint8_t *data){
    if(i2c_wait_free()) return -1;
    // se escribe el registro sin STOP y luego se lee con START repetido
    I2C1->CR2 = (addr << 1) | (1U << I2C_CR2_NBYTES_Pos) | I2C_CR2_START;
    if(i2c_wait(I2C_ISR_TXIS)) return -1;
    I2C1->TXDR = reg;
    if(i2c_wait(I2C_ISR_TC)) return -1;
    I2C1->CR2 = (addr << 1) | I2C_CR2_RD_WRN | (1U << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_START;
    if(i2c_wait(I2C_ISR_RXNE)) return -1;
    *data = I2C1->RXDR;
    if(i2c_wait(I2C_ISR_STOPF)) return -1;
    I2C1->ICR = I2C_ICR_STOPCF;
    return 0;
}
