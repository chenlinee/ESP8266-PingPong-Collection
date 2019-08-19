#include "delay.h"
#include "work_group_init.h"
#include "w25qxx.h"
//#include "wifi.h"
//#include "error_handle.h"
//#include "task_create.h"
#include "struct_define.h"
#include "esp8266_common.h"
#include "tcp_json_handle.h"
#include "work_group_status.h"
#include "work_group_online_status.h"
#include "esp8266_work_status.h"
#include "collection.h"

/**
*********************************************************************************************************
*    函 数 名: slave_register
*
*    功能说明: 从机加入网络后，向主机发起注册
*
*    形    参: void
*
*    返 回 值: bool
*
*********************************************************************************************************
*/
u8 slave_register(void)
{
    u8 success_flag=0;
    json_t *data=json_object();
    
    char ip[16];
    json_object_set_new(data, "ip", json_string(get_esp8266_work_status_ip(ip)));
    
    if(send_data_2_equipment(data, EQUIPMENT_MASTER, 2, 193)) {success_flag=1;}
    json_decref(data);
    
    return success_flag;
}

/**
*********************************************************************************************************
*    函 数 名: send_ready_meesage_2_slave
*
*    功能说明: 主机确定网络组在线设备准备完成之后，通知所有从机
*
*    形    参: void
*
*    返 回 值: bool
*
*********************************************************************************************************
*/
u8 send_ready_meesage_2_slave(void)
{
    u8 success_flag=0;
    json_t *data[MAX_EQUIPMENT-1];
    for(u8 equipment_code=EQUIPMENT_SLAVE_1;equipment_code<=MAX_EQUIPMENT;equipment_code++)
    {
        data[equipment_code-2]=json_object();
        json_object_set_new(data[equipment_code-2], "message", json_string("work group online init success"));
    }
    
    for(u8 equipment_code=EQUIPMENT_SLAVE_1;equipment_code<=MAX_EQUIPMENT;equipment_code++)
    {
        if(send_data_2_equipment(data[equipment_code-2], equipment_code, 131, 196)) { success_flag++;}
    }
    
    for(u8 equipment_code=EQUIPMENT_SLAVE_1;equipment_code<=MAX_EQUIPMENT;equipment_code++)
    {
        json_decref(data[equipment_code-2]);
    }
    
    if(success_flag==MAX_EQUIPMENT-1) {return 1;}
    else {return 0;}
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
    //设备状态恢复
    EQUIPMENT_QUERY_KEY_init();
    get_esp8266_work_status_from_work_group_data(ESP8266_WORK_MODE_AP);
    work_group_online_init();
    
    //从机的IP可能会改变，先初始化，等从机注册之后再写入
    for(u8 equipment_code=EQUIPMENT_SLAVE_1;equipment_code<MAX_EQUIPMENT+1;equipment_code++)
    {
        update_work_group_equipment_x_ip(equipment_code, "");
    }
    
    //硬件初始化
    ESP8266_ack_handle_on();
    send_at_cmd_json_stream(ESP8266_WORK_MODE_AP);
    
    //本机准备就绪
    set_work_group_online_equipment_status(get_esp8266_work_status_physical_equipment_id(), READY);
    
    //打印测试
    esp8266_work_status_printf();
    work_group_online_status_printf();
    
    
    //处理TCP消息
    handle_TCP_data_loop_task();
    
    if(check_work_group_online_status()==READY)
    {
        //通知所有从机，网络组准备就绪
        if(send_ready_meesage_2_slave())
        {
            //进入收集模式
            collection_test();
        }
    }
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
    //设备状态恢复
    EQUIPMENT_QUERY_KEY_init();
    get_esp8266_work_status_from_work_group_data(ESP8266_WORK_MODE_STA);
    work_group_online_init();
    //硬件初始化
    ESP8266_ack_handle_on();
    send_at_cmd_json_stream(ESP8266_WORK_MODE_STA);
    
    //向主机发起注册
    while(1)
    {
        if(slave_register())
        {
            set_work_group_online_equipment_status(get_esp8266_work_status_physical_equipment_id(), READY);
            break;
        }
        delay_ms(500);
    }
    
    //打印测试
    esp8266_work_status_printf();
    work_group_online_status_printf();
    
    //Json数据处理
    ESP8266_ack_handle_on();//使能ESP8266字符串解析处理
    handle_TCP_data_loop_task();
    
    if(check_work_group_online_status()==READY) { collection_test(); }
    
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
    work_group_status_init();              //工作组初始化
    ESP8266_ack_handle_on();
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
void work_group_connection_init(void)
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
