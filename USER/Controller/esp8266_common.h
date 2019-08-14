#ifndef __ESP8266_COMMON_H
#define __ESP8266_COMMON_H

#include <jansson.h>
#include "sys.h"
#include "struct_define.h"

//finish
void ESP8266_ack_handle_on(void);
void ESP8266_ack_handle_off(void);
void send_cmd_json_2_ESP8266(json_t *string);
u8 send_data_2_tcp_link(json_t *data, u8 linkId);
void TCP_ACK_TCP_CLOSED_handle(ack_data *ack_data_recieve, ESP8266_WORK_STATUS_DEF *esp8266_work_status);
void TCP_ACK_TCP_CONNECT_handle(ack_data *ack_data_recieve, ESP8266_WORK_STATUS_DEF *esp8266_work_status);
void esp8266_tcp_link_close(u8 lindId, ESP8266_WORK_STATUS_DEF *esp8266_work_status);

void send_at_cmd_json_stream(u8 work_mode);

//todo
u8 send_data_2_ipaddr(json_t *data, char *ipaddr);


#endif
