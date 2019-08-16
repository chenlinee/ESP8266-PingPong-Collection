#include "tcp_json_handle.h"
#include "esp8266_common.h"
#include "w25qxx.h"
#include <jansson.h>
#include "work_group_status.h"

//响应信息
u8 send_ack_message_above_128(u8 dataCode, char *message, u8 linkId, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    //json_string((const char*)message)作为传入参数不知是否会引起内存泄露
    json_t *ack_data = json_object(), *ack = json_object();
    json_object_set_new(ack_data, "message", json_string((const char*)message));
    
    char num_2_str[6];
    u16_2_string(esp8266_work_status->physical_equipment_id, num_2_str);
    json_object_set_new(ack, "id", json_string(num_2_str));
    
    u16_2_string(dataCode, num_2_str);
    json_object_set_new(ack, "dataCode", json_string(num_2_str));
    json_object_set_new(ack, "data", ack_data);

    if(!esp8266_work_status->tcp_status[linkId][0]) {return 0;}
    send_data_2_tcp_link(ack, linkId);
    
    json_decref(ack_data);
    json_decref(ack);
    return 1;
}

//拼接字符串
char * string_append(char data[], char *string)
{
    u8 data_end = 0;
    for(;data[data_end];data_end++);
    for(u8 i=0;*(string+i);i++) { data[data_end++] = *(string+i);}
    data[data_end]=0;
    return data;
}

//初始化为AP模式
void ap_mode_set_0(json_t *json_data, u8 linkId, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    json_t *data = json_object_get(json_data, "data");
    //更新本机状态表
    esp8266_work_status->physical_equipment_id = string_2_u16((char *)json_string_value(json_object_get(data, "physical_equipment_id")));
    esp8266_work_status->ip[0]=0;
    string_append(esp8266_work_status->ip, (char *)json_string_value(json_object_get(data, "ip")));
    //发送响应
    send_ack_message_above_128(128, "data recieved", linkId, esp8266_work_status);
    
    //关闭所有TCP连接
    esp8266_tcp_link_close(TCP_LINKID_ALL, esp8266_work_status);
    
    /*
    ****************************************************************************************************
    *    写入WiFi芯片工作指令
    ****************************************************************************************************
    */
    json_t *ap_cmd = json_array();
    //关闭回显
    #if SERIAL_DEBUG_MODE
    json_array_append_new(ap_cmd, json_string("ATE1"));
    #else
    json_array_append_new(ap_cmd, json_string("ATE0"));
    #endif
    //设置工作模式
    json_array_append_new(ap_cmd, json_string("AT+CWMODE_DEF=2"));
    //AT+CWSAP_CUR=设置WiFi名称和密码
    char cmd_string[128] = "AT+CWSAP_DEF=\"";
    string_append(cmd_string, (char *)json_string_value(json_object_get(data, "ssid")));
    string_append(cmd_string, "\",\"");
    string_append(cmd_string, (char *)json_string_value(json_object_get(data, "passwd")));
    string_append(cmd_string, "\",6,4");
    json_array_append_new(ap_cmd, json_string(cmd_string));
    //设置IP地址
    cmd_string[0]=0;
    string_append(cmd_string, "AT+CIPAP_DEF=\"");
    string_append(cmd_string, (char *)json_string_value(json_object_get(data, "ip")));
    string_append(cmd_string, "\"");
    json_array_append_new(ap_cmd, json_string(cmd_string));
    //设置多连接
    cmd_string[0]=0;
    string_append(cmd_string, "AT+CIPMUX=1");
    json_array_append_new(ap_cmd, json_string(cmd_string));
    //设置开启TCP服务器
    cmd_string[0]=0;
    string_append(cmd_string, "AT+CIPSERVER=1,1037");
    json_array_append_new(ap_cmd, json_string(cmd_string));
    //设置超时时间
    cmd_string[0]=0;
    string_append(cmd_string, "AT+CIPSTO=300");
    json_array_append_new(ap_cmd, json_string(cmd_string));
    //写入Flash && 改变工作状态
    work_mode_init_AT_cmd_write_2_flash(ap_cmd, ESP8266_WORK_MODE_AP);
    /*****************************************************************************************************/
    
    
    //更新work_group数据
    work_group_network_update((char *)json_string_value(json_object_get(data, "ssid")), (char *)json_string_value(json_object_get(data, "passwd")));
    equipment_identification_set(esp8266_work_status->physical_equipment_id);
    update_work_group_equipment_x_ip(esp8266_work_status->physical_equipment_id, esp8266_work_status->ip);
    
    //销毁Json数据
    json_decref(data);
    json_decref(ap_cmd);
}

void sta_mode_set_1(json_t *json_data, u8 linkId, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    json_t *data = json_object_get(json_data, "data");
    //更新本机状态表
    esp8266_work_status->physical_equipment_id = string_2_u16((char *)json_string_value(json_object_get(data, "physical_equipment_id")));
    esp8266_work_status->ip[0]=0;
    string_append(esp8266_work_status->ip, (char *)json_string_value(json_object_get(data, "ip")));
    
    //发送响应
    send_ack_message_above_128(128, "data recieved", linkId, esp8266_work_status);
    
    //关闭所有TCP连接
    esp8266_tcp_link_close(TCP_LINKID_ALL, esp8266_work_status);
    
    /*
    ****************************************************************************************************
    *    写入WiFi芯片工作指令
    ****************************************************************************************************
    */
    json_t *sta_cmd = json_object(), *sta_cmd_part1 = json_array(), *sta_cmd_part2 = json_array();
    //关闭回显
    #if SERIAL_DEBUG_MODE
    json_array_append_new(sta_cmd_part1, json_string("ATE1"));
    #else
    json_array_append_new(sta_cmd_part1, json_string("ATE0"));
    #endif
    //设置工作模式
    json_array_append_new(sta_cmd_part1, json_string("AT+CWMODE_DEF=1"));
    json_array_append_new(sta_cmd_part1, json_string("AT+CWLAPOPT=1,2"));
    json_array_append_new(sta_cmd_part1, json_string("AT+CWAUTOCONN=0"));
    //设置本机IP地址
    char cmd_string[128] = "";
    string_append(cmd_string, "AT+CIPSTA_DEF=\"");
    string_append(cmd_string, (char *)json_string_value(json_object_get(data, "ip")));
    string_append(cmd_string, "\"");
    json_array_append_new(sta_cmd_part1, json_string(cmd_string));
    json_object_set_new(sta_cmd, "sta_cmd_part1", sta_cmd_part1);
    
    //设置多连接
    cmd_string[0]=0;
    string_append(cmd_string, "AT+CIPMUX=1");
    json_array_append_new(sta_cmd_part2, json_string(cmd_string));
    //设置开启TCP服务器
    cmd_string[0]=0;
    string_append(cmd_string, "AT+CIPSERVER=1,1037");
    json_array_append_new(sta_cmd_part2, json_string(cmd_string));
    //设置超时时间
    cmd_string[0]=0;
    string_append(cmd_string, "AT+CIPSTO=300");
    json_array_append_new(sta_cmd_part2, json_string(cmd_string));
    json_object_set_new(sta_cmd, "sta_cmd_part2", sta_cmd_part2);
    //写入Flash && 改变工作状态
    work_mode_init_AT_cmd_write_2_flash(sta_cmd, ESP8266_WORK_MODE_STA);
    /*****************************************************************************************************/
    
    
    //更新work_group数据
    work_group_network_update((char *)json_string_value(json_object_get(data, "ssid")), (char *)json_string_value(json_object_get(data, "passwd")));
    equipment_identification_set(esp8266_work_status->physical_equipment_id);
    update_work_group_equipment_x_ip(EQUIPMENT_MASTER, (char *)json_string_value(json_object_get(data, "master_ip")));
    update_work_group_equipment_x_ip(esp8266_work_status->physical_equipment_id, esp8266_work_status->ip);
    
    //销毁Json数据
    json_decref(data);
    json_decref(sta_cmd_part1);
    json_decref(sta_cmd_part2);
    json_decref(sta_cmd);
}

void ack_register_2(json_t *json_data, u8 linkId, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    u8 equipment_code = string_2_u16((char *)json_string_value(json_object_get(json_data, "id")));
    json_t *data = json_object_get(json_data, "data");
    const char *ip = json_string_value(json_object_get(data, "ip"));
    update_work_group_equipment_x_ip(equipment_code, (char *)ip);
    
    //发送响应
    send_ack_message_above_128(193, "register success", linkId, esp8266_work_status);
    
    json_decref(data);
}
void ack_register_130(json_t *json_data, u8 linkId, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    json_t *data = json_object_get(json_data, "data");
    u8 equipment_code = string_2_u16((char *)json_string_value(json_object_get(data, "id")));
    char ip[16] ="";
    
    char ack_message[64] = "this id has registered, ip is \"";
    string_append(ack_message, get_work_group_equipment_x_ip(equipment_code, ip));
    get_work_group_equipment_x_ip(equipment_code, ip);
    string_append(ack_message, "\"");
    
    //发送响应
    send_ack_message_above_128(194, ack_message, linkId, esp8266_work_status);
    
    esp8266_tcp_link_close(linkId, esp8266_work_status);
    json_decref(data);
}


/**
*********************************************************************************************************
*    函 数 名: TCP_ACK_IPD_handle
*
*    功能说明: 分类TCP传来的JSON格式数据，送往各个专门函数处理
*
*    形    参: ack_data *ack_data_recieve, 接收到的TCP字符串
*              ESP8266_WORK_STATUS_DEF *esp8266_work_status, ESP8266状态句柄
*
*    返 回 值: u8, Json的
*
*********************************************************************************************************
*/
u8 TCP_ACK_IPD_handle(ack_data *ack_data_recieve, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    u8 return_code = 255;
    
    u8 linkId = ack_data_recieve->linkID;
    
    //加载Json数据
    json_t *json_data;
    json_error_t json_error;
    json_data = json_loads((char*)ack_data_recieve->data, JSON_DECODE_ANY, &json_error);
    
    //不符合Json格式的错误TCP数据
    if(~json_error.line) //没有错误时，0xFFFFFFFF
    {
        send_ack_message_above_128(255, json_error.text, linkId, esp8266_work_status);
        return return_code;
    }
    
    //处理Json数据
    u8 dataCode = string_2_u16((char *)json_string_value(json_object_get(json_data, "dataCode")));
    switch(dataCode) 
    {
        case 0 :
            ap_mode_set_0(json_data, linkId, esp8266_work_status);
            return_code = 0;
            break;
        case 1 :
            sta_mode_set_1(json_data, linkId, esp8266_work_status);
            return_code = 1;
            break;
        case 2 :
            ack_register_2(json_data, linkId, esp8266_work_status);
            return_code = 2;
            break;
        case 129 :
            send_ack_message_above_128(192, "init succeed, work on AP master mode", linkId, esp8266_work_status);
            return_code = 129;
            break;
        case 130 :
            ack_register_130(json_data, linkId, esp8266_work_status);
            return_code = 130;
            break;
        case 193 :
            return_code = 193;
            break;
        default :
            send_ack_message_above_128(254, "unknown dataCode", linkId, esp8266_work_status);
            return_code = 254;
    }
    
    //销毁Json数据
    json_decref(json_data);
    
    return return_code;
}

