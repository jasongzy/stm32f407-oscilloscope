#include "dac.h"
#include "sys.h"
#include "math.h"
#include "waveform.h"
#define PI  3.14159
#define DAC_DHR12R1_ADDRESS     0x40007408

u16 sinTable[tableSize];

void sin_Generation(void)
{
	u16 n;
	for(n=0;n<tableSize;n++)
	{
		sinTable[n] = (sin(2*PI*n/tableSize)+1)*2047;	
	}	
}

void TIM6_Configuration(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);	

 	TIM_PrescalerConfig(TIM6, 83, TIM_PSCReloadMode_Update);
  	TIM_SetAutoreload(TIM6, 4);
  	TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);
	TIM_Cmd(TIM6, ENABLE);
}

void DacGPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

  	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void DAC_DMA_Configuration(void)
{
	DAC_InitTypeDef  DAC_InitStructure;	

	DMA_InitTypeDef DMA_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

	DAC_InitStructure.DAC_Trigger = DAC_Trigger_T6_TRGO;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
	DAC_Init(DAC_Channel_1, &DAC_InitStructure);

	DMA_DeInit(DMA1_Stream5);
	DMA_InitStructure.DMA_Channel = DMA_Channel_7;  
	DMA_InitStructure.DMA_PeripheralBaseAddr = DAC_DHR12R1_ADDRESS;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&sinTable;
	DMA_InitStructure.DMA_BufferSize = tableSize;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA1_Stream5, &DMA_InitStructure);    
	
	DMA_Cmd(DMA1_Stream5, ENABLE);
	
	DAC_Cmd(DAC_Channel_1, ENABLE);
	
	DAC_DMACmd(DAC_Channel_1, ENABLE);
} 

void MYDAC_Init(void)
{
	sin_Generation();	
	DacGPIO_Configuration();
	TIM6_Configuration();
	
	DAC_DMA_Configuration();   
}

