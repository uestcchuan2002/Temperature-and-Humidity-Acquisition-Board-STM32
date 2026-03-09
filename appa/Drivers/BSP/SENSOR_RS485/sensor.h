#ifndef __SENSOR_H
#define __SENSOR_H

#include "stm32f4xx_hal.h"
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/RTC/rtc.h"
#include "freertos_demo.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"



/* =================== 传感器数据结构体 =================== */
typedef struct {
    float temperature;
    float humidity;
    uint8_t time[6]; 
} SensorData_t;



/* ===================== 缓冲区大小 ===================== */
#define USART3_RX_BUF_LEN   9
#define USART3_TX_BUF_LEN   256

/* ===================== 全局变量声明 ===================== */
extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;
extern uint8_t usart3_rx_buf[USART3_RX_BUF_LEN];
extern uint8_t usart3_tx_buf[USART3_TX_BUF_LEN];
extern uint16_t usart3_rx_len;

/* ===================== 函数声明 ===================== */
void USART3_DMA_Init(uint32_t baudrate); 				// USART3+DMA初始化
void USART3_DMA_SendData(uint8_t *pData, uint16_t len); // DMA发送
void USART3_Parse_Data(uint8_t *pData, uint16_t len);   // 数据解析
void Error_Handler(void);               				// 错误处理
int sensor_hw_init(uint32_t baudrate);
void USART3_Process_Task(void *argument);
void USART3_Send_Commend_Task(void *argument);
#endif
