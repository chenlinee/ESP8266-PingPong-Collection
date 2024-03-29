#include "task_create.h"
#include "led.h"
#include "key.h"
#include "delay.h"
#include "wifi.h"
#include "usart.h"
#include "work_group_init.h"
#include "struct_define.h"
#include <jansson.h>

//初始化任务堆栈空间
OS_STK START_TASK_STK[START_STK_SIZE];
OS_STK LED_TASK_STK[LED_STK_SIZE];
OS_STK SERIAL_TASK_STK[SERIAL_STK_SIZE];
OS_STK WIFIMODE_TASK_STK[WIFIMODE_STK_SIZE];
OS_STK KEY_TASK_STK[KEY_STK_SIZE];

//通知事件
OS_EVENT * tcp_ack_handle_start;			//tcp处理邮箱事件块指针
OS_EVENT * tcp_ack_OK_get;                  //确认是否得到OK回信
OS_EVENT * work_mode_status_change;         //工作状态改变
OS_EVENT * find_the_ssid;                   //检测ssid，收到WiFi特定信息后，才使用此信号量
OS_EVENT * key_detect;                      //按键检测邮箱
OS_EVENT * led_status;                      //LED显示状态邮箱

//创建开始任务
void task_create(void)
{
    OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//创建起始任务
}

//开始任务
void start_task(void *pdata)
{
    OS_CPU_SR cpu_sr=0;
	pdata = pdata; 
    
    tcp_ack_handle_start = OSMboxCreate((void*)0);	//创建tcp消息处理邮箱
    tcp_ack_OK_get = OSMboxCreate((void*)0);
    work_mode_status_change = OSMboxCreate((void*)0);
    find_the_ssid = OSMboxCreate((void*)0);
    key_detect = OSMboxCreate((void*)0);
    led_status = OSMboxCreate((void*)0);
    
  	OS_ENTER_CRITICAL();			//进入临界区(无法被中断打断)
    OSTaskCreate(key_task,(void *)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO);
 	OSTaskCreate(led_task,(void *)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);
 	OSTaskCreate(serial_task,(void *)0,(OS_STK*)&SERIAL_TASK_STK[SERIAL_STK_SIZE-1],SERIAL_TASK_PRIO);
    OSTaskCreate(wifiMode_task,(void *)0,(OS_STK*)&WIFIMODE_TASK_STK[WIFIMODE_STK_SIZE-1],WIFIMODE_TASK_PRIO);
	OSTaskSuspend(START_TASK_PRIO);	//挂起起始任务.
	OS_EXIT_CRITICAL();				//退出临界区(可以被中断打断)
}

//按键检测
void key_task(void *pdata)
{
    u8 key, i=0;		    						 
	while(1)
	{
		key=KEY_Scan(1);   
		if(key==KEY0_PRES)
        {
            i++;
            if(i==80)   //每个周期50毫秒, 共4秒
            {   
                i=0;
                #if SERIAL_DEBUG_MODE
                printf("key press\r\n");
                #endif
                OSMboxPost(key_detect,(void*)&key);//发送消息
            }
        } else { i=0;}
        
 		delay_ms(40);
	}
}

//LED任务
void led_task(void *pdata)
{
    LED1 = 0;
	while(1)
	{
		LED0=0;
		delay_ms(80);
		LED0=1;
		delay_ms(920);
	}
}

//串口调试程序
void serial_task(void *pdata)
{
    wifi_recieve_data_handle();
}

//WiFi工作模式选择程序
void wifiMode_task(void *pdata)
{
    work_group_connection_init();
    while(1)
    {
        LED1=!LED1;
        delay_ms(500);
    }
}



