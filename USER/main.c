#include "includes.h"
#include "sys.h" 	
#include "delay.h"	

#include "task_create.h"
#include "led.h"
#include "key.h"
#include "usart.h"
#include "wifi.h"
#include "w25qxx.h"
#include "controller.h"



/************************************************
//STM32F103ZE核心板
 UCOSII实验1-任务调度 实验 

************************************************/


u8 hardware_init(void)
{
    delay_init();	    	 //延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(115200);	 //串口初始化为115200
 	LED_Init();			     //LED端口初始化
	KEY_Init();          //初始化与按键连接的硬件接口
    W25QXX_Init();
    if(W25QXX_ReadID()!=W25Q16) return 0;
    wifi_init();
    return 1;
}

int main(void)
{	
    while(!hardware_init())
    {
        printf("W25Q16 Check Failed!\r\n");
		delay_ms(500);
        printf("Please Check!\r\n");
		delay_ms(500);
		LED1=!LED1;//DS1闪烁
        LED0=!LED1;
    }
    
    OSInit();   
    task_create();
    OSStart();
}

