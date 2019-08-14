#include "delay.h"
#include "controller.h"
#include "w25qxx.h"
#include "wifi.h"
#include "error_handle.h"
#include "task_create.h"
#include "struct_define.h"
#include "esp8266_common.h"
#include "tcp_json_handle.h"


/**
*********************************************************************************************************
*    函 数 名: esp8266_work_status_init
*
*    功能说明: esp8266芯片工作状态记录表初始化
*
*    形    参: 无
*
*    返 回 值: 无
*
*********************************************************************************************************
*/
ESP8266_WORK_STATUS_DEF esp8266_work_status;
void esp8266_work_status_init(void)
{
    esp8266_work_status.ap_sta_mode = ESP8266_WORK_MODE_INIT;
    esp8266_work_status.physical_equipment_id = EQUIPMENT_INIT;
    for(u8 i=0;i<IP_STRING_LENGTH;i++) { esp8266_work_status.ip[i] = 0x00; }
    for(u8 i=0;i<TCP_LINK_POOL;i++)
    {
        for(u8 j=0;j<TCP_STATUS_LENGTH;j++) { esp8266_work_status.tcp_status[i][j] = 0x00; }
    }
}

/**
*********************************************************************************************************
*    函 数 名: check_work_mode_change
*
*    功能说明: 检查是否需要改变工作状态
*
*    形    参: 无
*
*    返 回 值: bool
*
*********************************************************************************************************
*/
u8 check_work_mode_change()
{
    u8 *work_mode_change_recieve;
    u8 os_mail_read_err;
    work_mode_change_recieve = NULL;
    work_mode_change_recieve = OSMboxPend(work_mode_status_change, 10, &os_mail_read_err);
    if(work_mode_change_recieve && (*work_mode_change_recieve)) 
    {
        *work_mode_change_recieve = 0;
        ESP8266_ack_handle_off();
        return 1;
    }
    return 0;
}

/**
*********************************************************************************************************
*    函 数 名: handle_TCP_data_loop_task
*
*    功能说明: 处于长时间工作状态时，用这个函数循环接收TCP数据
*
*    形    参: 无
*
*    返 回 值: bool
*
*********************************************************************************************************
*/
void handle_TCP_data_loop_task(void)
{
    ESP8266_ack_handle_on();//使能ESP8266字符串解析处理
    u8 os_mail_read_err;
    ack_data *ack_data_recieve;
    while(1)
    {
        delay_ms(10);
        ack_data_recieve = OSMboxPend(tcp_ack_OK_get, 10, &os_mail_read_err);
        
        //接收到正确的确认消息(TCP_ACK_OK)
        if( ack_data_recieve && (ack_data_recieve->ack_type==TCP_ACK_OK) ) {ack_data_recieve = NULL;}
        
        //接收TCP 连接消息(TCP_ACK_TCP_CONNECT)
        if( ack_data_recieve && (ack_data_recieve->ack_type==TCP_ACK_TCP_CONNECT) )
        {
            TCP_ACK_TCP_CONNECT_handle(ack_data_recieve, &esp8266_work_status);
            ack_data_recieve = NULL;    //恢复接收邮箱状态
        }
        
        //接收TCP 断开连接消息(TCP_ACK_TCP_CLOSED)
        if( ack_data_recieve && (ack_data_recieve->ack_type==TCP_ACK_TCP_CLOSED) )
        {
            TCP_ACK_TCP_CLOSED_handle(ack_data_recieve, &esp8266_work_status);
            ack_data_recieve = NULL;    //恢复接收邮箱状态
        }
        
        //接收到TCP(TCP_ACK_IPD)消息--->>ack_data_recieve->data一定是Json Encode后的字符串
        if( ack_data_recieve && (ack_data_recieve->ack_type==TCP_ACK_IPD) )
        {
            TCP_ACK_IPD_handle(ack_data_recieve, &esp8266_work_status);
            ack_data_recieve = NULL;    //恢复接收邮箱状态
        }
        
        if(check_work_mode_change()) 
        {
            ESP8266_ack_handle_off();
            break;
        }
    }
}

/*<--------------------(  主要控制程序 )----------------------->
*<---------------------(  wifi_init_mode()  )------------------>
*<---------------------(  wifi_AP_mode()  )-------------------->
*<---------------------(  wifi_STA_mode()  )------------------->
*/

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
    send_at_cmd_json_stream(ESP8266_WORK_MODE_AP);
    
    handle_TCP_data_loop_task();
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
    send_at_cmd_json_stream(ESP8266_WORK_MODE_STA);
    
    handle_TCP_data_loop_task();
}


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
    send_at_cmd_json_stream(ESP8266_WORK_MODE_INIT);
    
    /*
    **********************************************************************************
    *    初始化第一部分完成，esp8266进入AP模式，建立TCP server(port:1037)
    *    接下来进入初始化第二部分，接收IPD的TCP消息，处理TCP连接建立断开
    **********************************************************************************
    */
    handle_TCP_data_loop_task();
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
    esp8266_work_status_init();     //esp8266芯片工作状态记录表初始化
    u8 datatemp[1];
    
    while(1)
    {
        W25QXX_Read(datatemp, 2, 1);	//读出字符长和工作模式
        if(datatemp[0]==ESP8266_WORK_MODE_INIT) wifi_init_mode();
        else if(datatemp[0]==ESP8266_WORK_MODE_AP) wifi_AP_mode();
        else if(datatemp[0]==ESP8266_WORK_MODE_STA) wifi_STA_mode();
        
        #if SERIAL_DEBUG_MODE
        printf("\r\n    work mode change\r\n\r\n");
        #endif
        delay_ms(10);
    }
}
