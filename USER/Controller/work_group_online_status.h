#ifndef __WORK_GROUP_ONLINE_STATUS_H
#define __WORK_GROUP_ONLINE_STATUS_H

#include "sys.h"

#define offset          1
//#define EQUIPMENT_SLAVE_1                   2u---->1u

void work_group_online_status_printf(void);


//网络组在线状态表初始化
void work_group_online_init(void);

//设置网络组在线表某个设备的状态
void set_work_group_online_equipment_status(u8 equipment_code, u8 status);

//获取网络组在线全部设备在线状态
u8 check_work_group_online_status(void);

#endif
