#ifndef __TCP_JSON_HANDLE_H
#define __TCP_JSON_HANDLE_H

#include "struct_define.h"

u8 TCP_ACK_IPD_handle(ack_data *ack_data_recieve);

void handle_TCP_data_loop_task(void);

#endif
