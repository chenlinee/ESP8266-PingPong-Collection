#ifndef __TASK_CREAT_H
#define __TASK_CREAT_H

#include "includes.h"
#include "sys.h"
#include <jansson.h>


void task_create(void);


/////////////////////////UCOSII任务设置///////////////////////////////////
//START 任务
//设置任务优先级
#define START_TASK_PRIO      			20 //开始任务的优先级设置为最低
//设置任务堆栈大小
#define START_STK_SIZE  				64
//任务堆栈在src中定义
//OS_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *pdata);


//LED任务
//设置任务优先级
#define LED_TASK_PRIO       			19
//设置任务堆栈大小
#define LED_STK_SIZE  		    		64
//任务堆栈在src中定义
//OS_STK LED_TASK_STK[LED0_STK_SIZE];
//任务函数
void led_task(void *pdata);

//串口调试程序
//设置任务优先级
#define SERIAL_TASK_PRIO                18
//设置任务堆栈大小
#define SERIAL_STK_SIZE                 150
//任务堆栈在src中定义
//OS_STK SERIAL_TASK_STK[SERIAL_STK_SIZE];
//任务函数
void serial_task(void *pdata);

//WiFi工作模式程序
//设置任务优先级
#define WIFIMODE_TASK_PRIO              10
//设置任务堆栈大小
#define WIFIMODE_STK_SIZE               400
//任务堆栈在src中定义
//OS_STK WIFIMODE_TASK_STK[WIFIMODE_STK_SIZE];
//任务函数
void wifiMode_task(void *pdata);

extern OS_EVENT * tcp_ack_handle_start;			//tcp ack信息邮箱事件块指针
extern OS_EVENT * tcp_ack_OK_get;               //确认是否得到OK回信

#endif
