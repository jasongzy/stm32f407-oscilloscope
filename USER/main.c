#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h"
#include "dac.h"
#include "adc.h"
#include "waveform.h"
#include "timer.h"
#include "stm32f4xx_it.h"
//ALIENTEK 探索者STM32F407开发板 实验13
//LCD显示实验-库函数版本
//技术支持：www.openedv.com
//淘宝店铺：http://eboard.taobao.com  
//广州市星翼电子科技有限公司  
//作者：正点原子 @ALIENTEK


void clear_point(u16 num);//更新显示屏当前列	
void Set_BackGround(void);//设置背景
void Lcd_DrawNetwork(void);//画网格
float get_vpp(u16 *buf);//获取峰峰值
void DrawOscillogram(u16* buf);//画波形图	

int main(void)
{
	u16   buff[800];
	float Adresult = 0;
	u8    Vpp_buff[20] = {0};
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);            //初始化延时函数
	uart_init(115200);		    //初始化串口波特率为115200
	LED_Init();					//初始化LED
 	LCD_Init();                 //初始化LCD FSMC接口
    Adc_Init();                 //初始化ADC
	Set_BackGround();	        //显示背景
	Lcd_DrawNetwork();
    MYDAC_Init();               //产生正弦波
	LED0 = 0;
	
  	while(1) 
	{		
		DrawOscillogram(buff);//画波形
		Adresult = get_vpp(buff);//峰峰值mv		
		sprintf((char*)Vpp_buff,"Vpp = %0.3fV",Adresult);
		POINT_COLOR = WHITE;
		BACK_COLOR = DARKBLUE;
		LCD_ShowString(330,425,210,24,24,Vpp_buff);
		LED0 = !LED0;					
	} 
}

void clear_point(u16 num)//更新显示屏当前列
{
	u16 index_clear_lie = 0; 
	POINT_COLOR = DARKBLUE ;
	for(index_clear_lie = 1;index_clear_lie < 400;index_clear_lie++)
	{		
		LCD_DrawPoint(num,index_clear_lie );
	}
	if(!(num%50))//判断hang是否为50的倍数 画列点
	{
		for(index_clear_lie = 10;index_clear_lie < 400;index_clear_lie += 10)
		{		
			LCD_Fast_DrawPoint(num ,index_clear_lie,WHITE );
		}
	}
	if(!(num%10))//判断hang是否为10的倍数 画行点
	{
		for(index_clear_lie = 50;index_clear_lie <400;index_clear_lie += 50)
		{		
			LCD_Fast_DrawPoint(num ,index_clear_lie,WHITE );
		}
	}	
	POINT_COLOR = YELLOW;	
}

void DrawOscillogram(u16 *buff)//画波形图
{
	static u16 Ypos1 = 0,Ypos2 = 0;
	u16 Yinit=100;
	u16 i = 0;
	POINT_COLOR = YELLOW;
	for(i = 1;i < 700;i++)//存储AD数值
	{
		buff[i] = Get_Adc(ADC_Channel_5);
	}
	for(i = 1;i < 700;i++)
	{
		clear_point(i );	
		Ypos2 = 400 - (Yinit + buff[i] * 165 / 4096);//转换坐标
		//Ypos2 = Ypos2 * bei;
		if(Ypos2 >700)
			Ypos2 =700; //超出范围不显示
		LCD_DrawLine (i ,Ypos1 , i+1 ,Ypos2);
		Ypos1 = Ypos2 ;
	}
    Ypos1 = 0;	
}

void Set_BackGround(void)
{
	POINT_COLOR = YELLOW;
    LCD_Clear(DARKBLUE);
	LCD_DrawRectangle(0,0,700,400);//矩形
	//LCD_DrawLine(0,220,700,220);//横线
	//LCD_DrawLine(350,20,350,420);//竖线
	//POINT_COLOR = WHITE;
	//BACK_COLOR = DARKBLUE;
	//LCD_ShowString(330,425,210,24,24,(u8*)"vpp=");	
}

void Lcd_DrawNetwork(void)
{
	u16 index_y = 0;
	u16 index_x = 0;	
	
    //画列点	
	for(index_x = 50;index_x < 700;index_x += 50)
	{
		for(index_y = 10;index_y < 400;index_y += 10)
		{
			LCD_Fast_DrawPoint(index_x,index_y,WHITE);	
		}
	}
	//画行点
	for(index_y = 50;index_y < 400;index_y += 50)
	{
		for(index_x = 10;index_x < 700;index_x += 10)
		{
			LCD_Fast_DrawPoint(index_x,index_y,WHITE);	
		}
	}
}

float get_vpp(u16 *buf)	   //获取峰峰值
{
	
	u32 max_data=buf[1];
	u32 min_data=0;//buf[1];
	u32 n=0;
	float Vpp=0;
	for(n = 1;n<700;n++)
	{
		if(buf[n] > max_data)
		{
			max_data = buf[n];
		}
//		if(buf[n] < min_data)
//		{
//			min_data = buf[n];
//		}			
	} 	
	Vpp = (float)(max_data - min_data);
	Vpp = Vpp*(3.3/4096);
	return Vpp;	
}

