#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdint.h>
#include <stdio.h>
#include <lwip/sockets.h>
#include "./BSP/LCD/lcd.h"
#include "./MALLOC/malloc.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/api.h"
#include "lwip_demo.h"
#include "ota.h"

/* 设置目标IP地址 */
// #define DEST_IP_ADDR0               192
// #define DEST_IP_ADDR1               168
// #define DEST_IP_ADDR2                 1
// #define DEST_IP_ADDR3               167

/* 需要自行设置目标IP地址 */
// #define IP_ADDR   "192.168.1.167"

#define LWIP_DEMO_RX_BUFSIZE 1460                     /* 最大接收数据长度 */
#define LWIP_DEMO_PORT 8080                          /* 连接的本地端口号 */
#define LWIP_SEND_THREAD_PRIO (tskIDLE_PRIORITY + 3) /* 数据发送线程优先级 */

/* 接收数据缓存数组 */
uint8_t g_lwip_demo_recvbuf[LWIP_DEMO_RX_BUFSIZE];
/* 数据发送缓冲区 */
uint8_t g_lwip_demo_sendbuf[] = "ALIENTEK DATA \r\n";

/* 数据发送标志位 */
uint8_t g_lwip_send_flag;
int g_sock_conn; /* 请求的socket描述符 */
int g_lwip_connect_state = 0;
static void lwip_send_thread(void *arg);
extern QueueHandle_t g_display_queue; /* 显示消息队列句柄 */

/**
 * @brief       发送数据线程创建函数
 * @param       无
 * @retval      无
 */
void lwip_data_send(void)
{
    sys_thread_new("lwip_send_thread", lwip_send_thread, NULL, 512, LWIP_SEND_THREAD_PRIO);
}

/**
 * @brief       lwip_demo测试入口函数
 * @param       无
 * @retval      无
 */
void lwip_demo(void)
{
    struct sockaddr_in server_addr; /* 服务器地址结构体 */
    struct sockaddr_in conn_addr;   /* 连接地址结构体 */
    socklen_t addr_len;             /* 地址长度 */
    int err;
    int length;
    int sock_fd;
    char *tbuf;
    
    lwip_data_send(); /* 创建一个发送线程 */

    sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); /* 建立一个新的socket连接 */
    memset(&server_addr, 0, sizeof(server_addr));        /* 将服务器地址清空 */
    server_addr.sin_family = AF_INET;                    /* 地址族：IPv4 */
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);     /* 注意转换为网络字节序，监听所有本地地址 */
    server_addr.sin_port = htons(LWIP_DEMO_PORT);        /* 使用指定的本地端口号 */

    tbuf = mymalloc(SRAMIN, 200);                     /* 申请内存 */
    sprintf((char *)tbuf, "Port:%d", LWIP_DEMO_PORT); /* 客户端端口号 */
    lcd_show_string(5, 150, 200, 16, 16, tbuf, BLUE);

    err = bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)); /* 建立绑定 */

    if (err < 0) /* 如果绑定失败则关闭套接字 */
    {
        closesocket(sock_fd); /* 关闭套接字 */
        myfree(SRAMIN, tbuf);
    }

    err = listen(sock_fd, 4); /* 监听连接请求，最大挂起连接数4 */

    if (err < 0) /* 如果监听失败则关闭套接字 */
    {
        closesocket(sock_fd); /* 关闭套接字 */
    }

    while (1)
    {
        g_lwip_connect_state = 0;
        addr_len = sizeof(struct sockaddr_in); /* 将连接地址长度赋值给addr_len */

        /* 对监听的请求进行连接，返回值赋给sock_conn */
        g_sock_conn = accept(sock_fd, (struct sockaddr *)&conn_addr, &addr_len);

        /* 返回值小于0表示连接失败，及时关闭套接字 */
        if (g_sock_conn < 0)
        {
            closesocket(sock_fd);
        }
        else
        {
            /* 客户端连接成功 */
            lcd_show_string(5, 90, 200, 16, 16, "State:Connection Successful", BLUE);
            g_lwip_connect_state = 1;
        }

        while (1)
        {
            memset(g_lwip_demo_recvbuf, 0, LWIP_DEMO_RX_BUFSIZE);
            /* 将接收到的数据放到接收缓冲区 */
            length = recv(g_sock_conn, g_lwip_demo_recvbuf, sizeof(g_lwip_demo_recvbuf), 0);

            if (length <= 0)
            {
                goto atk_exit;
            }

            /* 1.将接收到的数据封装成OTA数据包，并发送到OTA队列 */
            ota_packet_t pkt;
            if (length > sizeof(pkt.data)) {
                length = sizeof(pkt.data);
            }
            pkt.length = length;
            memcpy(pkt.data, g_lwip_demo_recvbuf, length);
            if (xQueueSend(xOtaQueue, &pkt, portMAX_DELAY) != pdPASS)
            {
                printf("Failed to send OTA packet to queue!\r\n");
            }

            /* 2.将接收到的数据发送到消息显示队列 */
//            void *buf_ptr = g_lwip_demo_recvbuf; /* 取缓存区地址 */
//            lwip_err = xQueueSend(g_display_queue, &buf_ptr, 0);
//            if (lwip_err == errQUEUE_FULL)
//            {
//                printf("Queue Key_Queue is full, data sending failed!\r\n");
//            }
        }
    atk_exit:
        if (g_sock_conn >= 0)
        {
            closesocket(g_sock_conn);
            g_sock_conn = -1;
            lcd_fill(5, 89, lcddev.width, 110, WHITE);
            lcd_show_string(5, 90, 200, 16, 16, "State:Disconnect", BLUE);
            myfree(SRAMIN, tbuf);
        }
    }
}

/**
 * @brief       发送数据线程函数
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
static void lwip_send_thread(void *pvParameters)
{
    pvParameters = pvParameters;

    while (1)
    {
        /* 有数据需要发送 且 处于连接状态 */
        if (((g_lwip_send_flag & LWIP_SEND_DATA) == LWIP_SEND_DATA) && (g_lwip_connect_state == 1))
        {
            send(g_sock_conn, g_lwip_demo_sendbuf, sizeof(g_lwip_demo_sendbuf), 0); /* 发送数据 */
            g_lwip_send_flag &= ~LWIP_SEND_DATA;
        }

        vTaskDelay(100); /* 延时100ms，降低CPU占用 */
    }
}
