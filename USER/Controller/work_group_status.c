#include "work_group_status.h"
#include <jansson.h>
#include "w25qxx.h"
#include "struct_define.h"

char EQUIPMENT_QUERY_KEY[MAX_EQUIPMENT+1][8];

void EQUIPMENT_QUERY_KEY_init(void)
{
    for(u8 i=0;i<MAX_EQUIPMENT+1;i++) { EQUIPMENT_QUERY_KEY[i][0] = 0; }
    string_append(EQUIPMENT_QUERY_KEY[EQUIPMENT_MASTER], "MASTER");
    for(u8 i=EQUIPMENT_SLAVE_1;i<MAX_EQUIPMENT+1;i++) { string_append(EQUIPMENT_QUERY_KEY[i], "SLAVE_"); }
    for(u8 i=EQUIPMENT_SLAVE_1;i<MAX_EQUIPMENT+1;i++) 
    { 
        EQUIPMENT_QUERY_KEY[i][6]= i -1 + '0'; 
        EQUIPMENT_QUERY_KEY[i][7]= 0; 
    }
} 

/*
*********************************************************************************************************
*    函 数 名: work_group_write_update
*
*    功能说明: 更新数据到flash
*
*    形    参: void
*
*    返 回 值: void
*********************************************************************************************************
*/
void work_group_write_update(json_t *work_group)
{
    //写入Flash外存
    char *work_group_data_string = json_dumps(work_group, JSON_ENCODE_ANY);
    u16 length = 0;
    for(;*(work_group_data_string+length);length++);
    length++;
    u8 length_8[2];
    length_8[0]=(u8)(length>>8);
    length_8[1]=(u8)length;
    
    W25QXX_Write((u8*)length_8, WORK_GROUP_STATUS_LENGTH_ADDR, 2);
    W25QXX_Write((u8*)work_group_data_string, WORK_GROUP_STATUS_ADDR, length);
    
    free(work_group_data_string);
}

/*
*********************************************************************************************************
*    函 数 名: get_work_group
*
*    功能说明: 获取work group数据，用户需要手动释放返回的json格式数据内存
*
*    形    参: void
*
*    返 回 值: json_t *
*********************************************************************************************************
*/
json_t * get_work_group(void)
{
    //读出flash数据
    u8 length_8[2];
    W25QXX_Read(length_8, WORK_GROUP_STATUS_LENGTH_ADDR, 2);
    u16 length;
    length = ((u16)length_8[0]<<8) + length_8[1];
    u8 work_group_data_string[length];
    W25QXX_Read(work_group_data_string, WORK_GROUP_STATUS_ADDR, length);
    
    //加载flash数据为Json格式
    json_error_t json_error;
    json_t *work_group;
    work_group = json_loads((char*)work_group_data_string, JSON_DECODE_ANY, &json_error);
    
    return work_group;
}

/*
*********************************************************************************************************
*    函 数 名: work_group_init
*
*    功能说明: 建立工作状态记录表
*              {
*                   "network_data":{
*                       "ssid": "wifi_name",
*                       "passwd": "password"
*                   },
*                   "equipment_identification":{
*                       "id":"2"
*                   }
*                   EQUIPMENT_KEY_MASTER:{
*                       "ip": "192.168.16.1"
*                   },
*                   EQUIPMENT_KEY_SLAVE_1:{
*                       "ip": "192.168.16.123"
*                   },
*                   EQUIPMENT_KEY_SLAVE_2:{
*                       "ip": "192.168.16.121"
*                   }
*               }
*
*    形    参: void
*
*    返 回 值: void
*********************************************************************************************************
*/
void work_group_init()
{
    EQUIPMENT_QUERY_KEY_init();
    
    json_t *work_group=json_object(), *network_data=json_object(),
           *equipment_identification=json_object(),*equipment_data[MAX_EQUIPMENT];
    
    json_object_set_new(network_data, "ssid", json_string(""));
    json_object_set_new(network_data, "passwd", json_string(""));
    json_object_set_new(work_group, "network_data", network_data);
    
    json_object_set_new(equipment_identification, "id", json_string(""));
    json_object_set_new(work_group, "equipment_identification", equipment_identification);
    
    for(u8 i=EQUIPMENT_MASTER-1;i<=MAX_EQUIPMENT-1;i++)
    {
        equipment_data[i]=json_object();
        json_object_set_new(equipment_data[i], "ip", json_string(""));
    }
    
    for(u8 i=EQUIPMENT_MASTER;i<=MAX_EQUIPMENT;i++)
    {
        json_object_set_new(work_group, EQUIPMENT_QUERY_KEY[i], equipment_data[i-1]);
    }
    
    
    //更新到Flash
    work_group_write_update(work_group);
    
    //释放内存
    for(u8 i=EQUIPMENT_MASTER-1;i<=MAX_EQUIPMENT-1;i++)
    {
        json_decref(equipment_data[i]);
    }
    json_decref(equipment_identification);
    json_decref(network_data);
    json_decref(work_group);
}

/*
*********************************************************************************************************
*    函 数 名: work_group_network_update
*
*    功能说明: 更新"network_data"
*
*    形    参: char *ssid, char *passwd
*
*    返 回 值: void
*********************************************************************************************************
*/
void work_group_network_update(char *ssid, char *passwd)
{
    json_t *work_group = get_work_group();
    
    json_t *network_data = json_object_get(work_group, "network_data");
    json_object_set(network_data, "ssid", json_string(ssid));
    json_object_set(network_data, "passwd", json_string(passwd));
    
    
    json_object_set(work_group, "network_data", network_data);
    
    work_group_write_update(work_group);
    json_decref(work_group);
    json_decref(network_data);
}

/*
*********************************************************************************************************
*    函 数 名: get_work_group_ssid
*
*    功能说明: 获取网络组的SSID
*
*    形    参: char *ssid, char *passwd
*
*    返 回 值: char* ssid
*********************************************************************************************************
*/
char* get_work_group_ssid(char ssid[])
{
    json_t *work_group = get_work_group();
    json_t *network_data = json_object_get(work_group, "network_data");
    
    const char *ssid_reference = json_string_value(json_object_get(network_data, "ssid"));
    
    u8 i=0;
    for(;*(ssid_reference+i);i++)
    {
        ssid[i] = *(ssid_reference+i);
    }
    ssid[i] = 0;
    
    json_decref(work_group);
    json_decref(network_data);
    
    return ssid;
}

/*
*********************************************************************************************************
*    函 数 名: get_work_group_passwd
*
*    功能说明: 获取网络组的SSID
*
*    形    参: char *ssid, char *passwd
*
*    返 回 值: char* ssid
*********************************************************************************************************
*/
char* get_work_group_passwd(char passwd[])
{
    json_t *work_group = get_work_group();
    json_t *network_data = json_object_get(work_group, "network_data");
    
    const char *ssid_reference = json_string_value(json_object_get(network_data, "passwd"));
    
    u8 i=0;
    for(;*(ssid_reference+i);i++)
    {
        passwd[i] = *(ssid_reference+i);
    }
    passwd[i] = 0;
    
    json_decref(work_group);
    json_decref(network_data);
    
    return passwd;
}

/*
*********************************************************************************************************
*    函 数 名: update_work_group_equipment_x_ip
*
*    功能说明: 更新equipment的IP地址
*
*    形    参: u8 equipment_code, char *ip
*
*    返 回 值: void
*********************************************************************************************************
*/
void update_work_group_equipment_x_ip(u8 equipment_code, char *ip)
{
    json_t *work_group = get_work_group();
    
    json_t *equipment_data = json_object_get(work_group, EQUIPMENT_QUERY_KEY[equipment_code]);
    json_object_set(equipment_data, "ip", json_string(ip));
    
    json_object_set(work_group, EQUIPMENT_QUERY_KEY[equipment_code], equipment_data);
    
    work_group_write_update(work_group);
    json_decref(work_group);
    json_decref(equipment_data);
}

/*
*********************************************************************************************************
*    函 数 名: get_work_group_equipment_x_ip
*
*    功能说明: 获取网络组的某台设备的ip地址
*
*    形    参: char *equipment_query_key, char ip[]
*
*    返 回 值: char* ip
*********************************************************************************************************
*/
char* get_work_group_equipment_x_ip(u8 equipment_code, char ip[])
{
    json_t *work_group = get_work_group();
    json_t *equipment_data = json_object_get(work_group, EQUIPMENT_QUERY_KEY[equipment_code]);
    
    const char *ip_reference = json_string_value(json_object_get(equipment_data, "ip"));
    
    u8 i=0;
    for(;*(ip_reference+i);i++)
    {
        ip[i] = *(ip_reference+i);
    }
    ip[i] = 0;
    
    json_decref(work_group);
    json_decref(equipment_data);
    
    return ip;
}

/*
*********************************************************************************************************
*    函 数 名: equipment_identification_set
*
*    功能说明: 设置当前设备位于工作组中的ID
*
*    形    参: u8 equipment_code
*
*    返 回 值: void
*********************************************************************************************************
*/
void equipment_identification_set(u8 equipment_code)
{
    json_t *work_group = get_work_group();
    json_t *equipment_identification = json_object_get(work_group, "equipment_identification");
    
    char equipment_code_str[2]="";
    u16_2_string(equipment_code, equipment_code_str);
    json_object_set(equipment_identification, "id", json_string(equipment_code_str));
    
    json_object_set(work_group, "equipment_identification", equipment_identification);
    
    work_group_write_update(work_group);
    json_decref(work_group);
    json_decref(equipment_identification);
}

/*
*********************************************************************************************************
*    函 数 名: equipment_identification_set
*
*    功能说明: 设置当前设备位于工作组中的ID
*
*    形    参: void
*
*    返 回 值: u8 equipment_code
*********************************************************************************************************
*/
u8 equipment_identification_get(void)
{
    json_t *work_group = get_work_group();
    json_t *equipment_identification = json_object_get(work_group, "equipment_identification");
    
    const char *id_reference = json_string_value(json_object_get(equipment_identification, "id"));
    u8 equipment_code = string_2_u16((char *)id_reference);
    
    json_decref(work_group);
    json_decref(equipment_identification);
    
    return equipment_code;
}

/*
*********************************************************************************************************
*    函 数 名: work_group_printf
*
*    功能说明: //打印工作组记录表到串口一
*
*    形    参: void
*
*    返 回 值: void
*********************************************************************************************************
*/
void work_group_printf(void)
{
    json_t *work_group = get_work_group();
    
    char *work_group_str = json_dumps(work_group, JSON_ENCODE_ANY);
    printf("work_group:%s\r\n", work_group_str);
    
    free(work_group_str);
    json_decref(work_group);
}

