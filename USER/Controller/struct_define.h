#ifndef __STRUCT_DEFINE_H
#define __STRUCT_DEFINE_H

#include "sys.h" 	
#include "delay.h"
#include "includes.h"
#include <jansson.h>

//常用函数
char * string_append(char data[], char *string);
u16 string_2_u16( char * num_str );
char * u16_2_string(u16 num, char num_str[]);
void work_mode_change(void);

/*
*********************************************************************************************************
*   功能说明: 是否开启串口调试模式
*               0: 关闭
*               1: 开启
*               若开启串口调试模式，串口一连接到上位机，串口二连接到ESP8266
*               上位机发往串口一的数据都将转发到ESP8266
*               ESP8266返回的所有数据都将转发到串口一
*
*********************************************************************************************************
*/
#define SERIAL_DEBUG_MODE       1       //是否开启串口调试模式
#define TCP_DATA_RESEND_COUNT   3       //主动发送TCP消息失败时，最大重发次数

/*
*********************************************************************************************************
*   功能说明: 由于WiFi模块一次返回多行字符串，使用循环队列缓存每行字符串
*             使用固定数组长度，避免内存碎片而出错。目前占用2415字节(2019年8月1日 19点43分)
*
*   结    构: head                                    :(u8    )队列头标记
*             rear                                    :(u8    )队列尾标记
*             tag                                     :(u8    )队列当前长度
*             WIFI_REC_DATA_LEN[WIFI_DATA_LEN]        :(u16   )队列中每条接收缓存的字节长度，第一、二位同时用于标记接收状态
*                                                       - 指令的长度WIFI_REC_DATA_LEN[WIFI_DATA_LEN]&0x3fff
*                                                       - 第一位WIFI_REC_DATA_LEN[WIFI_DATA_LEN]&0x8000表示接收到'\n'，接收完成
*                                                       - 第二位WIFI_REC_DATA_LEN[WIFI_DATA_LEN]&0x4000表示接收到'\r'
*             WIFI_RX_BUF[WIFI_DATA_LEN][WIFI_REC_LEN]:(u8    )队列中存放的数据
*
*********************************************************************************************************
*/
#define WIFI_REC_LEN  			400  	//定义最大接收字节数 200
#define WIFI_DATA_LEN           12      //定义最大指令缓存数
typedef struct WIFI_REC_LIST
{
    u8 head;
    u8 rear;
    u8 count;
    u16 WIFI_REC_DATA_LEN[WIFI_DATA_LEN];
    u8 WIFI_RX_BUF[WIFI_DATA_LEN][WIFI_REC_LEN];
}WIFI_DATA;

/*
*********************************************************************************************************
*   功能说明: 此结构用于邮件通信tcp_ack_handle_start传输的数据结构
*             是否开启ESP8266返回值解析处理
*   
*   结    构: ack_string_decode                      :(u8    )是否开启ESP8266返回值解析处理
*             ack[ESP8266_ACK_STRING_LENGTH]         :(u8    )期待的ESP8266返回值(备用)
*
*********************************************************************************************************
*/
#define ESP8266_ACK_STRING_LENGTH   40
typedef struct REQUEST_TCP_ACK_HANDLE_MAILBOX{
    u8 ack_string_decode;
    u8 ack[ESP8266_ACK_STRING_LENGTH];
} request_ack_handle;

/*
*********************************************************************************************************
*   功能说明: 此结构用于邮件通信tcp_ack_OK_get传输的数据结构
*             初步处理ESP8266的返回值，传递给高优先级程序处理
*   
*   结    构: ack_type                              :(u8    )ESP8266返回的数据类型，值为TCP_ACK_**宏定义
*             linkID                                :(u8    )若是TCP连接相关的ACK，则包含有link id信息(没有则不管)
*             data[WIFI_REC_LEN]                    :(u8    )若是TCP数据(即+IPD,*)，则把有效JSON待解析字符串存入
*
*********************************************************************************************************
*/
typedef struct TCP_ACK_DATA_MAILBOX{
    u8 ack_type;
    u8 linkID;
    u8 data[WIFI_REC_LEN];
} ack_data;

/*
*********************************************************************************************************
*                                             TCP ACK TYPES
*********************************************************************************************************
*/
#define TCP_ACK_USELESS                 0
#define TCP_ACK_OK                      1
#define TCP_ACK_TCP_CONNECT             2
#define TCP_ACK_TCP_CLOSED              3
#define TCP_ACK_IPD                     4
#define TCP_ACK_SEND_CONFIRM            5
#define TCP_ACK_SEND_OK                 6       //"SEND OK"
#define FIND_SSID_ACK_CONFIRM           7
#define TCP_ACK_FAIL                    8
#define TCP_ACK_WIFI_CONNECTED          9


/*
*********************************************************************************************************
*                                     ESP8266_WORK_STATUS CODES
*********************************************************************************************************
*/
/* <!-----------------ESP8266 WORK MODE CODES----------------> */
#define ESP8266_WORK_MODE_INIT              0u
#define ESP8266_WORK_MODE_AP                1u
#define ESP8266_WORK_MODE_STA               2u

/* <!-----------------ESP8266 TCP LINK STATUS CODES----------> */
#define ESP8266_TCP_UNLINK                  0u
#define ESP8266_TCP_LINK                    1u

/* <!-----------------PHYSICAL EQUIPMENT CODES---------------> */
#define MAX_EQUIPMENT                       3u

#define EQUIPMENT_INIT                      0u      //不可更改, 只能加1递增
#define EQUIPMENT_MASTER                    1u
#define EQUIPMENT_SLAVE_1                   2u
#define EQUIPMENT_SLAVE_2                   3u


/*
*********************************************************************************************************
*                                        ESP8266 TCP LINK_ID
*********************************************************************************************************
*/
#define TCP_LINKID_ALL                      5u

//关闭所有TCP连接
#define TCP_SEND_ACK_DATA_CODE_NONE         64u


//工作组在线状态设置
#define DROPPED         0
#define SUSPEND         1
#define READY           2
#define STOP            3

#endif
