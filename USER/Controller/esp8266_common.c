#include "esp8266_common.h"
#include "struct_define.h"
#include "delay.h"
#include "task_create.h"
#include "w25qxx.h"
#include "error_handle.h"

//回车换行符
u8 enter[2] = {0x0d, 0x0a};

/**
*********************************************************************************************************
*    函 数 名: esp8266_ack_handle_on/esp8266_ack_handle_off
*
*    功能说明: 使能/关闭esp8266返回字符串解析
*
*    形    参: 无
*
*    返 回 值: 无
*
*********************************************************************************************************
*/
request_ack_handle handle_ack_message_send;
//开启esp8266返回字符串解析
void ESP8266_ack_handle_on(void)
{
    //开启esp8266
    handle_ack_message_send.ack_string_decode=1;
    OSMboxPost(tcp_ack_handle_start,(void*)&handle_ack_message_send);//发送消息
    delay_ms(15);//给低优先级程序CPU时间接收消息
}
//关闭esp8266返回字符串解析
void ESP8266_ack_handle_off(void)
{
    handle_ack_message_send.ack_string_decode=0;
    OSMboxPost(tcp_ack_handle_start,(void*)&handle_ack_message_send);//发送消息
    delay_ms(15);//给低优先级程序CPU时间接收消息
}


/*
*********************************************************************************************************
*    函 数 名: send_cmd_string_2_usart2
*    功能说明: 向串口2发送命令字符串,结束标志为字符串结束标志0x00（硬件层实现）
*    形    参: const char *cmd_string（字符串头指针，结束标志0x00）
*    返 回 值: 无
*********************************************************************************************************
*/
void send_cmd_string_2_usart2(char *cmd_string)
{
    for(u8 i=0;*(cmd_string+i);i++)
    {
        USART_SendData(USART2, *(cmd_string+i));//向串口2发送数据
        while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);//等待发送结束
    }
    for(u8 i=0;i<2;i++)
    {
        USART_SendData(USART2, enter[i]);//向串口2发送数据
        while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);//等待发送结束
    }
}


/*
*********************************************************************************************************
*    函 数 名: send_cmd_json_2_ESP8266
*
*    功能说明: 处理一条固定格式的json_t格式的AT指令
*
*    形    参: json_t*（string）格式
*
*    返 回 值: 无
*********************************************************************************************************
*/
void send_cmd_json_2_ESP8266(json_t *string)
{
    /*
     **********************************************************************************
     *  json_string_length()返回值是字符串长度，不包含包括string类型最后的结束标识符0x00;
     *    如"ATE0"[0123]，返回值是4
     *  json_string_value()返回值是字符串，包含string类型最后的0x00;
     *    如"ATE0"，返回值是char[5]={"A", "T", "E", "0", 0x00};
     **********************************************************************************
     */
    u8 cmd_char_length = json_string_length(string);
    const char *cmd_json_char = json_string_value(string);  //随着函数结束自动释放常量内存
    
    send_cmd_string_2_usart2((char *)cmd_json_char);
    
    ack_data *ack_data_recieve;
    ack_data_recieve = NULL;
    u8 os_mail_read_err;
    for(u8 t=0;t<255;t++)
    {
        delay_ms(20);
        ack_data_recieve = OSMboxPend(tcp_ack_OK_get, 1, &os_mail_read_err);
        if( ack_data_recieve && (ack_data_recieve->ack_type==TCP_ACK_OK) ) {break;}
    }
}


/*
*********************************************************************************************************
*    函 数 名: send_data_2_tcp_link
*
*    功能说明: 发送数据到已建立的TCP链接中，一般仅用于响应请求
*
*    形    参: json_t* data, 待发送的Json数据
*              u8 linkId, 发往数据的通道
*
*    返 回 值: bool, 1-成功, 0-失败并处理错误
*********************************************************************************************************
*/
u8 send_data_2_tcp_link(json_t *data, u8 linkId)
{
    char cmd[] = "AT+CIPSENDEX=*,1024";
    cmd[13] = '0' + linkId;
    
    send_cmd_string_2_usart2(cmd);
    ack_data *ack_data_recieve;
    u8 os_mail_read_err;
    for(u8 t=0;t<10;t++)
    {
        delay_ms(10);
        ack_data_recieve = OSMboxPend(tcp_ack_OK_get, 10, &os_mail_read_err);
        if( ack_data_recieve && (ack_data_recieve->ack_type==TCP_ACK_SEND_CONFIRM) ) 
        {
            char *data_string;
            data_string = json_dumps(data, JSON_ENCODE_ANY);
            send_cmd_string_2_usart2(data_string);
            free(data_string);
            break;
        }
    }
    
    char end[] = "\\0";
    send_cmd_string_2_usart2(end);
    for(u8 t=0;t<100;t++)
    {
        delay_ms(20);
        ack_data_recieve = OSMboxPend(tcp_ack_OK_get, 10, &os_mail_read_err);
        if( ack_data_recieve && (ack_data_recieve->ack_type==TCP_ACK_SEND_OK) ) {break;}
    }
    
    return 1;
}

/*
*********************************************************************************************************
*    函 数 名: send_data_2_ipaddr
*
*    功能说明: 发送数据到某个地址中，一般用于主动发送数据
*
*    形    参: json_t* data, 待发送的Json数据
*              u8 linkId, 发往数据的IP地址
*
*    返 回 值: 无
*********************************************************************************************************
*/
u8 establish_tcp_link(char *ipaddr)
{
    return 1;
}

u8 send_data_2_ipaddr(json_t *data, char *ipaddr)
{
    return 1;
}

/*
*********************************************************************************************************
*    函 数 名: esp8266_tcp_link_close
*
*    功能说明: 关闭某一个TCP连接
*
*    形    参: u8 linkId
*               #define TCP_LINKID_0                0b0000 0001
*               #define TCP_LINKID_1                0b0000 0010
*               #define TCP_LINKID_2                0b0000 0100
*               #define TCP_LINKID_3                0b0000 1000
*               #define TCP_LINKID_4                0b0001 0000
*               #define TCP_LINKID_ALL              0b0001 1111
*
*    返 回 值: 无
*********************************************************************************************************
*/
void esp8266_tcp_link_close_send_cmd(u8 linkId, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    ack_data *ack_data_recieve;
    ack_data_recieve = NULL;
    u8 os_mail_read_err;
    char at_cipclose[] = "AT+CIPCLOSE=*";
    
    at_cipclose[12] = (char)('0' + linkId);
    send_cmd_string_2_usart2(at_cipclose);
    for(u8 t=0;t<10;t++)
    {
        delay_ms(10);
        ack_data_recieve = OSMboxPend(tcp_ack_OK_get, 2, &os_mail_read_err);
        if( ack_data_recieve && (ack_data_recieve->ack_type==TCP_ACK_TCP_CLOSED) )
        {
            TCP_ACK_TCP_CLOSED_handle(ack_data_recieve, esp8266_work_status);
            break;
        }
    }
}
void esp8266_tcp_link_close(u8 lindId, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    if( (lindId&TCP_LINKID_0) && (esp8266_work_status->tcp_status[0][0]) ) 
    {esp8266_tcp_link_close_send_cmd(0, esp8266_work_status);}
    
    if( (lindId&TCP_LINKID_1) && (esp8266_work_status->tcp_status[1][0]) ) 
    {esp8266_tcp_link_close_send_cmd(1, esp8266_work_status);}
    
    if( (lindId&TCP_LINKID_2) && (esp8266_work_status->tcp_status[2][0]) ) 
    {esp8266_tcp_link_close_send_cmd(2, esp8266_work_status);}
    
    if( (lindId&TCP_LINKID_3) && (esp8266_work_status->tcp_status[3][0]) ) 
    {esp8266_tcp_link_close_send_cmd(3, esp8266_work_status);}
    
    if( (lindId&TCP_LINKID_4) && (esp8266_work_status->tcp_status[4][0]) ) 
    {esp8266_tcp_link_close_send_cmd(4, esp8266_work_status);}
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
void TCP_ACK_TCP_CLOSED_handle(ack_data *ack_data_recieve, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    esp8266_work_status->tcp_status[ack_data_recieve->linkID][0]=ESP8266_TCP_UNLINK;
    esp8266_work_status->tcp_status[ack_data_recieve->linkID][1]=EQUIPMENT_INIT;
}

void TCP_ACK_TCP_CONNECT_handle(ack_data *ack_data_recieve, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    esp8266_work_status->tcp_status[ack_data_recieve->linkID][0]=ESP8266_TCP_LINK;
}

/*
*********************************************************************************************************
*    函 数 名:  string_2_u16
*               u16_2_string
*
*    功能说明:  数字字符串转数字
*               数字转数字字符串
*               最大65535
*
*    形    参: char * num_str
*              u16  num, char num_str[]
*
*    返 回 值: u16  num
*              void
*
*********************************************************************************************************
*/
u16 string_2_u16( char *num_str )
{
    u16 num=0;
    for(u8 i=0;*(num_str+i);i++)
    {
        num = num*10 + (u16)(*(num_str+i) - '0');
    }
    return num;
}
void u16_2_string(u16 num, char num_str[])
{
    char num_str_reverse[6];
    u8 i;
    
    if(!num) 
    {
        num_str[0] = '0';
        num_str[1] = 0;
        return;
    }
    
    for(i=0;i<6;i++) { num_str_reverse[i]=0; }
    for(i=0;num;i++)
    {
        num_str_reverse[i] = num%10 + '0';
        num = num/10;
    }
    num_str[i] = 0;
    for(u8 j=i;j>0;j--)
    {
        num_str[i-j] = num_str_reverse[j-1];
    }
}


/*
*********************************************************************************************************
*    函 数 名:  send_at_cmd_json_stream
*
*    功能说明:  为三种工作模式初始化WiFi芯片。
*               读取flash对应区域的json数据，还原成AT指令发往WIFI芯片
*
*    形    参: u8 work_mode 
*               #define ESP8266_WORK_MODE_INIT              0u
*               #define ESP8266_WORK_MODE_AP                1u
*               #define ESP8266_WORK_MODE_STA               2u
*
*    返 回 值: void
*
*********************************************************************************************************
*/
void send_at_cmd_json_stream(u8 work_mode)
{
    u16 ipaddr = work_mode*4096;
    
    //读出flash数据
    u8 lengthTemp[2];
    W25QXX_Read(lengthTemp,ipaddr,2);
    u16 json_char_length;
    json_char_length = ((u16)lengthTemp[0]<<8) + lengthTemp[1];
    u8 json_char[json_char_length];
    W25QXX_Read(json_char,ipaddr+100,json_char_length);
    
    //加载flash数据为Json格式
    json_error_t json_error;
    json_t *json_data, *cmd, *json_cmd;
    json_data = json_loads((char*)json_char, JSON_ENSURE_ASCII, &json_error);
    json_cmd = json_data;
    
    ESP8266_ack_handle_on();
    if(work_mode==ESP8266_WORK_MODE_STA)
    {
        json_cmd = json_object_get(json_data, "sta_cmd");
        json_t *cwlap_json_string = json_object_get(json_data, "master_ssid");
        const char *cwlap = json_string_value(cwlap_json_string);
        char cwlap_full[64] = "";
        //" AT+CWLAP="ssid" "
        string_append(cwlap_full, "AT+CWLAP=\"");
        string_append(cwlap_full, (char*)cwlap);
        string_append(cwlap_full, "\"");
        json_decref(cwlap_json_string);
        
        cwlap_json_string = json_string("AT+CWMODE_CUR=1");
        send_cmd_json_2_ESP8266(cwlap_json_string);
        json_decref(cwlap_json_string);
        
        cwlap_json_string = json_string(cwlap_full);
        u8 *ack_data_find_ssid_recieve;
        ack_data_find_ssid_recieve = NULL;
        u8 os_mail_read_err;
        while(1)
        {
            send_cmd_json_2_ESP8266(cwlap_json_string);               //发送到芯片
            
            delay_ms(50);
            ack_data_find_ssid_recieve = OSMboxPend(find_the_ssid, 1, &os_mail_read_err);
            if( ack_data_find_ssid_recieve && ( (*ack_data_find_ssid_recieve)==FIND_SSID_ACK_CONFIRM) ) 
            {
                *ack_data_find_ssid_recieve = 0;
                break;
            }
            
            delay_ms(500);
        }
        json_decref(cwlap_json_string);
    }
    
    //处理Json包的数据指令
    u8 cmd_quantity;
    if(json_is_array(json_cmd))
    {
        cmd_quantity = json_array_size(json_cmd);
        
        for(u8 i=0; i<cmd_quantity; i++)
        {
            cmd = json_array_get(json_cmd, i);         //获取一条指令
            send_cmd_json_2_ESP8266(cmd);               //发送到芯片
            json_decref(cmd);
        }
        ESP8266_ack_handle_off();
    }else error_handle(1);
    json_decref(json_cmd);
    json_decref(json_data);
}

/*
*********************************************************************************************************
*    函 数 名:  work_mode_change
*
*    功能说明:  发送改变工作状态邮箱，三种工作状态接收到此消息后，退出循环，重新确定工作状态
*
*    形    参: void
*
*    返 回 值: void
*
*********************************************************************************************************
*/
u8 work_mode_change_send;
void work_mode_change(void)
{
    work_mode_change_send = 1;
    OSMboxPost(work_mode_status_change,(void*)&work_mode_change_send);
}





