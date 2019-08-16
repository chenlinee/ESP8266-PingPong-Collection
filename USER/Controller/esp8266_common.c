#include "esp8266_common.h"
#include "struct_define.h"
#include "delay.h"
#include "task_create.h"
#include "w25qxx.h"
#include "error_handle.h"
#include "tcp_json_handle.h"
#include "work_group_status.h"

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
    const char *cmd_json_char = json_string_value(string);  //随着string释放而释放常量内存
    send_cmd_string_2_usart2((char *)cmd_json_char);
    
    ack_data *ack_data_recieve;
    ack_data_recieve = NULL;
    u8 os_mail_read_err;
    for(u8 t=0;t<25;t++)
    {
        delay_ms(100);
        ack_data_recieve = OSMboxPend(tcp_ack_OK_get, 10, &os_mail_read_err);
        if( ack_data_recieve && (ack_data_recieve->ack_type==TCP_ACK_OK) ) {break;}
    }
}

//
void send_at_cmd_json_stream_STA(void)
{
    u32 addr = 0x1000*ESP8266_WORK_MODE_STA;
    
    ESP8266_ack_handle_on();
    //读出flash数据
    u8 lengthTemp[2];
    W25QXX_Read(lengthTemp,addr,2);
    u16 json_char_length;
    json_char_length = ((u16)lengthTemp[0]<<8) + lengthTemp[1];
    u8 json_char[json_char_length];
    W25QXX_Read(json_char,addr+0x100,json_char_length);
    
    //加载flash数据为Json格式
    json_error_t json_error;
    json_t *AT_stream_data, *sta_cmd_part, *cmd;
    AT_stream_data = json_loads((char*)json_char, JSON_ENSURE_ASCII, &json_error);
    
    sta_cmd_part = json_object_get(AT_stream_data, "sta_cmd_part1");
    //处理Json包的数据指令
    u8 cmd_quantity;
    if(json_is_array(sta_cmd_part))
    {
        cmd_quantity = json_array_size(sta_cmd_part);
        for(u8 i=0; i<cmd_quantity; i++)
        {
            cmd = json_array_get(sta_cmd_part, i);              //获取一条指令
            send_cmd_json_2_ESP8266(cmd);                       //发送到芯片
            json_decref(cmd);
        }
    }else error_handle(1);
    json_decref(sta_cmd_part);
    
    //连接WiFi
    char at_cwlap[96] = "", ssid[64]="";
    // AT+CWLAP="ssid"
    string_append(at_cwlap, "AT+CWLAP=\"");
    string_append(at_cwlap, get_work_group_ssid(ssid));
    string_append(at_cwlap, "\"");
    u8 *ack_data_find_ssid_recieve;
    ack_data_find_ssid_recieve = NULL;
    u8 os_mail_read_err;
    u8 ssid_find_flag=0;
    for(u16 times=0; times<60; times++)
    {
        send_cmd_string_2_usart2(at_cwlap);                     //发送到芯片
        for(u8 i=0;i<40;i++)
        {
            delay_ms(50);
            ack_data_find_ssid_recieve = OSMboxPend(find_the_ssid, 10, &os_mail_read_err);
            if( ack_data_find_ssid_recieve && ( (*ack_data_find_ssid_recieve)==FIND_SSID_ACK_CONFIRM) ) 
            {
                *ack_data_find_ssid_recieve = 0;
                ssid_find_flag = 1;
                break;
            }
        }
        if(ssid_find_flag) {break;}
        delay_ms(500);
    }
    //系统暂停TODO
    if(!ssid_find_flag)
    {
        printf("can not find the network group ssid");
        return;
    }
    //等待主机开机后，完成初始化
    delay_ms(1000);
    
    char passwd[64]="";
    char at_cwjap[128] = "AT+CWJAP_DEF=\"";
    string_append(at_cwjap, ssid);
    string_append(at_cwjap, "\",\"");
    string_append(at_cwjap, get_work_group_passwd(passwd));
    string_append(at_cwjap, "\"");
    
    ack_data *ack_data_recieve;
    ack_data_recieve = NULL;
    u8 wifi_connect_flag=0;
    for(u16 times=0; times<10; times++)
    {
        send_cmd_string_2_usart2(at_cwjap);                     //发送到芯片
        for(u8 i=0;i<40;i++)
        {
            delay_ms(100);
            ack_data_recieve = OSMboxPend(tcp_ack_OK_get, 10, &os_mail_read_err);
            ack_data_find_ssid_recieve = OSMboxPend(find_the_ssid, 10, &os_mail_read_err);
            if( ack_data_find_ssid_recieve && ( (*ack_data_find_ssid_recieve)==TCP_ACK_WIFI_CONNECTED) ) 
            {
                *ack_data_find_ssid_recieve = 0;
                ssid_find_flag = 1;
                continue;
            }
            if( ack_data_recieve && (ack_data_recieve->ack_type==TCP_ACK_OK) ) 
            {
                wifi_connect_flag=2;
                break;
            }
            
            if( ack_data_recieve && (ack_data_recieve->ack_type==TCP_ACK_FAIL) ) 
            {
                wifi_connect_flag=0;
                break;
            }
        }
        if(wifi_connect_flag) {break;}
        delay_ms(500);
    }
    //系统暂停TODO
    if(!wifi_connect_flag)
    {
        
        return;
    }
    
    sta_cmd_part = json_object_get(AT_stream_data, "sta_cmd_part2");
    //处理Json包的数据指令
    if(json_is_array(sta_cmd_part))
    {
        cmd_quantity = json_array_size(sta_cmd_part);
        for(u8 i=0; i<cmd_quantity; i++)
        {
            cmd = json_array_get(sta_cmd_part, i);              //获取一条指令
            send_cmd_json_2_ESP8266(cmd);                       //发送到芯片
            json_decref(cmd);
        }
    }else error_handle(1);
    json_decref(sta_cmd_part);
    
    json_decref(AT_stream_data);
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
    if(work_mode==ESP8266_WORK_MODE_STA)
    {
        send_at_cmd_json_stream_STA();
    }else
    {
        u32 addr = 0x1000*work_mode;
        
        //读出flash数据
        u8 lengthTemp[2];
        W25QXX_Read(lengthTemp,addr,2);
        u16 json_char_length;
        json_char_length = ((u16)lengthTemp[0]<<8) + lengthTemp[1];
        u8 json_char[json_char_length];
        W25QXX_Read(json_char,addr+0x100,json_char_length);
        
        //加载flash数据为Json格式
        json_error_t json_error;
        json_t *AT_stream_data, *cmd;
        AT_stream_data = json_loads((char*)json_char, JSON_ENSURE_ASCII, &json_error);
        
        //处理Json包的数据指令
        u8 cmd_quantity;
        if(json_is_array(AT_stream_data))
        {
            cmd_quantity = json_array_size(AT_stream_data);
            ESP8266_ack_handle_on();
            for(u8 i=0; i<cmd_quantity; i++)
            {
                cmd = json_array_get(AT_stream_data, i);            //获取一条指令
                send_cmd_json_2_ESP8266(cmd);                       //发送到芯片
                json_decref(cmd);
            }
        }else error_handle(1);
        
        #if SERIAL_DEBUG_MODE
        send_cmd_string_2_usart2("ATE1");
        #else 
        send_cmd_string_2_usart2("ATE0");
        #endif
        
        json_decref(AT_stream_data);
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
    ESP8266_ack_handle_on();
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
*    函 数 名: establish_tcp_link
*
*    功能说明: 向特定IP地址建立TCP连接
*              AT+CIPSTART=<link ID>,<type>,<remote IP>,<remote port>[,<TCP keep alive>]
*
*    形    参: char *ipaddr, IP地址
*
*    返 回 值: ESP8266的linkId, 或者找不到IP的错误返回值(5)
*********************************************************************************************************
*/
u8 establish_tcp_link(char *ipaddr, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    u8 linkId = 5, i;
    for(i=4;i!=0xff;i--)
    {
        if( !esp8266_work_status->tcp_status[i][0] ) {break;}
    }
    if(i==0xff) {return linkId;}
    linkId = i;
    
    char at_cipstart[64] = "", linkId_str[3];
    string_append(at_cipstart, "AT+CIPSTART=");
    string_append(at_cipstart, u16_2_string(linkId, linkId_str));
    string_append(at_cipstart, ",\"TCP\",\"");
    string_append(at_cipstart, ipaddr);
    string_append(at_cipstart, "\",1037");
    
    for(u8 j=0;j<10;j++)
    {
        send_cmd_string_2_usart2(at_cipstart);
        for(u8 t=0;t<10;t++)  
        {
            delay_ms(40);
            if( esp8266_work_status->tcp_status[linkId][0] ) {return linkId;}
        }
    }
    
    return 5;
}
/*
*********************************************************************************************************
*    函 数 名: send_data_2_ipaddr
*
*    功能说明: 发送数据到某个地址中，一般用于主动发送数据
*
*    形    参: json_t* data, 待发送的Json数据
*              char *equipment_query_key, 目的主机PHYSICAL EQUIPMENT QUERY KEY
*              u8 dataCode, Json数据的dataCode
*              u8 ack_dataCode, 期待的返回值dataCode, TCP_SEND_ACK_DATA_CODE_NONE 表示不需要检测返回值
*              ESP8266_WORK_STATUS_DEF *esp8266_work_status, WiFi芯片状态句柄
*
*    返 回 值: bool
*********************************************************************************************************
*/
u8 send_data_2_equipment(json_t *data, u8 equipment_code, u8 dataCode, u8 ack_dataCode, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    json_t *full_data = json_object();
    
    //"id"
    char num_2_str[6];
    u16_2_string(esp8266_work_status->physical_equipment_id, num_2_str);
    json_object_set_new(full_data, "id", json_string(num_2_str));
    //"dataCode"
    json_object_set_new(full_data, "dataCode", json_string(u16_2_string(dataCode, num_2_str)));
    //"data"
    json_object_set_new(full_data, "data", data);
    
    //储存目的主机的IP地址
    char ip[16] = "";
    
    //建立TCP连接
    ESP8266_ack_handle_on();
    u8 linkId = establish_tcp_link(get_work_group_equipment_x_ip(equipment_code, ip), esp8266_work_status);
    if(linkId==5) {return 0;}
    
    //向该linkId发送数据
    ack_data *ack_data_recieve;
    u8 os_mail_read_err, is_success=0;
    //最多重发3次
    for(u8 t=0;t<TCP_DATA_RESEND_COUNT;t++)
    {
        send_data_2_tcp_link(full_data, linkId);
        if( ack_dataCode == TCP_SEND_ACK_DATA_CODE_NONE ) { is_success = 1; break;}
        for(u8 i=0;i<15;i++)
        {
            delay_ms(20);
            ack_data_recieve = OSMboxPend(tcp_ack_OK_get, 10, &os_mail_read_err);
            if( ack_data_recieve && (ack_data_recieve->ack_type==TCP_ACK_IPD) ) 
            {
                //检查返回值
                if(  TCP_ACK_IPD_handle(ack_data_recieve, esp8266_work_status) == ack_dataCode )
                {
                    is_success = 1;
                    break;
                }
            }
        }
        if(is_success) {break;}
    }
    
    esp8266_tcp_link_close(linkId, esp8266_work_status);

    json_decref(full_data);
    if(is_success) {return 1;}
    else {return 0;}
}

/*
*********************************************************************************************************
*    函 数 名: esp8266_tcp_link_close
*
*    功能说明: 关闭某一个TCP连接
*
*    形    参: u8 linkId, 5表示关闭全部连接
*
*    返 回 值: 无
*********************************************************************************************************
*/
void esp8266_tcp_link_close_send_cmd(u8 linkId, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    char at_cipclose[] = "AT+CIPCLOSE=*";
    
    at_cipclose[12] = (char)('0' + linkId);
    send_cmd_string_2_usart2(at_cipclose);
    for(u8 t=0;t<10;t++)
    {
        delay_ms(10);
        if( !esp8266_work_status->tcp_status[linkId][0] ) { break;}
    }
}
void esp8266_tcp_link_close(u8 linkId, ESP8266_WORK_STATUS_DEF *esp8266_work_status)
{
    switch(linkId)
    {
        case 0 :
            while(esp8266_work_status->tcp_status[linkId][0]) 
            {
                esp8266_tcp_link_close_send_cmd(linkId, esp8266_work_status);
                delay_ms(20);
            }
            break;
        case 1 :
            while(esp8266_work_status->tcp_status[linkId][0]) 
            {
                esp8266_tcp_link_close_send_cmd(linkId, esp8266_work_status);
                delay_ms(20);
            }
            break;
        case 2 :
            while(esp8266_work_status->tcp_status[linkId][0]) 
            {
                esp8266_tcp_link_close_send_cmd(linkId, esp8266_work_status);
                delay_ms(20);
            }
            break;
        case 3 :
            while(esp8266_work_status->tcp_status[linkId][0]) 
            {
                esp8266_tcp_link_close_send_cmd(linkId, esp8266_work_status);
                delay_ms(20);
            }
            break;
        case 4 :
            while(esp8266_work_status->tcp_status[linkId][0]) 
            {
                esp8266_tcp_link_close_send_cmd(linkId, esp8266_work_status);
                delay_ms(20);
            }
            break;
        case TCP_LINKID_ALL :
            for(u8 i=0;i<TCP_LINKID_ALL;i++) 
            {
                while(esp8266_work_status->tcp_status[linkId][0]) 
                {
                    esp8266_tcp_link_close_send_cmd(linkId, esp8266_work_status);
                    delay_ms(20);
                }
            }
            break;
    }        
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
char * u16_2_string(u16 num, char num_str[])
{
    char num_str_reverse[6];
    u8 i;
    
    if(!num) 
    {
        num_str[0] = '0';
        num_str[1] = 0;
        return num_str;
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
    return num_str;
}

/*
*********************************************************************************************************
*    函 数 名:  scan_wifi_ssid
*
*    功能说明:  扫描work group网络的SSID，没有扫描到不会退出
*
*    形    参: void
*
*    返 回 值: void
*********************************************************************************************************
*/
void scan_wifi_ssid(void)
{    
    ESP8266_ack_handle_on();
    
    json_t *at_cmd;
    
    at_cmd = json_string("AT+CWMODE_CUR=1");
    send_cmd_json_2_ESP8266(at_cmd);
    json_decref(at_cmd);
    
    char at_cmd_str[96] = "", ssid[64]="";
    // AT+CWLAP="ssid"
    string_append(at_cmd_str, "AT+CWLAP=\"");
    string_append(at_cmd_str, get_work_group_ssid(ssid));
    string_append(at_cmd_str, "\"");
    
    at_cmd = json_string(at_cmd_str);
    u8 *ack_data_find_ssid_recieve;
    ack_data_find_ssid_recieve = NULL;
    u8 os_mail_read_err;
    while(1)
    {
        send_cmd_json_2_ESP8266(at_cmd);               //发送到芯片
        delay_ms(50);
        ack_data_find_ssid_recieve = OSMboxPend(find_the_ssid, 1, &os_mail_read_err);
        if( ack_data_find_ssid_recieve && ( (*ack_data_find_ssid_recieve)==FIND_SSID_ACK_CONFIRM) ) 
        {
            *ack_data_find_ssid_recieve = 0;
            break;
        }
        delay_ms(500);
    }
    json_decref(at_cmd);
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

/*
*********************************************************************************************************
*    函 数 名:  work_mode_init_AT_cmd_write_2_flash
*
*    功能说明:  发送改变工作状态邮箱，三种工作状态接收到此消息后，退出循环，重新确定工作状态
*
*    形    参: void
*
*    返 回 值: void
*
*********************************************************************************************************
*/
void work_mode_init_AT_cmd_write_2_flash(json_t *at_cmd_stream, u8 work_mode)
{
    //写入Flash
    char *ap_mode_at_string = json_dumps(at_cmd_stream, JSON_ENSURE_ASCII);
    u16 size = 0;
    for(size=0;*(ap_mode_at_string+size);size++);
    size++;
    u8 size_8[3];
    size_8[0]=(u8)(size>>8);
    size_8[1]=(u8)size;
    size_8[2]=work_mode;
    
    W25QXX_Write((u8*)(size_8+2),2,1);
    W25QXX_Write((u8*)size_8, work_mode*0x1000, 2);
    W25QXX_Write((u8*)ap_mode_at_string, work_mode*0x1000+0x100,size);
    
    free(ap_mode_at_string);
    
    work_mode_change();
}



