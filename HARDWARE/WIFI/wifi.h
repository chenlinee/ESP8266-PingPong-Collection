#ifndef __WIFI_H
#define __WIFI_H
#include "stdio.h"	
#include "sys.h" 

#define EN_WIFI_RX 			    1		//使能（1）/禁止（0）串口2接收
//如果想串口中断接收，请不要注释以下宏定义
void wifi_init(void);

void wifi_recieve_data_handle(void);
#endif

