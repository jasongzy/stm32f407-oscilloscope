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

#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h"
#include "beep.h"
//#include "dac.h"
#include "adc.h"
#include "waveform.h"
#include "timer.h"
#include "stm32f4xx_it.h"
#include "key.h"
#include "stm32f4xx.h" // Device header
//void touch_task(void);  //触摸功能，可添加，这里只有TFTLCD所以不用
void clear_point(u16 num);		//更新显示屏当前列
void Set_BackGround(void);		//设置背景
void Lcd_DrawNetwork(void);		//画网格
float get_vpp(u16 *buf);		//获取峰峰值
void DrawOscillogram(u16 *buf); //画波形图

void sin_Generation(void);
void sawtooth_Generation(void);
void triangle_Generation(void);
void rectangle_Generation(void);

u32 max_data;
u16 position = 200; //波形图中心轴
u8 num = 0;
//u8 runstop = 1;

int main(void)
{
	u16 buff[800];
	float Adresult = 0;
	u8 Vpp_buff[20] = {0};
	u8 key = 0;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置系统中断优先级分组2
	delay_init(168);								//初始化延时函数
	uart_init(115200);								//初始化串口波特率为115200
	LED_Init();										//初始化LED
	BEEP_Init();									//初始化蜂鸣器端口
	LCD_Init();										//初始化LCD FSMC接口
	Adc_Init();										//初始化ADC
	Set_BackGround();								//显示背景
	Lcd_DrawNetwork();
	KEY_Init();
	LED0 = 0;
	MYDAC_Init();

	while (1)
	{
		//void touch_task(void);
		DrawOscillogram(buff);	//画波形
		Adresult = get_vpp(buff); //峰峰值mv
		sprintf((char *)Vpp_buff, "Vpp = %0.3fV", Adresult);
		POINT_COLOR = WHITE;
		BACK_COLOR = DARKBLUE;
		//LCD_ShowString(156,201,66,12,12,Vpp_buff);
		LCD_ShowString(330, 425, 210, 24, 24, Vpp_buff);
		//runstop=KEY_Scan(0);
		//		if(runstop==1){
		POINT_COLOR = WHITE;
		//LCD_ShowString(258,0,288,29,12,"STOP");
		LCD_ShowString(600, 425, 288, 29, 24, "STOP");
		POINT_COLOR = YELLOW;
		//LCD_ShowString(289,0,319,29,12,"RUN");
		LCD_ShowString(665, 425, 319, 29, 24, "RUN");
		LED0 = !LED0; //}
					  //else if(runstop==0)
					  //{
					  //	POINT_COLOR = YELLOW;
					  //	LCD_ShowString(258,0,288,29,12,"STOP");
					  //	LCD_ShowString(289,0,319,29,12,"RUN");
					  //}

		key = KEY_Scan(0); //得到键值
		if (key)
		{
			switch (key)
			{
			case WKUP_PRES:
				sin_Generation();
				BEEP = 1;
				delay_ms(100);
				BEEP = 0;
				break;
				//				case KEY0_PRES:
				//					runstop=0;
				//					break;
			case KEY1_PRES:
				triangle_Generation();
				BEEP = 1;
				delay_ms(100);
				BEEP = 0;
				break;
			case KEY2_PRES:
				sawtooth_Generation();
				BEEP = 1;
				delay_ms(100);
				BEEP = 0;
				break;
			}
		}
		else if (key == KEY0_PRES)
		{
			BEEP = 1;
			delay_ms(100);
			BEEP = 0;
			do
			{
				//runstop=0;
				//void DrawOscillogram(u16* buf);
				POINT_COLOR = YELLOW;
				//LCD_ShowString(258,0,288,29,12,"STOP");
				LCD_ShowString(600, 425, 288, 29, 24, "STOP");
				POINT_COLOR = WHITE;
				//LCD_ShowString(289,0,319,29,12,"RUN");
				LCD_ShowString(665, 425, 319, 29, 24, "RUN");
				key = KEY_Scan(0);
				if (key == 0 || key == 2 || key == 3 || key == 4)
				{
					BEEP = 1;
					delay_ms(100);
					BEEP = 0;
					switch (key)
					{
					case WKUP_PRES:
						sin_Generation();
						break;
					case KEY1_PRES:
						triangle_Generation();
						break;
					case KEY2_PRES:
						sawtooth_Generation();
						break;
					}
					break;
				}
			} while (1);
		}
	}
}

void clear_point(u16 num) //更新显示屏当前列
{
	u16 lie = 0;
	POINT_COLOR = DARKBLUE;
	for (lie = 1; lie < 400; lie++)
	{
		LCD_DrawPoint(num, lie);
	}
	if (!(num % 50)) //判断hang是否为50的倍数 画列点
	{
		for (lie = 10; lie < 400; lie += 10)
		{
			LCD_Fast_DrawPoint(num, lie, WHITE);
		}
	}
	if (!(num % 10)) //判断hang是否为10的倍数 画行点
	{
		for (lie = 50; lie < 400; lie += 50)
		{
			LCD_Fast_DrawPoint(num, lie, WHITE);
		}
	}
	POINT_COLOR = YELLOW;
}

void DrawOscillogram(u16 *buff) //画波形图
{
	//runstop = KEY_Scan(0);

	//if (runstop == 1 | runstop == 2 | runstop == 3 | runstop == 4) //画正弦波
	//{
	static u16 Ypos1 = 0, Ypos2 = 0;
	u16 i = 0;
	for (i = 1; i < 700; i++) //存储AD数值
	{
		buff[i] = Get_Adc(ADC_Channel_5);
		//					delay_us(1);
	}
	for (i = 1; i < 700; i++)
	{
		clear_point(i);
		Ypos2 = position - (buff[i] * 165 / 4096); //转换坐标//4096
		Ypos2 = Ypos2 * 2;						   //纵坐标倍数
		if (Ypos2 > 400)
			Ypos2 = 400;					  //超出范围不显示
		LCD_DrawLine(i, Ypos1, i + 1, Ypos2); //波形连接
		Ypos1 = Ypos2;
	}
	Ypos1 = 0;
	//}

	//		else if(runstop==0)//暂停动态显示
	//		{
	//			do
	//			{
	////				u8 a=0;
	//				runstop = KEY_Scan(0);
	//				POINT_COLOR = YELLOW;
	//				LCD_ShowString(258,0,288,29,12,"STOP");
	//				POINT_COLOR = WHITE;
	//				LCD_ShowString(289,0,319,29,12,"RUN");
	//				//runstop=1;
	//				if( runstop == 2|runstop==3|runstop==4 ) break;
	//			}
	//			while(1);
	//		}

	//else if (runstop == 2)
	//{
	//			static u16 Ypos1 = 0,Ypos2 = 0;
	//			u16 i = 0;
	//			for(i = 1;i < 255;i++)//存储AD数值
	//				{
	//					buff[i]=Get_Adc(ADC_Channel_5);
	////					delay_us(1);
	//				}
	//				for(i = 1;i < 255;i++)
	//				{
	//					clear_point(i);
	//					Ypos2 = position - ( buff[i] * 165 / 4096);//转换坐标//4096
	//					if(Ypos2 >255)
	//						Ypos2 =255; //超出范围不显示
	//					LCD_DrawLine (i ,Ypos1 , i+1 ,Ypos2);//波形连接
	//				Ypos1 = Ypos2 ;
	//				}
	//    Ypos1 = 0;
	//}

	//else if (runstop == 3)
	//{
	//}

	//else if (runstop == 4)
	//{
	//}
}
void Set_BackGround(void)
{
	LCD_Clear(DARKBLUE);
	POINT_COLOR = WHITE;
	LCD_DrawRectangle(0, 0, 700, 400);
	//	POINT_COLOR = WHITE;
	//	LCD_ShowString(258,0,288,29,12,"STOP");
	//	LCD_ShowString(289,0,319,29,12,"RUN");
}

void Lcd_DrawNetwork(void)
{
	u16 index_y = 0;
	u16 index_x = 0;
	//POINT_COLOR = WHITE;
	//LCD_DrawRectangle(258,0,288,29);
	//POINT_COLOR = WHITE;
	//LCD_DrawRectangle(289,0,319,29);
	//画列点
	for (index_x = 50; index_x < 700; index_x += 50)
	{
		for (index_y = 10; index_y < 400; index_y += 10)
		{
			LCD_Fast_DrawPoint(index_x, index_y, WHITE);
		}
	}
	//画行点
	for (index_y = 50; index_y < 400; index_y += 50)
	{
		for (index_x = 10; index_x < 700; index_x += 10)
		{
			LCD_Fast_DrawPoint(index_x, index_y, WHITE);
		}
	}
}

float get_vpp(u16 *buf) //获取峰峰值
{

	u32 max_data = buf[0];
	u32 min_data = buf[0];
	u32 n = 0;
	float Vpp = 0;
	for (n = 1; n < 256; n++)
	{
		if (buf[n] > max_data)
		{
			max_data = buf[n];
		}
		else if (buf[n] < min_data)
			min_data = buf[n];
	}
	Vpp = (float)(max_data - min_data);
	Vpp = Vpp * (3.3 / 4096);
	max_data = max_data * (3.3 / 4096);
	min_data = min_data * (3.3 / 4096);
	return Vpp;
}
