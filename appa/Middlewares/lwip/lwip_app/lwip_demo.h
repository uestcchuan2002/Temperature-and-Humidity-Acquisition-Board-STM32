#ifndef _LWIP_DEMO_H
#define _LWIP_DEMO_H
#include "./SYSTEM/sys/sys.h"


#define LWIP_SEND_DATA              0X80    /* 定义有数据发送 */
extern uint8_t g_lwip_send_flag;              /* 数据发送标志位 */
extern int g_sock_conn; 

void lwip_demo(void);

#endif /* _CLIENT_H */
