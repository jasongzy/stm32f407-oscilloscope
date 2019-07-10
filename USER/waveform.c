//STM32 使用DMA+DAC+TIMER输出正弦波//

/* 

          那么对于使用DMA+DAC+TIMER产生正弦波的原理或过程，我有这样一个简单的理解：
      先将一个可以生成正弦波的数据表保存在静态内存中，然后在DAC以及这块内存中间使
      用DMA建立一个通道，经过以上步骤之后，DAC模块就可以通过DMA通道拿取静态内存中
      可以生成正弦波的数据，拿取数据，然后经过数模准换，在引脚进行输出就可以得到正
      弦波了。那么当然，这个速度是非常快的，如果没有一定的延时，那么得到的估计就是
      一个变化很快的模拟量。所以这个时候就需要使用定时器TIMER了。DAC在初始化的时候，
      可以设置成使用定时器触发，这就意味着，当定时器溢满的时候，就会触发DAC工作。
      这样一来，就可以通过改变定时器的定时时间来改变正弦波的周期了。

                          电压大小的显示用DAC来处理
*/

//#include "dac.h"
#include "sys.h"
#include "math.h"
#include "waveform.h"
#include "key.h"
#include "delay.h"
#define PI 3.14159
#define DAC_DHR12R1_ADDRESS 0x40007408

u16 sinTable[tableSize]; //256;
//u16 sawtoothTable[tableSize];//256;
//u16 triangleTable[tableSize];//256;
//u16 rectangleTable[tableSize];//256;

u8 KEY_Scan(u8 mode);

//将正弦波数据保存在静态内存中
void sin_Generation(void)
{
	u16 n;
	u16 temp = 1023;
	//temp=KEY_Scan(0);
	if (KEY_Scan(0) == 4)
	{
		temp = temp + 200;
	}
	for (n = 0; n < tableSize; n++)
	{
		sinTable[n] = (sin(2 * PI * n / tableSize) + 1) * (temp);
	}
}

//锯齿波
void sawtooth_Generation(void)
{
	u16 n;
	for (n = 0; n < tableSize; n++)
	{
		sinTable[n] = ((2 * n * 1000) / tableSize);
	}
}

//三角波
void triangle_Generation(void)
{
	u16 n;
	for (n = 0; n < tableSize / 2; n++)
	{
		sinTable[n] = ((2 * n * 1000) / tableSize);
	}
	for (; n < tableSize; n++)
	{
		sinTable[n] = (2 * 1000 - (2 * n * 1000) / tableSize);
	}
}

//矩形波
void rectangle_Generation(void)
{
	u16 n;
	for (n = 0; n < tableSize / 2; n++)
	{
		sinTable[n] = 0;
	}
	for (; n < (tableSize); n++)
	{
		sinTable[n] = 1000;
	}
}

//当定时器溢满的时候，就会触发DAC工作，这样一来，就可以通过改变定时器的定时时间来改变正弦波的周期了
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
	GPIO_InitTypeDef GPIO_InitStructure; //结构体

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); //使能GPIOA时钟

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;		 //选定4号引脚,4,5都可以
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;	 //模拟输入
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //不上拉，不下拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);			 //确定为GPIOA的初始化
}

//后在DAC以及这块内存中间使用DMA建立一个通道
void DAC_DMA_Configuration(void)
{
	DAC_InitTypeDef DAC_InitStructure; //DAC结构体

	DMA_InitTypeDef DMA_InitStructure; //DMA结构体

	//使能DMA1时钟，则我们要找DMA1的请求映射来进行下面的配置
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE); //使能DAC时钟

	DAC_InitStructure.DAC_Trigger = DAC_Trigger_T6_TRGO;			//使用触发功能
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None; //不用STM32自带的波形
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;  //一般设置不使用输出缓存
	//STM32的DAC有两个通道，这里我们使用的是DAC的通道1，所以对DAC进行初始化
	DAC_Init(DAC_Channel_1, &DAC_InitStructure);

	//DMA1请求映射，DMA的数据流5和通道7为DAC1功能
	DMA_DeInit(DMA1_Stream5);					   //设置数据流5
	DMA_InitStructure.DMA_Channel = DMA_Channel_7; //设置通道7
	//把正弦波的数据表保存在静态内存中
	DMA_InitStructure.DMA_PeripheralBaseAddr = DAC_DHR12R1_ADDRESS;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&sinTable; //定义DMA外设基地址
	DMA_InitStructure.DMA_BufferSize = tableSize;				 //设置DMA缓存大小
	//外设数据宽度
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	//设置为循环工作模式
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA1_Stream5, &DMA_InitStructure);

	DMA_Cmd(DMA1_Stream5, ENABLE); //使能DMA1数据流5

	DAC_Cmd(DAC_Channel_1, ENABLE); //使能DAC的通道1

	//将DAC通道和DMA通道接起来
	DAC_DMACmd(DAC_Channel_1, ENABLE); //使能DAC_DMA通道1
}

//void opt(void)
//{
//	u8 t ;
//	t=KEY_Scan(0);
//	if(t==2)
//	{
//		while(1)
//		{
//			sin_Generation();
//			if(KEY_Scan(0)!=1)
//				break;
//		}
//	}
//}

void MYDAC_Init(void)
{
	// opt();
	//u8 key;
	//sawtooth_Generation();
	DacGPIO_Configuration();
	TIM6_Configuration();
	DAC_DMA_Configuration();
	//	while(1)
	//	{
	//		key=KEY_Scan(0);		//得到键值
	//	   	if(key)
	//		{
	//			switch(key)
	//			{
	//				case WKUP_PRES:	//控制蜂鸣器
	//					sin_Generation();
	//					break;
	////				case KEY0_PRES:	//控制LED0翻转
	////
	////					break;
	//				case KEY1_PRES:	//控制LED1翻转
	//					triangle_Generation();
	//					break;
	//				case KEY2_PRES:	//同时控制LED0,LED1翻转
	//           sawtooth_Generation();
	//					break;
	//			}
	//		}
	//		else delay_ms(10);
	//	}
}

////	u8 opt=0;
////	opt = KEY_Scan(0);
////	if(opt==1)
////	{
////		while(1)
////		{
//	sin_Generation();
////		}
////	}
//	//else if(opt==2)
//	triangle_Generation();
//
//	//else if(opt==3)
//	sawtooth_Generation();
//
//	//else if(opt==4)
//	rectangle_Generation();
//
