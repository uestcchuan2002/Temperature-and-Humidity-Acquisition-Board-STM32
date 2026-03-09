/**
 ****************************************************************************************************
 * @file        freertos_demo.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-01-11
 * @brief       lwIP+FreeRTOS操作系统移植 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 探索者 F407开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */
 
#ifndef __FREERTOS_DEMO_H
#define __FREERTOS_DEMO_H

#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/SENSOR_RS485/sensor.h"
#include "./BSP/KEY/key.h"
#include "./MALLOC/malloc.h"
#include "./BSP/IWDG/iwdg.h"
#include "ota.h"
#include "lwip_comm.h"
#include "lwipopts.h"
#include "lwip_demo.h"
#include "sockets.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

extern uint8_t g_ota_is_running;

extern TaskHandle_t LEDTask_Handler; 
extern TaskHandle_t USART3__PROCESSTask_Handler;
extern TaskHandle_t USART3__SEND_COMMENDTask_Handler;
extern TaskHandle_t SENSOR_DATA_UPLOADTask_Handler;
extern TaskHandle_t KEYTask_Handler;
extern TaskHandle_t DISPLAYTask_Handler;

extern QueueHandle_t xSensorDataQueue; 
extern TaskHandle_t IWDG_FRESHTask_Handler;

void freertos_demo(void);   /* 创建lwIP的任务函数 */

#endif
