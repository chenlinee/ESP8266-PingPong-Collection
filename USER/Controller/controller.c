#include "delay.h"
#include "controller.h"
#include "w25qxx.h"
#include "wifi.h"
#include <jansson.h>
#include "error_handle.h"
#include "task_create.h"
#include "struct_define.h"

/*
读取工作模式标识符
字节0，1为flash第一个扇区0-4095字节内包含的初始化Json字符串长度
字节2为该设备的工作模式，0=>初始化模式，1=>AP主机模式，2=>STA从机模式
*/
u8 datatemp[3];

//回车换行符
u8 enter[2] = {0x0d, 0x0a};

/*
*********************************************************************************************************
*    函 数 名: esp8266_ack_handle_on/esp8266_ack_handle_off
*    功能说明: 使能/关闭esp8266返回字符串解析
*    形    参: 无
*    返 回 值: 无
*********************************************************************************************************
*/
request_ack_handle handle_ack_message_send;
//开启esp8266返回字符串解析
void ESP8266_ack_handle_on(void)
{
    //开启esp8266
    handle_ack_message_send.flag=1;
    OSMboxPost(tcp_ack_handle_start,(void*)&handle_ack_message_send);//发送消息
    delay_ms(15);//给低优先级程序CPU时间接收消息
}
//关闭esp8266返回字符串解析
void ESP8266_ack_handle_off(void)
{
    handle_ack_message_send.flag=0;
    OSMboxPost(tcp_ack_handle_start,(void*)&handle_ack_message_send);//发送消息
    delay_ms(15);//给低优先级程序CPU时间接收消息
}

/*
*********************************************************************************************************
*    函 数 名: send_cmd_string_2_usart2
*    功能说明: 向串口2发送命令字符串,结束标志为字符串结束标志0x00（硬件层实现）
*    形    参: const char *cmd_string（字符串头指针，结束标志0x00）
*    返 回 值: 无
*********************************************************************************************************
*/
void send_cmd_string_2_usart2(const char *cmd_string)
{
    for(u8 i=0;*(cmd_string+i);i++)
    {
        USART_SendData(USART2, *(cmd_string+i));//向串口2发送数据
        while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);//等待发送结束
    }
    for(u8 i=0;i<2;i++)
    {
        USART_SendData(USART2, enter[i]);//向串口2发送数据
        while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);//等待发送结束
    }
}

/*
*********************************************************************************************************
*    函 数 名: send_cmd_json_2_ESP8266
*
*    功能说明: 处理一条固定格式的json_t格式的AT指令
*
*    形    参: json_t*（string）格式
*
*    返 回 值: 无
*********************************************************************************************************
*/
void send_cmd_json_2_ESP8266(json_t *string)
{
    /*
     **********************************************************************************
     *  json_string_length()返回值是字符串长度，不包含包括string类型最后的结束标识符0x00;
     *    如"ATE0"[0123]，返回值是4
     *  json_string_value()返回值是字符串，包含string类型最后的0x00;
     *    如"ATE0"，返回值是char[5]={"A", "T", "E", "0", 0x00};
     **********************************************************************************
     */
    u8 cmd_char_length = json_string_length(string);
    const char *cmd_json_char = json_string_value(string);  //随着函数结束自动释放常量内存
    
    send_cmd_string_2_usart2(cmd_json_char);
    
    ack_data *ack_data_recieve;
    u8 os_mail_read_err;
    for(u8 t=0;t<150;t++)
    {
        delay_ms(20);
        ack_data_recieve = OSMboxPend(tcp_ack_OK_get, 10, &os_mail_read_err);
        if( ack_data_recieve && (ack_data_recieve->flag==TCP_ACK_OK) ) {break;}
    }
}

/*<--------------------(  主要控制程序 )----------------------->
*<---------------------(  wifi_init_mode()  )------------------>
*<---------------------(  wifi_AP_mode()  )-------------------->
*<---------------------(  wifi_STA_mode()  )------------------->
*/

/*
*********************************************************************************************************
*    函 数 名: wifi_init_mode
*
*    功能说明: WiFi初始化模式，等待上位机决定设备进入AP/STA模式
*
*    形    参: 无
*
*    返 回 值: 无
*********************************************************************************************************
*/
void wifi_init_mode(void)
{
    //读出flash数据
    u16 json_char_length;
    json_char_length = ((u16)datatemp[0]<<8) + datatemp[1];
    u8 json_char[json_char_length];
    W25QXX_Read(json_char,100,json_char_length);
    
    //加载flash数据为Json格式
    json_error_t json_error;
    json_t *json_data, *cmd;
    json_data = json_loads((char*)json_char, JSON_ENSURE_ASCII, &json_error);
    
    //处理Json包的数据指令
    u8 cmd_quantity;
    if(json_is_array(json_data))
    {
        cmd_quantity = json_array_size(json_data)-1;
        ESP8266_ack_handle_on();
        for(u8 i=0; i<cmd_quantity; i++)
        {
            cmd = json_array_get(json_data, i);         //获取一条指令
            send_cmd_json_2_ESP8266(cmd);               //发送指令到芯片
            json_decref(cmd);
        }
        ESP8266_ack_handle_off();
    }else error_handle(1);
    json_decref(json_data);
    
    /*
    **********************************************************************************
    *    初始化第一部分完成，esp8266进入AP模式，建立TCP server(port:1037)
    *    接下来进入初始化第二部分，等待具有WiFi功能的上位机，决定该设备处于何种工作状态
    **********************************************************************************
    */
    ESP8266_ack_handle_on();//使能ESP8266字符串解析处理
    u8 os_mail_read_err;
    ack_data *ack_data_recieve;
    while(1)
    {
        delay_ms(30);
        ack_data_recieve = OSMboxPend(tcp_ack_OK_get, 10, &os_mail_read_err);
        if( ack_data_recieve && (ack_data_recieve->flag==TCP_ACK_IPD) )
        {
            json_data = json_loads((char*)ack_data_recieve->data, JSON_ENSURE_ASCII, &json_error);
            printf("controller.c:%s\r\n", json_dumps(json_data, JSON_ENSURE_ASCII));
            json_decref(json_data);
        }
    }
    
    
}

/*
*********************************************************************************************************
*    函 数 名: wifi_AP_mode
*    功能说明: 系统初始化完成后，进入AP工作模式，控制STA从机工作
*    形    参: 无
*    返 回 值: 无
*********************************************************************************************************
*/
void wifi_AP_mode(void)
{
    
}

/*
*********************************************************************************************************
*    函 数 名: wifi_STA_mode
*    功能说明: 系统上电后，选择WiFi模块的工作模式（初始化/AP/STA）
*    形    参: 无
*    返 回 值: 无
*********************************************************************************************************
*/
void wifi_STA_mode(void)
{
    
}

/*
*********************************************************************************************************
*    函 数 名: system_software_init
*    功能说明: 系统上电后，选择WiFi模块的工作模式（初始化/AP/STA）
*    形    参: 无
*    返 回 值: 无
*********************************************************************************************************
*/
void system_software_init(void)
{
    W25QXX_Read(datatemp, 0, 3);	//读出字符长和工作模式
    
    if(datatemp[2]==0) wifi_init_mode();
    else if(datatemp[2]==1) wifi_AP_mode();
    else if(datatemp[2]==2) wifi_STA_mode();
}


