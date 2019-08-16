#ifndef __WORK_GROUP_STATUS_H
#define __WORK_GROUP_STATUS_H

#include "sys.h"

#define WORK_GROUP_STATUS_LENGTH_ADDR   0x5000
#define WORK_GROUP_STATUS_ADDR          0x5100

void EQUIPMENT_QUERY_KEY_init(void);

//初始化工作组记录表
void work_group_init(void);

//更新和获取"network_data"
void work_group_network_update(char *ssid, char *passwd);
//获取网络组的SSID和passwd
char* get_work_group_ssid(char ssid[]);
char* get_work_group_passwd(char passwd[]);

//设置和获取设备在网络组中的equipment_code
void equipment_identification_set(u8 equipment_code);
u8 equipment_identification_get(void);

//更新网络组的某台设备的IP地址
void update_work_group_equipment_x_ip(u8 equipment_code, char *ip);

//获取网络组的某台设备的ip地址
char* get_work_group_equipment_x_ip(u8 equipment_code, char ip[]);

//打印工作组记录表到串口一
void work_group_printf(void);

#endif
