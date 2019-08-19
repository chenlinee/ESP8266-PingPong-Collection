#include "fan.h"


//初始化PA1和PC3为输出口.并使能这两个口的时钟
/*
    电路图中，PC3对应网络标号SWITCH1，PA1对应网络标号SWITCH2
    但是为了方便布局，实际电路连接相反
    因此，PA1->SWITCH1, PC3->SWITCH2
    低电平有效
*/
//风扇控制 IO初始化
void FAN_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOC, ENABLE);	 //使能PA,PC端口时钟

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;			    //SWITCH1-->PA.1 端口配置
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	    //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	    //IO口速度为50MHz
    GPIO_Init(GPIOA, &GPIO_InitStructure);				    //根据设定参数初始化GPIOA.1
    GPIO_SetBits(GPIOA,GPIO_Pin_1);						    //PA.1 输出高

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;	    	    //SWITCH2-->PC.3 端口配置, 推挽输出
    GPIO_Init(GPIOC, &GPIO_InitStructure);	  			    //推挽输出 ，IO口速度为50MHz
    GPIO_SetBits(GPIOC,GPIO_Pin_3); 					    //PC.3 输出高
}

void fan_set(u8 fan_status)
{
    switch(fan_status)
    {
        case FAN_OFF :
            GPIO_SetBits(GPIOA,GPIO_Pin_1);
            GPIO_SetBits(GPIOC,GPIO_Pin_3);
            break;
        case FAN_LOW_SPEED :
            GPIO_ResetBits(GPIOA,GPIO_Pin_1);
            GPIO_SetBits(GPIOC,GPIO_Pin_3);
            break;
        case FAN_HIGH_SPEED :
            GPIO_ResetBits(GPIOA,GPIO_Pin_1);
            GPIO_ResetBits(GPIOC,GPIO_Pin_3);
            break;
    }
}

