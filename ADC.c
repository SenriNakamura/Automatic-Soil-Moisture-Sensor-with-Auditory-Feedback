#include "stm32l476xx.h"
#include <stdint.h>
#include "SysClock.h"
#include "ADC.h"


void ADC1_wakeup(){
	volatile int wait_time;
	
	//DEEPPWD = 0: ADC not in the deep-power-down mode
	//DEEPPWD = 1: ADC in the deep-power-down mode (default reset state)
	if ((ADC1->CR & ADC_CR_DEEPPWD) == ADC_CR_DEEPPWD)
		ADC1->CR &= ~ADC_CR_DEEPPWD;
	
	//Enable the ADC internal voltage regulator before any operation
	ADC1->CR |= ADC_CR_ADVREGEN;
	
	wait_time = 20* (80000000 / 1000000);
	while(wait_time != 0) {
		wait_time--;
	}
}


//setting high speed clock to 80MHz
void ADC_Common_Configuration(){
	SYSCFG->CFGR1 |= SYSCFG_CFGR1_BOOSTEN; //I/O analog switches voltage booster. The I/O analog switches resistance increases when the VDDA voltageb is too low.
	//V_REFINT enable
	ADC123_COMMON->CCR |= ADC_CCR_VREFEN;
	
	//ADC Clock Source: System Clock, PLLSAI1
	//Max ADC Clock: 80MHz
	
	//ADC presclaer to select the frequency of the clock to the ADC
	ADC123_COMMON->CCR &= ~ADC_CCR_PRESC; //0000: input ADC clock not divided
	
	//ADC clock mode
	// 00: CK_ADCx (x=123) (Asynchronous clock mode)
	// 01: HCLK/1 (Synchronous clock mode)
	// 10: HCLK/2 (Synch)
	// 11: HCLK/4 (synch)
	ADC123_COMMON->CCR &= ~ADC_CCR_CKMODE; //HCLK = 80MHz
	ADC123_COMMON->CCR |= ADC_CCR_CKMODE_0; 
	
	ADC123_COMMON->CCR &= ~ADC_CCR_DUAL;
	
	
}


//ADC Pin initilization
//PA1 (ADC12_IN6)
void ADC_Pin_Init(){
	//Enable the clock of GPIO Port A
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	
	//GPIO Pin Initialization
	//GPIO speed: Low speed(00), Medium speed (01), Fast (10), High (11)
	//Output type: pushpulll (0, reset), open drain (1)
	//GPIO Mode:  Input(00), Output(01), AlterFunc(10), Analog(11, reset)
	//Configure PA1(ADC12_IN6) as analog
	GPIOA->MODER |= 3U<<(2*1) | 3U<<(2*2); //mode 11 = analog
	
	//GPIO push pull
	GPIOA->PUPDR &= ~(3U<<(2*1) | 3U<<(2*2)); //No pull-up, no pull down
	
	//GPIO port analoog siwtch control register (ASCR)
	//0;disconnect analog switch to the ADC input
	//1:connect analog switch to the ADC input
	GPIOA->ASCR |= GPIO_ASCR_EN_1;
}

//initialize ADC
void ADC_Init(){
	//initialize timer 4 for ADC triggering
	TIM4_Init();
	
	//Enable the clock of ADC
	RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN; //enable ADC clock
	RCC->AHB2RSTR |= RCC_AHB2RSTR_ADCRST; //Asser ADC reset
	(void)RCC->AHB2RSTR; //short delay
	RCC->AHB2RSTR &= ~RCC_AHB2RSTR_ADCRST; //clear ADC reset
	
	ADC_Pin_Init();
	ADC_Common_Configuration();
	ADC1_wakeup();
	
	//Configure resolution and alignment
	ADC1->CFGR &= ~ADC_CFGR_RES; //Resolution: 12-bit
	ADC1->CFGR |= ADC_CFGR_CONT; // Continuous conversion mode
	ADC1->CFGR &= ~ADC_CFGR_ALIGN; //Data Alignment: Right
	
	//Configure sequence length and channel
	ADC1->SQR1 &= ~ADC_SQR1_L; //1 conversion in the sequence
	ADC1->SQR1 &= ~ADC_SQR1_SQ1; 
	ADC1->SQR1 |= (6U << 6); // PA1; ADC12_IN6
	ADC1->DIFSEL &= ~ADC_DIFSEL_DIFSEL_6; //subgke-ended for PA1:
	
	//Configure sample time
	ADC1->SMPR1 &= ~ADC_SMPR1_SMP6;
	ADC1->SMPR1 |= 3U << 18; // sample time: 24.5 ADC clock cycles
	
	 //clear continuous mode
  ADC1->CFGR &= ~ADC_CFGR_EXTEN;
	//Enable ADC
	ADC1->CR |= ADC_CR_ADEN;
	while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);
	
}	
//assuming sampling rate is 10kHz, the timer 4 is set to generate 10,000 outputs to trigger ADC.
void TIM4_Init(){
	

	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM4EN; //Enable clock of Timer 4
	TIM4->CR1 &= ~TIM_CR1_CMS; //clear edge-aligned mode bits
	TIM4->CR1 &= ~TIM_CR1_DIR; // Counting direction: up counting
	TIM4->CR2 &= ~TIM_CR2_MMS; //clear master mode selection bits
	TIM4->CR2 |= TIM_CR2_MMS2; //Select 100 = OC1REF as TRGO
	
	//OC1M: Output Compare 1 mode
	TIM4->CCMR1 &= ~TIM_CCMR1_OC1M; //clear mode bits
	TIM4->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2; //0110 PWM mode 1
	
	//Timer driving frequency = 80 MHz/(1 + PSC) = 80 MHz/(1+7) = 10 MHz
	//Trigger frequency = 10 MHz / (1 + ARR) = 10MHz/1000 = 10KHz
	
	TIM4->PSC = 7; //max 65535
	TIM4->ARR = 999; //max 65535
	TIM4->CCR1 = 500; //Duty ration 50%
	
	TIM4->CCER |= TIM_CCER_CC1E; //OC1 signal is output
	TIM4->CR1 |= TIM_CR1_CEN; //Enable timer
}

void ADC_IRQHandler(){
		NVIC_ClearPendingIRQ(ADC1_2_IRQn);
	
    if ((ADC1->ISR & ADC_ISR_EOC) == ADC_ISR_EOC) {
        ADC1->ISR |= ADC_ISR_EOC; // Clear the End of Conversion flag
    }
    
    if ((ADC1->ISR & ADC_ISR_EOS) == ADC_ISR_EOS) {
        ADC1->ISR |= ADC_ISR_EOS; // Clear the End of Sequence flag
    }
}
