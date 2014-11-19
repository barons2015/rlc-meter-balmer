// balmer@inbox.ru RLC Meter 303
// 2013-2014

#include "hw_config.h"
#include <math.h>
//#include "SysTick/systick.h"
#include "dac.h"
#include "adc.h"

#define DAC_ZERO 2047
#define DAC_AMPLITUDE 1200

//max 375 khz
#define MIN_SINUS_PERIOD 192

static uint16_t g_dac_amplitude = DAC_AMPLITUDE;
static uint16_t g_sinusBuffer[SINUS_BUFFER_SIZE];
static uint32_t SinusBufferSize = SINUS_BUFFER_SIZE;
static uint32_t g_dac_period = 0; // * 1/SystemCoreClock sec SystemCoreClock==72000000
float g_sinusBufferFloat[SINUS_BUFFER_SIZE];

uint32_t DacPeriod(void)
{
	return g_dac_period;
}

float DacFrequency()
{
	if(g_dac_period==0)
		return 1.0f;
	return SystemCoreClock/(float)g_dac_period;
}

uint32_t DacSamplesPerPeriod(void)
{
	return SinusBufferSize;
}

uint32_t DacSampleTicks(void)
{
	return g_dac_period/SinusBufferSize;
}
/*
static void DacSquareCalculate()
{
	for(int i=0; i<SinusBufferSize; i++)
	{
		float s;
		if(i<(SinusBufferSize/2))
			s = -1;
		else
			s = +1;
		g_sinusBufferFloat[i] = s;
		g_sinusBuffer[i] = (uint16_t) lround(s*g_dac_amplitude)+DAC_ZERO;
	}
}
*/
void DacSinusCalculate()
{
	float mul = 2*pi/SinusBufferSize;
	for(int i=0; i<SinusBufferSize; i++)
	{
		float s = sin(i*mul);
		g_sinusBufferFloat[i] = s;
		g_sinusBuffer[i] = (uint16_t) lround(s*g_dac_amplitude)+DAC_ZERO;
	}
}

void DacInit(void)
{  
	DacSinusCalculate();
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);	

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

  	DAC_InitTypeDef DAC_InitStructure;
  	DAC_StructInit(&DAC_InitStructure);
	DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
	
	DAC_Init(DAC_Channel_1, &DAC_InitStructure);
	//delay_us(100);
	DAC_Cmd(DAC_Channel_1, ENABLE);
	DAC_SetChannel1Data(DAC_Align_12b_R, DAC_ZERO);

}

/*
If frequency<=1 khz
	SinusBufferSize maximal
	TIM_Period = SystemCoreClock / SINUS_BUFFER_SIZE / frequency
*/
void DacSetFrequency(uint32_t frequency)
{
	DacSetPeriod(SystemCoreClock/frequency, DAC_AMPLITUDE);
}

/*
	sinusPeriod in SystemCoreClock quants
*/
void DacSetPeriod(uint32_t sinusPeriod, uint16_t amplitude)
{
	g_dac_amplitude = amplitude;
	if(sinusPeriod<MIN_SINUS_PERIOD)
		sinusPeriod = MIN_SINUS_PERIOD;
	//assert_param(frequency>=100 && frequency<=200000);
	DMA_Cmd(DMA1_Channel2, DISABLE);
	TIM_Cmd(TIM2, DISABLE);

	DAC_SetChannel1Data(DAC_Align_12b_R, DAC_ZERO);

	uint32_t prescaler;
	uint32_t period;
	prescaler = 1;
	period = 24;

	//Нужно чтобы SinusBufferSize был кратен 4
	sinusPeriod = (sinusPeriod/(period*4))*(period*4);

	//Пусть будет не кратным 4, попробуем частоту 100 КГц
	//sinusPeriod = (sinusPeriod/period)*period;

	SinusBufferSize = sinusPeriod/period;

	if(SinusBufferSize>SINUS_BUFFER_SIZE)
	{
		//period = 72;
		prescaler = sinusPeriod/period/SINUS_BUFFER_SIZE;
		while(SINUS_BUFFER_SIZE*prescaler*period<sinusPeriod)
		{
			prescaler++;
		}

		uint32_t p4 = period*prescaler*4;
		sinusPeriod = (sinusPeriod/p4)*p4;

		SinusBufferSize = sinusPeriod/period/prescaler;
	}

	DacSinusCalculate();

	DMA_InitTypeDef DMA_InitStructure;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&DAC->DHR12R1;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&g_sinusBuffer;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = SinusBufferSize;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel2, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel2, ENABLE);


	//72 MHz / TIM_Prescaler / TIM_Period
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Period = period-1;
	TIM_TimeBaseStructure.TIM_Prescaler = prescaler-1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_SetCounter(TIM2, 0);

	TIM_DMACmd(TIM2, TIM_DMA_Update, ENABLE);

#ifdef USE_ADC12
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 1;
	TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_Low;
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);//ADC1 EXT3
#else
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 1;
	TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_Low;
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);//ADC3 EXT1
#endif

	g_dac_period = period * prescaler * SinusBufferSize;
}

void DacStart()
{
	TIM_Cmd(TIM2, ENABLE);
}