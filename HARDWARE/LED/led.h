#ifndef __LED_H
#define __LED_H	 
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//STM32F103ZE核心板
//LED驱动代码	   
								  
////////////////////////////////////////////////////////////////////////////////// 
#define LED0 PBout(5)// PB5
#define LED1 PEout(5)// PE5	

void LED_Init(void);//初始化

		 				    
#endif
