#include "work_group_online_status.h"
#include "struct_define.h"
#include "esp8266_work_status.h"

typedef struct EQUIPMENT_ONLINE_STATUS_TABLE{
                    u8 status;                  //DROPPED, SUSPEND, READY
                    u8 motor_speed;             //电机速度
                    u8 motor_position;          //电机位置
                    u8 fan;                     //风扇启停
                    u8 offline_interval;        //心跳包间隔数量
} equipment_online_status;

struct WORK_GROUP_ONLINE_TABLE{
                    u8 group_status;            //STOP, SUSPEND, READY
                    equipment_online_status equipment[MAX_EQUIPMENT];
} work_group_online;
/*
*********************************************************************************************************
*    函 数 名: work_group_online_init
*
*    功能说明: 建立工作组在线设备、电机位置和速度、风扇启停、心跳包间隔时长状态记录表
                typedef struct EQUIPMENT_ONLINE_STATUS_TABLE{
                    u8 status;                  //DROPPED, SUSPEND, READY
                    u8 motor_speed;             //电机速度
                    u8 motor_position;          //电机位置
                    u8 fan;                     //风扇启停
                    u8 offline_interval;        //心跳包间隔数量
                }
                typedef struct WORK_GROUP_ONLINE_TABLE{
                    u8 group_status;            //STOP, READY
                    equipment_online_status slave[MAX_EQUIPMENT-1];
                }
                对于从机来说，其他所有设备通过主机设置后，可统一看作是主机
*
*    形    参: void
*
*    返 回 值: void
*********************************************************************************************************
*/
void work_group_online_init(void)
{
    work_group_online.group_status=DROPPED;
    
    for(u8 i=0;i<=MAX_EQUIPMENT-offset;i++)
    {
        work_group_online.equipment[i].status=DROPPED;
        work_group_online.equipment[i].motor_speed=0;
        work_group_online.equipment[i].motor_position=0;
        work_group_online.equipment[i].fan=0;
        work_group_online.equipment[i].offline_interval=0;
    }
}

//设置在线设备状态
void set_work_group_online_equipment_status(u8 equipment_code, u8 status)
{
    equipment_code = equipment_code-offset; //计算偏移量
    work_group_online.equipment[equipment_code].status=status;
    check_work_group_online_status();
}

//检查网络组设备是否准备就绪
u8 check_work_group_online_status(void)
{
    if(get_esp8266_work_status_ap_sta_mode() == ESP8266_WORK_MODE_AP)
    {   
        for(u8 i=0;i<=MAX_EQUIPMENT-offset;i++)
        {
            if(work_group_online.equipment[i].status!=READY)
            {
                work_group_online.group_status=STOP;
                return work_group_online.group_status;
            }
        }
        
        work_group_online.group_status=READY;
        return work_group_online.group_status;
    }
    if(get_esp8266_work_status_ap_sta_mode() == ESP8266_WORK_MODE_STA)
    {   
        if(work_group_online.equipment[EQUIPMENT_MASTER-offset].status!=READY)
        {
            work_group_online.group_status=STOP;
            return work_group_online.group_status;
        }
        if(work_group_online.equipment[get_esp8266_work_status_physical_equipment_id()-offset].status!=READY)
        {
            work_group_online.group_status=STOP;
            return work_group_online.group_status;
        }
        
        work_group_online.group_status=READY;
        return work_group_online.group_status;
    }
    work_group_online.group_status=STOP;
    return work_group_online.group_status;
}


void work_group_online_status_printf(void)
{
    printf("work_group_online.group_status:%d\r\n", work_group_online.group_status);
    for(u8 i=0;i<=MAX_EQUIPMENT-offset;i++) printf("work_group_online.equipment[%d].status:%d\r\n", i, work_group_online.equipment[i].status);
}

