#include "wifi.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "task_create.h"
#include "struct_define.h"

#include "ESP8266_common.h"
#include "esp8266_work_status.h"
/////////////////////////////////////////////////////////////////
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos 使用	  
#endif 

#if EN_WIFI_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
WIFI_DATA  wifi_data;     //接收缓存队列
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 WIFI_RX_STA=0;       //接收状态标记

void usart2_init(void)
{
    //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
  
    //USART2_TX   GPIOA.2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.2

    //USART2_RX	  GPIOA.3初始化
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;//PA3
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.3 

    //Usart2 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器

    //USART 初始化设置

    USART_InitStructure.USART_BaudRate = 115200;//串口波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

    USART_Init(USART2, &USART_InitStructure); //初始化串口2
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启串口接受中断
    USART_Cmd(USART2, ENABLE);                    //使能串口2 
    
    /*
    *******************************************************************************
    *   TC：发送完成 
    *     当包含有数据的一帧发送完成后，由硬件将该位置位。如果USART_CR1中的TCIE为1，则产生中断。由软件序列清除该位(先读USART_SR，然后写入USART_DR)。
    *     TC位也可以通过写入0来清除，只有在多缓存通讯中才推荐这种清除程序。 
    *       0：发送还未完成；1：发送完成。
    *   初始化时先读USART_SR位，避免硬件复位后第一个字节出错
    *******************************************************************************
    */
    USART_GetFlagStatus(USART2, USART_FLAG_TC);
}

void wifi_receive_buf_init(void)
{
    /*初始化接收缓存*/
    wifi_data.head=0;
    wifi_data.rear=0;
    wifi_data.count=0;
    for(u8 i=0;i<WIFI_DATA_LEN;i++) wifi_data.WIFI_REC_DATA_LEN[i]=0;
}

void wifi_init(void)
{
    delay_ms(100);  //wifi模块上电时，会打印大量无用数据。设置该延迟避免无用数据使用循环队列溢出
    usart2_init();
    wifi_receive_buf_init();
}

void USART2_IRQHandler(void)                	//串口2中断服务程序
{
	u8 Res;
#if SYSTEM_SUPPORT_OS 		//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
	{
		Res=USART_ReceiveData(USART2);	//读取接收到的数据
        
		if(((WIFI_RX_STA&0x8000)==0)&&wifi_data.count<WIFI_DATA_LEN)//接收未完成
		{
            if(Res == 0x3E) //发送数据">"符
            {
                wifi_data.WIFI_RX_BUF[wifi_data.rear][WIFI_RX_STA&0X3FFF]=Res;  //写入WiFi接收缓存数组
                WIFI_RX_STA++;
                WIFI_RX_STA|=0x8000;	//接收完成了 
                wifi_data.WIFI_REC_DATA_LEN[wifi_data.rear]=WIFI_RX_STA;
                WIFI_RX_STA=0;
                wifi_data.rear=(wifi_data.rear+1)%WIFI_DATA_LEN; //队列不满，指针加一
                wifi_data.count++;
            }
			else if(WIFI_RX_STA&0x4000)//接收到了0x0d
			{
				if(Res!=0x0a)WIFI_RX_STA=0;//接收错误,重新开始
				else
                {
                    WIFI_RX_STA|=0x8000;	//接收完成了 
                    wifi_data.WIFI_REC_DATA_LEN[wifi_data.rear]=WIFI_RX_STA;
                    WIFI_RX_STA=0;
                    wifi_data.rear=(wifi_data.rear+1)%WIFI_DATA_LEN; //队列不满，指针加一
                    wifi_data.count++;
                }
			}
			else //还没收到0X0D
			{	
				if(Res==0x0d)WIFI_RX_STA|=0x4000;
				else
                {
                    wifi_data.WIFI_RX_BUF[wifi_data.rear][WIFI_RX_STA&0X3FFF]=Res;  //写入WiFi接收缓存数组
                    WIFI_RX_STA++;
                    if(WIFI_RX_STA==(WIFI_REC_LEN-2)) WIFI_RX_STA=0;//接收数据过长,重新开始接收	  
                }
			}
		}   		 
    } 
#if SYSTEM_SUPPORT_OS 	//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntExit();  											 
#endif
} 

/*<--------------------(  wifi接收到的数据解析 )----------------------->*/

/*
*********************************************************************************************************
*    函 数 名: min
*
*    功能说明: 比较两个输入数据的大小，返回较小的数
*
*    形    参: (u16)num1, (u16)num2
*
*    返 回 值: u16
*********************************************************************************************************
*/
u16 min(u16 num1, u16 num2)
{
    if(num1<num2) {return num1;}
    return num2;
}
/*
*********************************************************************************************************
*    函 数 名: ack_string_check
*
*    功能说明: 比较ESP8266的返回的数据，是否与期待返回的数据一致
*
*    形    参: (char *)ack_string，期待的返回值
*              (u16   )length，期待的返回值长度
*
*    返 回 值: u8，一致则返回1，不一致返回0
*********************************************************************************************************
*/
u8 ack_string_check(char* ack_string, u8 length)
{
    length = min(length, wifi_data.WIFI_REC_DATA_LEN[wifi_data.head]&0x3fff);
    if(!length) {return 0;}
    for(u16 t=0;t<length;t++)
    {
        if( (ack_string[t]=='*') && (wifi_data.WIFI_RX_BUF[wifi_data.head][t]>='0')
                && (wifi_data.WIFI_RX_BUF[wifi_data.head][t]<='4')) {continue;}  //'*'表示通配符
        if( ((u8)ack_string[t])!=wifi_data.WIFI_RX_BUF[wifi_data.head][t]) {return 0;}
    }
    return 1;
}
/*
*********************************************************************************************************
*    函 数 名: esp8266_tcp_data_handle
*
*    功能说明: 处理与TCP连接相关的数据
*               根据ESP8266的数据手册，接受数据有如下过程。
*               1）client端发起TCP连接，
*                  SERVER端提示:0,CONNECT（0为TCP连接的ID，最多支持5个TCP连接）
*                  CLIENT端提示:OK（client发起TCP连接的AT指令为（多连接模式）
*               2）TCP连接发送数据，AT指令为:AT+CIPSENDEX=<link ID>,<length>(length最大为2048)
*                  数据发送方提示:>(接收到>后，串口开始接收数据，长度达到length或者遇到字符"\0"结束。C中"\"需转义，即"\\0")
*                  接收方收到数据，提示格式为:+IPD,<link ID>,<length>:<string>
*    形    参: 无
*
*    返 回 值: 无
*********************************************************************************************************
*/
ack_data ack_data_send;
u8 ack_data_find_ssid_send[2];
void ESP8266_tcp_IPDdata_handle(void)
{
    ack_data_send.ack_type=0;
    
    /* <!-------------------------"OK"---------------------------> */
    //FINISH
    char ack_OK[] = "OK";
    if(ack_string_check(ack_OK, 2)) 
    {
        ack_data_send.ack_type=TCP_ACK_OK;
        OSMboxPost(tcp_ack_OK_get,(void*)&ack_data_send);
        return;
    }
    
    /* <!------------------------"FAIL"--------------------------> */
    //FINISH
    char ack_FAIL[] = "FAIL";
    if(ack_string_check(ack_FAIL, 4)) 
    {
        ack_data_send.ack_type=TCP_ACK_FAIL;
        OSMboxPost(tcp_ack_OK_get,(void*)&ack_data_send);
        return;
    }
    
    /* <!-------------------------"*,CONNECT"--------------------> */
    char ack_CONNECT[] = "*,CONNECT";
    //TODO
    if(ack_string_check(ack_CONNECT, 9))
    {
        ack_data_send.ack_type=TCP_ACK_TCP_CONNECT;
        ack_data_send.linkID=(u8)(wifi_data.WIFI_RX_BUF[wifi_data.head][0] - '0');
        TCP_ACK_TCP_CONNECT_handle(&ack_data_send);
        return;
    }
    
    /* <!-------------------------"*,CLOSED"---------------------> */
    //TODO
    char ack_CLOSED[] = "*,CLOSED";
    if(ack_string_check(ack_CLOSED, 8))
    {
        ack_data_send.ack_type=TCP_ACK_TCP_CLOSED;
        ack_data_send.linkID=(u8)(wifi_data.WIFI_RX_BUF[wifi_data.head][0] - '0');
        TCP_ACK_TCP_CLOSED_handle(&ack_data_send);
        return;
    }
    
    /* <!-------------------------"+IPD,*"-----------------------> */
    //FINISH
    char ack_IPD[] = "+IPD,*";
    if(ack_string_check(ack_IPD, 6))
    {
        /*
        *******************************************************************************
        *   功能：解析WiFi接收的TCP相关消息
        *         - esp8266接收到TCP消息时，会返回如下格式
        *         - 0123456789abcdefghij
        *         - +IPD,0,12:0123456789
        *         - 0->link id
        *         - 10->此次接收的字节长度
        *         - 0123456789->冒号之后，为此次接收到的消息。为了调试方便，主从机之间发消息
        *           会加上\r\n两个额外字节，处理时要忽略这两个字节
        *******************************************************************************
        */
        u16 dataLength=0;
        ack_data_send.ack_type=TCP_ACK_IPD;
        ack_data_send.linkID = (u8)(wifi_data.WIFI_RX_BUF[wifi_data.head][5] - '0');
        //获取ACK信息中的字节长度信息
        u8 i;
        for(i=0;wifi_data.WIFI_RX_BUF[wifi_data.head][7+i]!=':';i++) {dataLength = dataLength*10 + (u8)(wifi_data.WIFI_RX_BUF[wifi_data.head][7+i] - '0');}
        //此例子中，跳出for循环时，i=2，表示字节长度的位数
        ack_data_send.data[dataLength-2] = 0x00;
        for(u16 j=0;j<dataLength-2;j++) {ack_data_send.data[j] = wifi_data.WIFI_RX_BUF[wifi_data.head][8+i+j];}
        //返回处理结果
        OSMboxPost(tcp_ack_OK_get,(void*)&ack_data_send);
        return;
    }
    
    /* <!---------------------------">"--------------------------> */
    //FINISH
    char ack_send_confirm[] = ">";
    if(ack_string_check(ack_send_confirm, 1))
    {
        ack_data_send.ack_type = TCP_ACK_SEND_CONFIRM;
        OSMboxPost(tcp_ack_OK_get,(void*)&ack_data_send);
        return;
    }
    
    /* <!---------------------------"SEND OK"--------------------------> */
    char ack_send_sendok[] = "SEND OK";
    if(ack_string_check(ack_send_sendok, 7))
    {
        ack_data_send.ack_type = TCP_ACK_SEND_OK;
        OSMboxPost(tcp_ack_OK_get,(void*)&ack_data_send);
        return;
    }
    
    /* <!--------------------------"+CWLAP:("--------------------------> */
    char ack_send_CWLAP_find_ssid[] = "+CWLAP:(";
    if(ack_string_check(ack_send_CWLAP_find_ssid, 8))
    {
        ack_data_find_ssid_send[0] = FIND_SSID_ACK_CONFIRM;
        OSMboxPost(find_the_ssid,(void*)&ack_data_find_ssid_send[0]);
        return;
    }
    
    /* <!----------------------"WIFI CONNECTED"------------------------> */
    char ack_send_CWJAP_connect_wifi[] = "WIFI CONNECTED";
    if(ack_string_check(ack_send_CWJAP_connect_wifi, 8))
    {
        ack_data_find_ssid_send[1] = TCP_ACK_WIFI_CONNECTED;
        OSMboxPost(find_the_ssid,(void*)&ack_data_find_ssid_send[1]);
        return;
    }
}

/*
*********************************************************************************************************
*    函 数 名: wifi_recieve_data_handle
*
*    功能说明: 处理ESP8266返回的二进制数据，解析后的数据通过邮箱tcp_ack_OK_get发送到上位机
*
*    形    参: 无
*
*    返 回 值: 无
*********************************************************************************************************
*/
void wifi_recieve_data_handle(void)
{
    u16 len_usart1;
    u16 len_wifi;
    u8 os_mail_read_err;
    
    int8_t handle_ack_message_recieve_flag=0;
    request_ack_handle *handle_ack_message_recieve;  //tcp消息处理标识符
    while(1)
    {
        //检查是否需要特殊处理esp8266字符串
        handle_ack_message_recieve = OSMboxPend(tcp_ack_handle_start,5,&os_mail_read_err);
        if( handle_ack_message_recieve ) {
            handle_ack_message_recieve_flag = handle_ack_message_recieve->ack_string_decode;
        }
        
        #if SERIAL_DEBUG_MODE
        //串口一接收的数据转发到串口二（WiFi模块），调试用
        if(USART_RX_STA&0x8000)     
        {
            len_usart1=USART_RX_STA&0x3fff;//得到此次接收到的数据长度
            USART_RX_BUF[len_usart1]=0x0d;
            USART_RX_BUF[len_usart1+1]=0x0a;
            for(u16 t=0;t<len_usart1+2;t++)
            {
                USART_SendData(USART2, USART_RX_BUF[t]);//向串口2发送数据
                while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);//等待发送结束
            }
            USART_RX_STA=0;
        }
        #endif
        
        
        
        if(wifi_data.WIFI_REC_DATA_LEN[wifi_data.head]&0x8000)
        {
            #if SERIAL_DEBUG_MODE
            //处理串口二接收到的数据，同时把串口二接收的数据转发到串口一（调试用）
            len_wifi=wifi_data.WIFI_REC_DATA_LEN[wifi_data.head]&0x3fff;//得到此次接收到的数据长度
            wifi_data.WIFI_RX_BUF[wifi_data.head][len_wifi]=0x0d;
            wifi_data.WIFI_RX_BUF[wifi_data.head][len_wifi+1]=0x0a;
            printf("usart2:");
            for(u16 t=0;t<len_wifi+2;t++)
            {
                USART_SendData(USART1, wifi_data.WIFI_RX_BUF[wifi_data.head][t]);//向串口1发送数据
                while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
            }
            #endif
            /*
            *******************************************************************************
            *   功能：解析WiFi接收的TCP相关消息
            *         使用一个邮箱事件，高优先级的任务开启处理函数
            *         由于高优先级的任务总是又优先权，可以保证待处理的数据总是在高优先级的任务
            *         需要的时候，才送往待处理区。
            *   为有用数据定义一个专用数据结构
            *       tcp_recieve{
            *           u8 handle_flag; //非零标志表示有数据等待处理
            *           u8 link_ID;
            *           u16 length;
            *           u8 data[400];
            *       }
            *   该结构保证每次仅处理一条指令
            *******************************************************************************
            */
            if(handle_ack_message_recieve_flag) {ESP8266_tcp_IPDdata_handle();}
            
            //指令长度状态为复位，出队列，队首指针加一
            wifi_data.WIFI_REC_DATA_LEN[wifi_data.head]=0;
            wifi_data.head=(wifi_data.head+1)%WIFI_DATA_LEN;
            wifi_data.count--;
        }
        delay_ms(20);
    }
}

#endif	
