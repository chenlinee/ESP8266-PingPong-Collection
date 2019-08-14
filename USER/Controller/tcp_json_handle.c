#include "tcp_json_handle.h"
#include "esp8266_common.h"
#include "w25qxx.h"
#include <jansson.h>

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
void string_append(char data[], char *string)
{
    u8 data_end = 0;
    for(;data[data_end];data_end++);
    for(u8 i=0;*(string+i);i++) { data[data_end++] = *(string+i);}
    data[data_end]=0;
}

//初始化为AP模式
void ap_sta_mode_set_0(json_t *json_data, u8 linkId, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    json_t *data = json_object_get(json_data, "data");
    u8 physical_equipment_id = string_2_u16((char *)json_string_value(json_object_get(data, "physical_equipment_id")));
    esp8266_work_status->physical_equipment_id = physical_equipment_id;
    esp8266_work_status->ip[0]=0;
    string_append(esp8266_work_status->ip, (char *)json_string_value(json_object_get(data, "ip")));
    //发送响应
    send_ack_message_above_128(128, "data recieved", linkId, esp8266_work_status);
    
    //关闭所有TCP连接
    esp8266_tcp_link_close(TCP_LINKID_ALL, esp8266_work_status);
    
    //构建AP模式AT指令
    json_t *ap_cmd = json_array();
    //关闭回显
    #if SERIAL_DEBUG_MODE
    json_array_append_new(ap_cmd, json_string("ATE1"));
    #else
    json_array_append_new(ap_cmd, json_string("ATE0"));
    #endif
    //设置工作模式
    json_array_append_new(ap_cmd, json_string("AT+CWMODE_CUR=2"));
    //AT+CWSAP_CUR=设置WiFi名称和密码
    char cmd_string[128] = "AT+CWSAP_CUR=\"";
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
    
    
    //写入Flash外存
    char *ap_mode_at_string = json_dumps(ap_cmd, JSON_ENSURE_ASCII);
    u16 size = 0;
    for(size=0;*(ap_mode_at_string+size);size++);
    size++;
    u8 size_8[3];
    size_8[0]=(u8)(size>>8);
    size_8[1]=(u8)size;
    size_8[2]=ESP8266_WORK_MODE_AP;
    
    W25QXX_Write((u8*)(size_8+2),2,1);
    W25QXX_Write((u8*)size_8,4096,2);
    W25QXX_Write((u8*)ap_mode_at_string,4096+100,size);
    
    //销毁Json数据
    free(ap_mode_at_string);
    json_decref(data);
    json_decref(ap_cmd);
    
    //改变工作状态
    work_mode_change();
}

void ap_sta_mode_set_1(json_t *json_data, u8 linkId, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    json_t *data = json_object_get(json_data, "data");
    u8 physical_equipment_id = string_2_u16((char *)json_string_value(json_object_get(data, "physical_equipment_id")));
    esp8266_work_status->physical_equipment_id = physical_equipment_id;
    esp8266_work_status->ip[0]=0;
    string_append(esp8266_work_status->ip, (char *)json_string_value(json_object_get(data, "ip")));
    //发送响应
    send_ack_message_above_128(128, "data recieved", linkId, esp8266_work_status);
    
    //关闭所有TCP连接
    esp8266_tcp_link_close(TCP_LINKID_ALL, esp8266_work_status);
    
    //构建AP模式AT指令
    json_t *sta_cmd = json_array();
    //关闭回显
    #if SERIAL_DEBUG_MODE
    json_array_append_new(sta_cmd, json_string("ATE1"));
    #else
    json_array_append_new(sta_cmd, json_string("ATE0"));
    #endif
    //设置工作模式
    json_array_append_new(sta_cmd, json_string("AT+CWMODE_CUR=1"));
    //AT+CWJAP_CUR=设置master WiFi的名称和密码
    char cmd_string[128] = "AT+CWJAP_CUR=\"";
    string_append(cmd_string, (char *)json_string_value(json_object_get(data, "ssid")));
    string_append(cmd_string, "\",\"");
    string_append(cmd_string, (char *)json_string_value(json_object_get(data, "passwd")));
    string_append(cmd_string, "\"");
    json_array_append_new(sta_cmd, json_string(cmd_string));
    //设置本机IP地址
    cmd_string[0]=0;
    string_append(cmd_string, "AT+CIPSTA_CUR=\"");
    string_append(cmd_string, (char *)json_string_value(json_object_get(data, "ip")));
    string_append(cmd_string, "\"");
    json_array_append_new(sta_cmd, json_string(cmd_string));
    //设置多连接
    cmd_string[0]=0;
    string_append(cmd_string, "AT+CIPMUX=1");
    json_array_append_new(sta_cmd, json_string(cmd_string));
    //设置开启TCP服务器
    cmd_string[0]=0;
    string_append(cmd_string, "AT+CIPSERVER=1,1037");
    json_array_append_new(sta_cmd, json_string(cmd_string));
    //设置超时时间
    cmd_string[0]=0;
    string_append(cmd_string, "AT+CIPSTO=300");
    json_array_append_new(sta_cmd, json_string(cmd_string));
    
    
    json_t *sta_data = json_object();
    //添加sta_cmd到sta_data
    json_object_set_new(sta_data, "sta_cmd", sta_cmd);
    //设置master主机的IP地址
    cmd_string[0]=0;
    string_append(cmd_string, (char *)json_string_value(json_object_get(data, "master_ip")));
    json_object_set_new(sta_data, "master_ip", json_string(cmd_string));
    //设置master主机的ssid, 用于扫描
    cmd_string[0]=0;
    string_append(cmd_string, (char *)json_string_value(json_object_get(data, "ssid")));
    json_object_set_new(sta_data, "master_ssid", json_string(cmd_string));
    
    
    //写入Flash外存
    char *sta_mode_at_string = json_dumps(sta_data, JSON_ENSURE_ASCII);
    u16 size = 0;
    for(size=0;*(sta_mode_at_string+size);size++);
    size++;
    u8 size_8[3];
    size_8[0]=(u8)(size>>8);
    size_8[1]=(u8)size;
    size_8[2]=ESP8266_WORK_MODE_STA;
    
    W25QXX_Write((u8*)(size_8+2),2,1);
    W25QXX_Write((u8*)size_8,4096*2,2);
    W25QXX_Write((u8*)sta_mode_at_string,4096*2+100,size);
    
    //销毁Json数据
    free(sta_mode_at_string);
    json_decref(data);
    json_decref(sta_cmd);
    json_decref(sta_data);
    
    //改变工作状态
    work_mode_change();
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
            ap_sta_mode_set_0(json_data, linkId, esp8266_work_status);
            return_code = 0;
            break;
        case 1 :
            ap_sta_mode_set_1(json_data, linkId, esp8266_work_status);
            return_code = 1;
            break;
        case 129 :
            send_ack_message_above_128(192, "init succeed, work on AP master mode", linkId, esp8266_work_status);
            return_code = 129;
            break;
        default :
            send_ack_message_above_128(254, "unknown dataCode", linkId, esp8266_work_status);
            return_code = 254;
    }
    
    //销毁Json数据
    json_decref(json_data);
    
    return return_code;
}

