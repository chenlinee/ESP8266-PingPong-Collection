#include "esp8266_work_status.h"
#include "work_group_status.h"
#include "esp8266_common.h"

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
#define IP_STRING_LENGTH        16
#define TCP_LINK_POOL           5
#define TCP_STATUS_LENGTH       18
typedef struct ESP8266_WORK_STATUS{
    u8 ap_sta_mode;
    u8 physical_equipment_id;
    char ip[IP_STRING_LENGTH];
    char tcp_status[TCP_LINK_POOL][TCP_STATUS_LENGTH];
} ESP8266_WORK_STATUS_DEF;

ESP8266_WORK_STATUS_DEF esp8266_work_status;




void esp8266_work_status_printf(void)
{
    printf("ESP8266_WORK_STATUS{\r\n");
    printf("\tap_sta_mode: %d;\r\n", esp8266_work_status.ap_sta_mode);
    printf("\tphysical_equipment_id: %d;\r\n", esp8266_work_status.physical_equipment_id);
    printf("\tip: %s;\r\n", esp8266_work_status.ip);
    printf("}\r\n");
}




//初始化esp8266_work_status
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

//从flash中恢复esp8266_work_status
void get_esp8266_work_status_from_work_group_data(u8 esp826_work_mode_code)
{
    set_esp8266_work_status_ap_sta_mode(esp826_work_mode_code);
    esp8266_work_status.physical_equipment_id = equipment_identification_get();
    get_work_group_equipment_x_ip(esp8266_work_status.physical_equipment_id, esp8266_work_status.ip);
    //关闭所有TCP连接
    esp8266_tcp_link_close(TCP_LINKID_ALL);
}



//设置esp8266当前IP地址
void set_esp8266_work_status_ip(char *ip)
{
    u8 i=0;
    for(;*(ip+i);i++)
    {
        esp8266_work_status.ip[i] = *(ip+i);
    }
    esp8266_work_status.ip[i] = 0;
}
//获取esp8266当前IP地址
char *get_esp8266_work_status_ip(char ip[])
{
    u8 i=0;
    for(;esp8266_work_status.ip[i];i++)
    {
        ip[i] = esp8266_work_status.ip[i];
    }
    ip[i] = 0;
    
    return ip;
}



//设置esp8266当前physical_equipment_id
void set_esp8266_work_status_physical_equipment_id(u8 physical_equipment_id)
{
    esp8266_work_status.physical_equipment_id = physical_equipment_id;
}
//获取esp8266当前physical_equipment_id
u8 get_esp8266_work_status_physical_equipment_id(void)
{
    return esp8266_work_status.physical_equipment_id;
}



//设置esp8266当前TCP连接状态
void set_esp8266_work_status_tcp(u8 linkId, u8 link_status)
{
    esp8266_work_status.tcp_status[linkId][0] = link_status;
}
//获取esp8266当前TCP连接状态
u8 get_esp8266_work_status_tcp(u8 linkId)
{
    return esp8266_work_status.tcp_status[linkId][0];
}



//设置esp8266当前TCP连接的设备
void set_esp8266_work_status_tcp_equipment(u8 linkId, u8 equipment_code)
{
    esp8266_work_status.tcp_status[linkId][1] = equipment_code;
}
//获取esp8266当前TCP连接的设备
u8 get_esp8266_work_status_tcp_equipment(u8 linkId)
{
    return esp8266_work_status.tcp_status[linkId][1];
}



//设置esp8266所处工作模式
void set_esp8266_work_status_ap_sta_mode(u8 esp826_work_mode_code)
{
    esp8266_work_status.ap_sta_mode = esp826_work_mode_code;
}
//获取esp8266所处工作模式
u8 get_esp8266_work_status_ap_sta_mode(void)
{
    return esp8266_work_status.ap_sta_mode;
}




/*
*********************************************************************************************************
*    函 数 名:  TCP_ACK_TCP_CONNECT_handle
*               TCP_ACK_TCP_CLOSED_handle
*
*    功能说明:  ESP8266返回TCP连接建立字符串时，更新ESP8266的工作状态记录表
*               ESP8266返回TCP连接关闭字符串时，更新ESP8266的工作状态记录表
*
*    形    参: ack_data *ack_data_recieve
*
*    返 回 值: 无
*********************************************************************************************************
*/
void TCP_ACK_TCP_CLOSED_handle(ack_data *ack_data_recieve)
{
    set_esp8266_work_status_tcp(ack_data_recieve->linkID, ESP8266_TCP_UNLINK);
    set_esp8266_work_status_tcp_equipment(ack_data_recieve->linkID, EQUIPMENT_INIT);
}

void TCP_ACK_TCP_CONNECT_handle(ack_data *ack_data_recieve)
{
    set_esp8266_work_status_tcp(ack_data_recieve->linkID, ESP8266_TCP_LINK);
}
