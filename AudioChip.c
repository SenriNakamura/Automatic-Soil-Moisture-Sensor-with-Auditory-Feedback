#include "AudioChip.h"
#include "I2C.h"
#include <stm32l476xx.h>

void Audio_DAC_Init(void) {
		// Enable clock for GPIOE
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;

	// Configure PE3 as output
	GPIOE->MODER &= ~(3U << 6); // Clear bits
	GPIOE->MODER |= (1U << 6);  // Set as output

	// Set PE3 to high
	GPIOE->ODR |= (1U << 3);
	
  uint8_t Data_Send[6];
	Data_Send[0] = 0x00;
	Data_Send[1] = 0x99;
	I2C_SendData(AUDIO_I2C_ADDR, Data_Send, 2);
	volatile int j;
	for (j=0; j<400; j++){}
	
	Data_Send[0] = 0x47;
	Data_Send[1] = 0x80;
	uint8_t size_send = 2;
	I2C_SendData(AUDIO_I2C_ADDR, Data_Send, size_send);
	
	Data_Send[0] = 0x32;
	Data_Send[1] = 0xBB;
	I2C_SendData(AUDIO_I2C_ADDR, Data_Send, 2);
	
	Data_Send[0] = 0x32;
	Data_Send[1] = 0x3B;
	I2C_SendData(AUDIO_I2C_ADDR, Data_Send, 2);
	
	Data_Send[0] = 0x00;
	Data_Send[1] = 0x00;
	I2C_SendData(AUDIO_I2C_ADDR, Data_Send, 2);
}

void Set_Default_Audio_Registers(void) {
	set_DAC_register(0x00, 0x00);
	set_DAC_register(0x01, 0xE3);
	set_DAC_register(0x02, 0x9E);
	set_DAC_register(0x04, 0xAF);
	set_DAC_register(0x05, 0x81);
	set_DAC_register(0x06, 0x84); // original value 0x04
	set_DAC_register(0x07, 0x00);
	set_DAC_register(0x08, 0x81);
	set_DAC_register(0x09, 0x81);
	set_DAC_register(0x0A, 0x00);
	set_DAC_register(0x0C, 0x00);
	set_DAC_register(0x0D, 0x60);
	set_DAC_register(0x0E, 0x06);
	set_DAC_register(0x0F, 0x00);
	set_DAC_register(0x14, 0x00);
	set_DAC_register(0x15, 0x00);
	set_DAC_register(0x1A, 0x0A);
	set_DAC_register(0x1B, 0x0A);
	set_DAC_register(0x1C, 0x00);
	set_DAC_register(0x1D, 0x00);
	set_DAC_register(0x1E, 0x00);
	set_DAC_register(0x1F, 0x0F);
	set_DAC_register(0x20, 0xE5);
	set_DAC_register(0x21, 0xE5);
	set_DAC_register(0x22, 0x00);
	set_DAC_register(0x23, 0x00);
	set_DAC_register(0x24, 0x00);
	set_DAC_register(0x25, 0x00);
	set_DAC_register(0x26, 0x00);
	set_DAC_register(0x27, 0x00);
	set_DAC_register(0x28, 0x7F);
	set_DAC_register(0x29, 0xC0);
	set_DAC_register(0x2E, 0x00);
	set_DAC_register(0x2F, 0x00);
	set_DAC_register(0x30, 0x00);
	set_DAC_register(0x31, 0x00);
	set_DAC_register(0x34, 0x5F);
}

void SAI1_Init() {

    // Configure SAI1 MCLK output
    SAI1_Block_A->CR1 &= ~SAI_xCR1_NODIV;
    SAI1_Block_A->CR1 |= (SAI_xCR1_MCKDIV_0 >> 4); // Set MCLK division factor to 2

    // Enable SAI1 MCLK output
    SAI1_Block_A->CR1 |= 0x00010000;
}

void SAI1_GPIO_Init() {
    // Enable GPIOE clock
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;

    // Configure PE2 as alternate function for SAI1_MCLK_A
    GPIOE->MODER &= ~(3UL << (2 * 2));
    GPIOE->MODER |= (2UL << (2 * 2)); // Alternate function mode

    GPIOE->OTYPER &= ~(1UL << 2); // Push-pull output

    GPIOE->OSPEEDR |= (3UL << (2 * 2)); // High-speed output

    GPIOE->PUPDR &= ~(3UL << (2 * 2)); // No pull-up, pull-down

    // Configure PE2 alternate function to SAI1_MCLK_A (AF13)
    GPIOE->AFR[0] &= ~(0xFUL << (2 * 4));
    GPIOE->AFR[0] |= (13UL << (2 * 4));
}

// set a value to a register on the Audio Chip
void set_DAC_register(uint8_t DACRegister, uint8_t RegisterValue)
{
	uint8_t Data_Send[2];
	Data_Send[0] = DACRegister;
	Data_Send[1] = RegisterValue;
	I2C_SendData(AUDIO_I2C_ADDR, Data_Send, 2);
}

// read a register value on the Audio Chip
uint8_t read_DAC_register(uint8_t DACRegister, uint8_t *pData)
{
	uint8_t Data_Write[1];
	Data_Write[0] = DACRegister;
	I2C_SendData(AUDIO_I2C_ADDR, Data_Write, 1);
	
	I2C_ReceiveData(AUDIO_I2C_ADDR, pData, 1);
	return *pData;
}

void generate_beep(uint8_t freq_onTime, uint8_t offTime_volume, uint8_t occurance) {
	set_DAC_register(CS43L22_REG_BEEP_FREQ_ON_TIME, freq_onTime); // 1010 (freq) 0000 (on time)
	set_DAC_register(CS43L22_REG_BEEP_VOL_OFF_TIME, offTime_volume);  // 000 (off time) 11100 (beep volume)
	set_DAC_register(CS43L22_REG_BEEP_TONE_CFG, occurance); // 11 (beep occurance) 0 (beepmixdis) 0000 (bass/treble)
}

void my_audio_low(void) {
	//pitch: A6, On Time: ~430 ms, 
    //Beep Off Time: ~1.23s, Volume -6 dB
    //Occurence: Multiple, Mix disabled, corner freq: 5kHz, Bass 50Hz, Tone disabled 
	generate_beep(0b11010001,0b00000000,0b10000000);
}

void my_audio_mid(void) {
    //pitch: A6, On Time: ~430 ms,
    //Beep Off Time: ~1.23s, Volume -56 dB
    //Occurence: Multiple, Mix disabled, corner freq: 5kHz, Bass 50Hz, Tone disabled 
	generate_beep(0b11010001,0b00000111,0b10000000);
}

void my_audio_high(void) {
    //pitch: C7, On Time: ~430 ms,
    //Beep Off Time: ~1.23s, Volume -6 dB
    //Occurence: Multiple, Mix disabled, corner freq: 5kHz, Bass 50Hz, Tone disabled 
	generate_beep(0b11110001,0b00000000,0b10000000);
}
