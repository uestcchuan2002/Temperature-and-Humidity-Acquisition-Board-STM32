#include "freertos_demo.h"

/******************************************************************************************************/
/*FreeRTOS配置*/

/* START_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define START_TASK_PRIO 5            /* 任务优先级 */
#define START_STK_SIZE 512           /* 任务堆栈大小 */
TaskHandle_t StartTask_Handler;      /* 任务句柄 */
void start_task(void *pvParameters); /* 任务函数 */

/* LWIP_DEMO 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define LWIP_DMEO_TASK_PRIO 11           /* 任务优先级 */
#define LWIP_DMEO_STK_SIZE 512           /* 任务堆栈大小 */
TaskHandle_t LWIP_Task_Handler;          /* 任务句柄 */
void lwip_demo_task(void *pvParameters); /* 任务函数 */

/* LED_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define LED_TASK_PRIO 10           /* 任务优先级 */
#define LED_STK_SIZE 128           /* 任务堆栈大小 */
TaskHandle_t LEDTask_Handler;      /* 任务句柄 */
void led_task(void *pvParameters); /* 任务函数 */

/* USART3__PROCESS_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define USART3__PROCESS_TASK_PRIO 13          /* 任务优先级 */
#define USART3__PROCESS_STK_SIZE 512          /* 任务堆栈大小 */
TaskHandle_t USART3__PROCESSTask_Handler;     /* 任务句柄 */
void USART3_Process_Task(void *pvParameters); /* 任务函数 */

/* USART3__SEND_COMMEND_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define USART3__SEND_COMMEND_TASK_PRIO 12          /* 任务优先级 */
#define USART3__SEND_COMMEND_STK_SIZE 512          /* 任务堆栈大小 */
TaskHandle_t USART3__SEND_COMMENDTask_Handler;     /* 任务句柄 */
void USART3_Send_Commend_Task(void *pvParameters); /* 任务函数 */

/* SENSOR_DATA_UPLOAD_TASK 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define SENSOR_DATA_UPLOAD_TASK_PRIO 11           /* 任务优先级 */
#define SENSOR_DATA_UPLOAD_TASK_STK_SIZE 512      /* 任务堆栈大小 */
TaskHandle_t SENSOR_DATA_UPLOADTask_Handler;      /* 任务句柄 */
void Sensor_Data_UpData_Task(void *pvParameters); /* 任务函数 */

/* KEY_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define KEY_TASK_PRIO 11           /* 任务优先级 */
#define KEY_STK_SIZE 128           /* 任务堆栈大小 */
TaskHandle_t KEYTask_Handler;      /* 任务句柄 */
void key_task(void *pvParameters); /* 任务函数 */

/* DISPLAY_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define DISPLAY_TASK_PRIO 12           /* 任务优先级 */
#define DISPLAY_STK_SIZE 512           /* 任务堆栈大小 */
TaskHandle_t DISPLAYTask_Handler;      /* 任务句柄 */
void display_task(void *pvParameters); /* 任务函数 */

/* TCP_SEND_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define OTA_TASK_PRIO 10           /* 任务优先级 */
#define OTA_TASK_SIZE 1024         /* 任务堆栈大小 */
TaskHandle_t OTATask_Handler;      /* 任务句柄 */
void ota_task(void *pvParameters); /* 任务函数 */

/* TCP_SEND_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define TCP_SEND_TASK_PRIO 9            /* 任务优先级 */
#define TCP_SEND_TASK_SIZE 1024         /* 任务堆栈大小 */
TaskHandle_t TCP_SENDTask_Handler;      /* 任务句柄 */
void tcp_send_task(void *pvParameters); /* 任务函数 */

/* IWDG_FRESH_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define IWDG_FRESH_TASK_PRIO 6            /* 任务优先级 */
#define IWDG_FRESH_TASK_SIZE 128          /* 任务堆栈大小 */
TaskHandle_t IWDG_FRESHTask_Handler;      /* 任务句柄 */
void iwdg_fresh_task(void *pvParameters); /* 任务函数 */

/* 传感器数据传输队列 配置
 * 包括: 队列句柄 深度 传输内容
 */
QueueHandle_t xSensorDataQueue = NULL;             /* 数据传输队列 */
#define xSensorDataQueue_Deep 20                   /* 数据传输队列深度 */
#define xSensorDataQueue_Size sizeof(SensorData_t) /* 数据传输队列大小 */

/* 显示消息队列的数量 */
#define DISPLAYMSG_Q_NUM 20    /* 显示消息队列的数量 */
QueueHandle_t g_display_queue; /* 显示消息队列句柄 */

/******************************************************************************************************/

/**
 * @breif       加载UI
 * @param       mode :  bit0:0,不加载;1,加载前半部分UI
 *                      bit1:0,不加载;1,加载后半部分UI
 * @retval      无
 */
void lwip_test_ui(uint8_t mode)
{
    uint8_t speed;
    uint8_t buf[30];

    if (mode & 1 << 0)
    {
        lcd_show_string(6, 10, 200, 32, 32, "STM32", DARKBLUE);
        lcd_show_string(6, 40, lcddev.width, 24, 24, "lwIP Ping Test", DARKBLUE);
        lcd_show_string(6, 70, 200, 16, 16, "ATOM@ALIENTEK", DARKBLUE);
    }

    if (mode & 1 << 1)
    {
        lcd_show_string(5, 110, 200, 16, 16, "lwIP Init Successed", MAGENTA);

        if (g_lwipdev.dhcpstatus == 2)
        {
            sprintf((char *)buf, "DHCP IP:%d.%d.%d.%d", g_lwipdev.ip[0], g_lwipdev.ip[1], g_lwipdev.ip[2], g_lwipdev.ip[3]); /* 显示动态IP地址 */
        }
        else
        {
            sprintf((char *)buf, "Static IP:%d.%d.%d.%d", g_lwipdev.ip[0], g_lwipdev.ip[1], g_lwipdev.ip[2], g_lwipdev.ip[3]); /* 打印静态IP地址 */
        }

        lcd_show_string(5, 130, 200, 16, 16, (char *)buf, MAGENTA);

        speed = ethernet_chip_get_speed(); /* 得到网速 */

        if (speed)
        {
            lcd_show_string(5, 150, 200, 16, 16, "Ethernet Speed:100M", MAGENTA);
        }
        else
        {
            lcd_show_string(5, 150, 200, 16, 16, "Ethernet Speed:10M", MAGENTA);
        }
    }
}

/**
 * @breif       freertos_demo
 * @param       无
 * @retval      无
 */
void freertos_demo(void)
{
    printf("Entering freertos_demo...\r\n");
    
    // 检查 VTOR 偏移是否正确
    printf("Current VTOR: 0x%08X\r\n", SCB->VTOR);

    xTaskCreate((TaskFunction_t)start_task,
                (const char *)"start_task",
                (uint16_t)512, // 把 128 改成 512，防止栈溢出
                (void *)NULL,
                (UBaseType_t)START_TASK_PRIO,
                (TaskHandle_t *)&StartTask_Handler);

    printf("Starting Scheduler...\r\n");
    vTaskStartScheduler(); 
    
    // 如果走到这里，说明内存分配失败了
    printf("Scheduler failed!\r\n");
}

/**
 * @brief       start_task
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void start_task(void *pvParameters)
{
    pvParameters = pvParameters;

    g_lwipdev.lwip_display_fn = lwip_test_ui;

    g_lwipdev.lwip_display_fn = lwip_test_ui;
	
    lwip_test_ui(1); /* 加载后前部分UI */
	
    while (lwip_comm_init() != 0)
    {
        lcd_show_string(30, 110, 200, 16, 16, "lwIP Init failed!!", RED);
        delay_ms(500);
        lcd_fill(30, 50, 200 + 30, 50 + 16, WHITE);
        lcd_show_string(30, 110, 200, 16, 16, "Retrying...       ", RED);
        delay_ms(500);
        LED1_TOGGLE();
    }


    while (!ethernet_read_phy(PHY_SR)) /* 检查MCU与PHY芯片是否通信成功 */
    {
        printf("MCU与PHY芯片通信失败,请检查电路或者源码！！！！\r\n");
    }

    while ((g_lwipdev.dhcpstatus != 2) && (g_lwipdev.dhcpstatus != 0XFF)) /* 等待DHCP获取成功/超时溢出 */
    {
        vTaskDelay(5);
    }

    /* 传感器数据传输队列创建 */
    xSensorDataQueue = xQueueCreate(xSensorDataQueue_Deep, xSensorDataQueue_Size);
    if (xSensorDataQueue == NULL)
    {
        printf("Sensor data send queue create failed!\r\n");
        return;
    }
    g_display_queue = xQueueCreate(DISPLAYMSG_Q_NUM, sizeof(void *)); /* 创建消息Message_Queue,队列项长度是200长度 */

    /* OTA传输队列创建 */
    xOtaQueue = xQueueCreate(OTA_QUEUE_DEPTH, sizeof(ota_packet_t));
    if (xOtaQueue == NULL)
    {
        printf("OTA queue create failed!\r\n");
        return;
    }

    /* OTA发送队列创建 */
    xOtaSendSemaphore = xSemaphoreCreateBinary();

    /* 独立看门狗喂狗任务 */
    xTaskCreate((TaskFunction_t)iwdg_fresh_task,
                (const char *)"iwdg_fresh_task",
                (uint16_t)IWDG_FRESH_TASK_SIZE,
                (void *)NULL,
                (UBaseType_t)IWDG_FRESH_TASK_PRIO,
                (TaskHandle_t *)&IWDG_FRESHTask_Handler);

    /* LED测试任务 */
    xTaskCreate((TaskFunction_t)led_task,
                (const char *)"led_task",
                (uint16_t)LED_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)LED_TASK_PRIO,
                (TaskHandle_t *)&LEDTask_Handler);

    /* 串口数据处理任务 */
    xTaskCreate((TaskFunction_t)USART3_Process_Task,
                (const char *)"usart3_process_task",
                (uint16_t)USART3__PROCESS_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)USART3__PROCESS_TASK_PRIO,
                (TaskHandle_t *)&USART3__PROCESSTask_Handler);

    /* 传感器指令发送任务 */
    xTaskCreate((TaskFunction_t)USART3_Send_Commend_Task,
                (const char *)"usart3_send_commend_task",
                (uint16_t)USART3__SEND_COMMEND_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)USART3__SEND_COMMEND_TASK_PRIO,
                (TaskHandle_t *)&USART3__SEND_COMMENDTask_Handler);

    /* 传感器数据上传任务 */
    xTaskCreate((TaskFunction_t)Sensor_Data_UpData_Task,
                (const char *)"sensor_data_updata_task",
                (uint16_t)SENSOR_DATA_UPLOAD_TASK_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)SENSOR_DATA_UPLOAD_TASK_PRIO,
                (TaskHandle_t *)&SENSOR_DATA_UPLOADTask_Handler);

    /* key任务 */
    xTaskCreate((TaskFunction_t)key_task,
                (const char *)"key_task",
                (uint16_t)KEY_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)KEY_TASK_PRIO,
                (TaskHandle_t *)&KEYTask_Handler);

    /* 显示任务 */
    xTaskCreate((TaskFunction_t)display_task,
                (const char *)"display_task",
                (uint16_t)DISPLAY_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)DISPLAY_TASK_PRIO,
                (TaskHandle_t *)&DISPLAYTask_Handler);

    /* 创建lwIP任务 */
    xTaskCreate((TaskFunction_t)lwip_demo_task,
                (const char *)"lwip_demo_task",
                (uint16_t)LWIP_DMEO_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)LWIP_DMEO_TASK_PRIO,
                (TaskHandle_t *)&LWIP_Task_Handler);

    /* 创建tcp send 任务 */
    xTaskCreate((TaskFunction_t)tcp_send_task,
                (const char *)"tcp_send_task",
                (uint16_t)TCP_SEND_TASK_SIZE,
                (void *)NULL,
                (UBaseType_t)TCP_SEND_TASK_PRIO,
                (TaskHandle_t *)&TCP_SENDTask_Handler);

    /* 创建ota任务 */
    xTaskCreate((TaskFunction_t)ota_task,
                (const char *)"ota_task",
                (uint16_t)OTA_TASK_SIZE,
                (void *)NULL,
                (UBaseType_t)OTA_TASK_PRIO,
                (TaskHandle_t *)&OTATask_Handler);

    vTaskDelete(StartTask_Handler); /* 删除开始任务 */
}

/**
 * @brief       lwIP运行例程
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void lwip_demo_task(void *pvParameters)
{
    pvParameters = pvParameters;
    lwip_demo(); /* lwip测试代码 */

    while (1)
    {
        vTaskDelay(5);
    }
}

/**
 * @brief       系统再运行
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void led_task(void *pvParameters)
{
    pvParameters = pvParameters;

    while (1)
    {
        LED1_TOGGLE();

        xTaskNotify(IWDG_FRESHTask_Handler, 0x04, eSetBits); /* 正常运行，发送喂狗通知 */
        vTaskDelay(500);
    }
}

/**
 * @brief       传感器数据上传
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void Sensor_Data_UpData_Task(void *pvParameters)
{
    SensorData_t received_data;
    for (;;)
    {
        // 1. 阻塞等待队列中的数据（无数据时任务挂起，不占用CPU）
        if (xQueueReceive(xSensorDataQueue, &received_data, portMAX_DELAY) == pdTRUE)
        {
            printf("设备[%d] - 温度:%.1f 湿度:%.1f 20%d-%d-%d-%d-%d-%d,\r\n",
                    received_data.device_id, received_data.temperature, received_data.humidity, received_data.time[0], received_data.time[1],
                    received_data.time[2], received_data.time[3], received_data.time[4], received_data.time[5]);
        }
    }
}

/**
 * @brief       key_task
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void key_task(void *pvParameters)
{
    pvParameters = pvParameters;

    uint8_t key;

    while (1)
    {
        key = key_scan(0);

        if (KEY0_PRES == key)
        {
            g_lwip_send_flag |= LWIP_SEND_DATA; /* 标记LWIP有数据要发送 */
        }

        xTaskNotify(IWDG_FRESHTask_Handler, 0x02, eSetBits); /* 正常运行，发送喂狗通知 */

        vTaskDelay(10);
    }
}

/**
 * @brief       显示任务
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void display_task(void *pvParameters)
{
    pvParameters = pvParameters;
    void *recv_ptr;

    while (1)
    {
        if (g_display_queue != NULL)
        {

            if (xQueueReceive(g_display_queue, &recv_ptr, pdMS_TO_TICKS(500)) == pdTRUE)
            {
                // lcd_fill(30, 220, lcddev.width - 1, lcddev.height - 1, WHITE); /* 清上一次数据 */
                /* 显示接收到的数据 */
                // lcd_show_string(30, 220, lcddev.width - 30, lcddev.height - 230, 16, (char *)recv_ptr, RED);
                printf("%s", (char *)recv_ptr);
            }
        }

        // 确保 xTaskNotify 被调用
        xTaskNotify(IWDG_FRESHTask_Handler, 0x01, eSetBits); /* 正常运行，发送喂狗通知 */

        vTaskDelay(5);
    }
}

/**
 * @brief       TCP发送任务
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
static uint8_t send_buf[3] = {0x55, 0xAA, 0xB0};
void tcp_send_task(void *pvParameters)
{
    for (;;)
    {
        // 尝试获取信号量，超时时间10ms
        if (xSemaphoreTake(xOtaSendSemaphore, 0) == pdTRUE)
        {
            int ret = send(g_sock_conn, send_buf, sizeof(send_buf), 0);

            if (ret == sizeof(send_buf)) {
                printf("ACK sent\r\n");
            } else {
                printf("ACK send fail %d\r\n", ret);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/**
 * @brief       OTA任务
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void ota_task(void *pvParameters) /* 任务函数 */
{
    ota_packet_t pkt;

    while (1)
    {
        if (xQueueReceive(xOtaQueue, &pkt, portMAX_DELAY) == pdTRUE)
        {
            Ota_ProcessPacket(pkt.data, pkt.length);
        }
    }
}

uint8_t g_ota_is_running = 0;   // 0: 正常模式, 1: OTA 模式
/**
 * @brief       独立看门狗喂狗任务
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void iwdg_fresh_task(void *pvParameters)
{
    uint32_t task_status = 0;

    const uint32_t REQUIRED_MASK =
        0x01 |  // display
        0x02 |  // key
        0x04 |  // led
        0x08;   // usart

    for (;;)
    {
        xTaskNotifyWait(
            0,
            0,
            &task_status,
            pdMS_TO_TICKS(2000));

        if (g_ota_is_running)
        {
            if (task_status & 0x80)
            {
                IWDG_Feed();
                task_status &= ~0x80;
            }
        }
        else
        {
            if ((task_status & REQUIRED_MASK) == REQUIRED_MASK)
            {
                IWDG_Feed();

                /* 清除任务状态 */
                task_status &= ~REQUIRED_MASK;

                if (flag_ota_state_testing == 1 && boot_info.state == BOOT_TESTING)
                {
                    flag_ota_state_testing = 0;

                    boot_info.state = BOOT_NORMAL;
                    boot_info.boot_count = 0;

                    bootInfoStruct_storageTo24C02();

                    printf("OTA测试通过, 状态更新为 BOOT_NORMAL\r\n");
                    printf("OTA升级结束, 2秒后重启...\r\n");

                    NVIC_SystemReset();
                }
            }
        }
    }
}
