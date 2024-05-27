#include "I2C.h"
#include <stm32l476xx.h>

// I2C1_SCL is pin PB6
// I2C1_SDA is pin PB7
// Need to enable these GPIO pins to use I2C
void I2C_GPIO_Init(){
	
    //Enable GPIOB clock

		RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

    // Configuring PB6 and PB7 in Alternate function
		GPIOB->MODER &= ~((3UL<<(6*2)) | (3UL<<(7*2)));
    GPIOB->MODER |= ((2UL<<(6*2)) | (2UL<<(7*2)));

    // Selecting PB6 and PB7 as Pull up pins
		GPIOB->PUPDR &= ~((3UL<<(6*2)) | (3UL<<(7*2)) );
    GPIOB->PUPDR |= ( (1UL<<(6*2)) | (1UL<<(7*2)));

    // Setting PB6 and PB7 as open drain
    GPIOB->OTYPER |= ( (1UL<<6) | (1UL<<7) );

    // Setting PB6 and PB7 at high speed
		GPIOB->OSPEEDR &= ~((3UL<<(6*2)) | (3UL<<(7*2)) );
    GPIOB->OSPEEDR |= ( (2UL<<(6*2)) | (2UL<<(7*2)) );

    // Selecting the Alternate function (AF4)
		GPIOB->AFR[0] &= ~((15UL<<(6*4)) | (15UL<<(7*4)) );
    GPIOB->AFR[0] |= ( (4UL<<(6*4)) | (4UL)<<(7*4));
}

// Initializing I2C
void I2C_Init() {
	uint32_t OwnAddr = 0x52;
	
	// Enable the I2C clock and select SYSCLK as the clock source
	RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN; // I2C1 clock enable
	
	// 00 = PCLK, 01 = SYSCLK, 10 = HSI16, 11 = Reserved
	RCC->CCIPR &= ~RCC_CCIPR_I2C1SEL; // clear bits
	RCC->CCIPR |= RCC_CCIPR_I2C1SEL_0; // Select SYSCLK
	
	RCC->APB1RSTR1 |= RCC_APB1RSTR1_I2C1RST; // 1 = Reset I2C1
	RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_I2C1RST; // complete the reset
	
	// I2C CR1 Configuration
	// When the I2C is disabled (PE=0), the I2C performs a software reset
	I2C1->CR1 &= ~I2C_CR1_PE; // disable I2C
	I2C1->CR1 &= ~I2C_CR1_ANFOFF; // 0: analog noise filter enabled
	I2C1->CR1 &= ~I2C_CR1_DNF; // 0000: digital filter disabled
	I2C1->CR1 |= I2C_CR1_ERRIE; // errors interrupt enable
	
	I2C1->CR1 &= ~I2C_CR1_SMBHEN; // SMBus mode: 0 = I2C mode; 1 = SMBus mode
	
	I2C1->CR1 &= ~I2C_CR1_NOSTRETCH; // enable clock stretching
	
	// I2C TIMINGR Configuration
	I2C1->TIMINGR = 0;
	
	// SysTimer = 80MHz, PRESC=7, 80MHz/(1+7)=10MHz
	I2C1->TIMINGR &= ~I2C_TIMINGR_PRESC; // clear the prescaler
	I2C1->TIMINGR |= 7U << 28; // set clock prescaler to 7
	I2C1->TIMINGR |= 49U; // SCLL: SCL low period (master mode) > 4.7us
	I2C1->TIMINGR |= 49U << 8; // SCLH: SCL high period (master mode) > 4.0us
	I2C1->TIMINGR |= 14U << 20; // SCLDEL; Data setup time > 1.0us
	I2C1->TIMINGR |= 15U << 16; // SDADEL: Data hold time > 1.25us
	
	// I2C own address 1 register (I2C_OAR1)
	I2C1->OAR1 &= ~I2C_OAR1_OA1EN;
	I2C1->OAR1 = I2C_OAR1_OA1EN | OwnAddr; // 7 bit own address
	I2C1->OAR1 &= ~I2C_OAR2_OA2EN; // disable own address 2
	
	// I2C CR2 Configuration
	I2C1->CR2 &= ~I2C_CR2_ADD10; // 0 = 7-bit mode, 1 = 10-bit mode
	I2C1->CR2 |= I2C_CR2_AUTOEND; // enable the auto end
	I2C1->CR2 |= I2C_CR2_NACK; // for slave mode: set NACK
	I2C1->CR1 |= I2C_CR1_PE; // enable I2C1
}

// Sending the start bit
void I2C_Start(uint32_t DevAddress, uint8_t Size, uint8_t Direction) {
	// Direction = 0; Master requests a write transfer
	// Direction = 1; Master requests a read transfer
	uint32_t tmpreg = I2C1->CR2;
	tmpreg &= (uint32_t)~((uint32_t)(I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_STOP));
	if (Direction == 1)
		tmpreg |= I2C_CR2_RD_WRN; // read from slave
	else 
		tmpreg &= ~I2C_CR2_RD_WRN; // write to slave
	
	tmpreg |= (uint32_t)(((uint32_t) DevAddress & I2C_CR2_SADD) | (((uint32_t) Size << 16) & I2C_CR2_NBYTES));
	
	tmpreg |= I2C_CR2_START;
	I2C1->CR2 = tmpreg;
			
}

// Generating a stop bit
void I2C_Stop() {
	// Master: generate STOP bit after the current byte has been transferred
	I2C1->CR2 |= I2C_CR2_STOP;
	
	// Wait until STOPF flag is reset
	while((I2C1->ISR & I2C_ISR_STOPF) == 0);
	
	I2C1->ICR |= I2C_ICR_STOPCF; // write 1 to clear STOPF flag
}

// Waiting for the line idle
void I2C_WaitLineIdle() {
	// wait until I2C bus is ready
	while ((I2C1->ISR & I2C_ISR_BUSY) == I2C_ISR_BUSY); // if busy, wait
}

// receiving data via I2C
int8_t I2C_ReceiveData(uint8_t SlaveAddress, uint8_t *pData, uint8_t Size) {
	int i;
	if (Size <= 0 || pData == 0) return -1;
	
	I2C_WaitLineIdle();
	
	I2C_Start(SlaveAddress, Size, 1); // 1 = Receiving from the slave
	for (i=0; i < Size; i++) {
		// Wait until RXNE flag is set
		while ((I2C1->ISR & I2C_ISR_RXNE) == 0);
		pData[i] = I2C1->RXDR & I2C_RXDR_RXDATA;
	}
	
	while((I2C1->ISR & I2C_ISR_TC) == 0); // wait until TCR flag is set
	
	I2C_Stop();
	return 0;
}

// transmitting data via I2C
int8_t I2C_SendData(uint8_t SlaveAddress, uint8_t *pData, uint8_t Size) {
	volatile int i;
	if (Size <= 0 || pData == 0) return -1;
	
	// wiat until the line is idle
	I2C_WaitLineIdle();
	
	// the last argument: 0 = sending data to the slave
	I2C_Start(SlaveAddress, Size, 0);
	
	for (i=0; i<Size; i++) {
		// TXIS bit is set by hardware when the TXDR register is empty and the
		// data to be transmitted must be written in the TXDR register. It is
		// cleared when the next data to be sent is written in the TXDR register.
		// The TXIS flag is not set when a NACK is received.
		
		while ((I2C1->ISR & I2C_ISR_TXIS) == 0);
		
		// TXIS is cleared by writing to the =TXDR register
		I2C1->TXDR = pData[i] & I2C_TXDR_TXDATA;
	}
	
	// wait until TC flag is set
	while ((I2C1->ISR & I2C_ISR_TC) == 0 && (I2C1->ISR & I2C_ISR_NACKF) == 0);
	
	if ((I2C1->ISR & I2C_ISR_NACKF) != 0)
		return -1;
	
	I2C_Stop();
	
	return 0;
}