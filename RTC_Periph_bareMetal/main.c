/*

	This program configures the real time clock peripheral on the STM32F4.
	It also uses the Alarm feature to generate an interrupt.
	The time is set to 23:45, and an alarm occurs at 23:46.
	When the alarm occurs, the green LED (GPIO pin 12 of GPIOD) comes on.
*/



#include "stm32f4xx.h"                  // Device header

typedef struct
{
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	
}RTC_t;

static uint8_t hexToBCD(uint8_t hex)
{
  uint8_t bcd;
  uint8_t multiplieOfTen = 0;

  while(hex > 10)
  {
      hex -= 10;
      multiplieOfTen++;
  }
  bcd = (multiplieOfTen << 4) | hex;
  return bcd;
}

static uint8_t BCDToHex(uint8_t bcd)
{
	/*
	Description:
	Converts BCD to hexadecimal
	
	Parameters:
	1.) bcd: this parameter is passed with a value to be converted
	to hexadecimal.
	
	Return:
	1.) Hexadecimal equivalent of the argument passed to 'bcd'.
	*/
	uint8_t hex;
	
	hex = (((bcd & 0xF0)>>4)*10) + (bcd&0x0F);
	return hex;
}

static void Clocks_Init(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	PWR->CR |= PWR_CR_DBP;
	RCC->CSR |= RCC_CSR_LSION;
	while((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY);
	RCC->BDCR |= RCC_BDCR_RTCSEL_1 | RCC_BDCR_RTCEN; //LSI clock is used as RTC clock source	
}

static void GPIOD_Init(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
	GPIOD->MODER |= GPIO_MODER_MODE12_0;
}

static void RTC_Init(void)
{
	//RTC write operation enable
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;

	//Interrupt config
	
	EXTI->IMR |= EXTI_IMR_IM17;
	EXTI->RTSR |= EXTI_RTSR_TR17;
	NVIC_EnableIRQ(RTC_Alarm_IRQn);
}

static void RTC_Alarm_Init(uint8_t hour, uint8_t min)
{
	uint8_t timeBCD[3] = {0,0,0};
	
	RTC->CR &= ~RTC_CR_ALRAE; //Disable Alarm A
	while((RTC->ISR & RTC_ISR_ALRAWF) != RTC_ISR_ALRAWF); 
	timeBCD[1] = hexToBCD(min);
	timeBCD[2] = hexToBCD(hour);
	RTC->ALRMAR = (timeBCD[1] << 8) | (timeBCD[2] << 16) | RTC_ALRMAR_MSK4; //Mask the day
	
	RTC->CR |= RTC_CR_ALRAIE | RTC_CR_ALRAE;
	
	
}

static void RTC_SetTime(uint8_t hour, uint8_t min)
{
	uint8_t timeBCD[3] = {0,0,0}; //sec, min, hour
	//Calender init
	RTC->ISR |= RTC_ISR_INIT;
	while((RTC->ISR & RTC_ISR_INITF) != RTC_ISR_INITF);
	timeBCD[1] = hexToBCD(min);
	timeBCD[2] = hexToBCD(hour);
	RTC->TR |= (timeBCD[1] << 8) | (timeBCD[2] << 16);
	RTC->ISR &= ~RTC_ISR_INIT;
}

static void RTC_getTime(RTC_t* pTime)
{
	uint32_t read_RTC_TR;
	while((RTC->ISR & RTC_ISR_RSF) != RTC_ISR_RSF);
	read_RTC_TR = RTC->TR;
	pTime->seconds = BCDToHex(read_RTC_TR & 0x7F);
	pTime->minutes = BCDToHex((read_RTC_TR & (0x7F<<8)) >> 8);
	pTime->hours = BCDToHex((read_RTC_TR & (0x7F << 16)) >> 16);
	
}

RTC_t time;

int main(void)
{
	Clocks_Init();
	RTC_Init();
	GPIOD_Init();
	RTC_SetTime(23,45);
	RTC_Alarm_Init(23,46);
	
	
	while(1)
	{
		RTC_getTime(&time);
	}
	
}

void RTC_Alarm_IRQHandler(void)
{
	EXTI->PR |= EXTI_PR_PR17;
	GPIOD->ODR |= GPIO_ODR_OD12;
	RTC->ISR &= ~RTC_ISR_ALRAF;
}
