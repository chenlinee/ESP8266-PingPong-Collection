#ifndef __ESP8266_WORK_STATUS
#define __ESP8266_WORK_STATUS

#include "sys.h"
#include "struct_define.h"
/*
*********************************************************************************************************
*   功能说明: 记录芯片ESP8266的工作状态
*   
*   结    构: ap_sta_mode               :(u8    )ESP8266所处的工作状态
*                                         - 0: ap_mode
*                                         - 1: sta_mode
*
*             physical_equipment_id     :(u8    )ESP8266当前所处实际设备的地址-->PHYSICAL EQUIPMENT CODES
*
*             ip[IP_STRING_LENGTH]      :(u8    )ESP8266当前的IP地址
*                                         - 0123456789abcde    f 最长16位
*                                         -"192.168.123.123"0x00
*
*             tcp_status[TCP_LINK_POOL][TCP_STATUS_LENGTH]         
*                                       :(u8    )TCP连接资源池
*                                         - 0   1  23456789abcdefg   h 共18位
*                                         -0/1 0/1"192.168.123.124"0x00
*                                         -第1个字节表示该link id连接状态
*                                         -第二个字节表示是否获得该linkID对应的实际设备码，
*                                          同时用于标志是否获得IP地址-->PHYSICAL EQUIPMENT CODES
*
*********************************************************************************************************
*/

//测试用
void esp8266_work_status_printf(void);


void esp8266_work_status_init(void);

void get_esp8266_work_status_from_work_group_data(u8 esp826_work_mode_code);


//设置esp8266所处工作模式
void set_esp8266_work_status_ap_sta_mode(u8 esp826_work_mode_code);
//获取esp8266所处工作模式
u8 get_esp8266_work_status_ap_sta_mode(void);


//设置esp8266当前IP地址
void set_esp8266_work_status_ip(char *ip);
//获取esp8266当前IP地址
char *get_esp8266_work_status_ip(char ip[]);

//设置esp8266当前physical_equipment_id
void set_esp8266_work_status_physical_equipment_id(u8 physical_equipment_id);
//获取esp8266当前physical_equipment_id
u8 get_esp8266_work_status_physical_equipment_id(void);

//设置esp8266当前TCP连接状态
void set_esp8266_work_status_tcp(u8 linkId, u8 link_status);
//获取esp8266当前TCP连接状态
u8 get_esp8266_work_status_tcp(u8 linkId);

//设置esp8266当前TCP连接的设备
void set_esp8266_work_status_tcp_equipment(u8 linkId, u8 equipment_code);
//获取esp8266当前TCP连接的设备
u8 get_esp8266_work_status_tcp_equipment(u8 linkId);

void TCP_ACK_TCP_CLOSED_handle(ack_data *ack_data_recieve);
void TCP_ACK_TCP_CONNECT_handle(ack_data *ack_data_recieve);

#endif
