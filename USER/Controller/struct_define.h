#ifndef __STRUCT_DEFINE_H
#define __STRUCT_DEFINE_H

/*
*******************************************************************************
*   功能说明: 是否开启串口调试模式
*               0: 关闭
*               1: 开启
*               若开启串口调试模式，串口一连接到上位机，串口二连接到ESP8266
*               上位机发往串口一的数据都将转发到ESP8266
*               ESP8266返回的所有数据都将转发到串口一
*
*******************************************************************************
*/
#define SERIAL_DEBUG_MODE       1       //是否开启串口调试模式


/*
*******************************************************************************
*   功能说明: 由于WiFi模块一次返回多行字符串，使用循环队列缓存每行字符串
*             使用固定数组长度，避免内存碎片而出错。目前占用2415字节(2019年8月1日 19点43分)
*
*   结    构: head                                    :(u8    )队列头标记
*             rear                                    :(u8    )队列尾标记
*             tag                                     :(u8    )队列当前长度
*             WIFI_REC_DATA_LEN[WIFI_DATA_LEN]        :(u16   )队列中每条接收缓存的字节长度
*             WIFI_RX_BUF[WIFI_DATA_LEN][WIFI_REC_LEN]:(u8    )队列中存放的数据
*
*******************************************************************************
*/
#define WIFI_REC_LEN  			400  	//定义最大接收字节数 200
#define WIFI_DATA_LEN           6       //定义最大指令缓存数
typedef struct WIFI_REC_LIST
{
    u8 head;        //队列头部
    u8 rear;        //队列尾部
    u8 count;       //队列的长度
    u16 WIFI_REC_DATA_LEN[WIFI_DATA_LEN];           //队列中每个指令的长度WIFI_REC_DATA_LEN[WIFI_DATA_LEN]&0x3fff，同时标记该行指令的接收状态
                                                    //第一位WIFI_REC_DATA_LEN[WIFI_DATA_LEN]&0x8000表示接收到'\n'，接收完成
                                                    //第二位WIFI_REC_DATA_LEN[WIFI_DATA_LEN]&0x4000表示接收到'\r'
    u8 WIFI_RX_BUF[WIFI_DATA_LEN][WIFI_REC_LEN];    //指令缓存
}WIFI_DATA;

/*
*******************************************************************************
*   功能说明: 此结构用于邮件通信tcp_ack_handle_start传输的数据结构
*             是否开启ESP8266返回值解析处理
*   
*   结    构: flag                                   :(u8    )是否开启ESP8266返回值解析处理
*             ack[ESP8266_ACK_STRING_LENGTH]         :(u8    )期待的ESP8266返回值(备用)
*
*******************************************************************************
*/
#define ESP8266_ACK_STRING_LENGTH   40
typedef struct REQUEST_TCP_ACK_HANDLE_MAILBOX{
    u8 flag;
    u8 ack[ESP8266_ACK_STRING_LENGTH];
} request_ack_handle;

/*
*******************************************************************************
*   功能说明: 此结构用于邮件通信tcp_ack_handle_start传输的数据结构
*             是否开启ESP8266返回值解析处理
*   
*   结    构: flag                                  :(u8    )ESP8266返回的数据类型，值为TCP_ACK_**宏定义
*             linkID                                :(u8    )若是TCP连接相关的ACK，则包含有link id信息(没有则不管)
*             data[WIFI_REC_LEN]                    :(u8    )若是TCP数据(即+IPD,*)，则把有效JSON待解析字符串存入
*
*******************************************************************************
*/
typedef struct TCP_ACK_DATA_MAILBOX{
    u8 flag;
    u8 linkID;
    u8 data[WIFI_REC_LEN];
} ack_data;

/*
*********************************************************************************************************
*                                             TCP ACK CODES
*********************************************************************************************************
*/
#define TCP_ACK_OK                      1u
#define TCP_ACK_TCP_CONNECT             2u
#define TCP_ACK_TCP_CLOSED              3U
#define TCP_ACK_IPD                     4U
#define TCP_ACK_SEND_CONFIRM            5U












#endif
