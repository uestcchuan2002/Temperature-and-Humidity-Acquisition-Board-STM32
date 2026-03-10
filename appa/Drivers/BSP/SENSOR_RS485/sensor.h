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
    uint8_t device_id;                              // 设备ID
    float temperature;                              // 温度值
    float humidity;                                 // 湿度值
    uint8_t time[6];                                // 时间数组（时/分/秒/年/月/日，可根据实际定义调整）
} __attribute__((packed)) SensorData_t;

/* ===================== 缓冲区大小定义 ===================== */
#define USART3_RX_BUF_LEN   9                       // USART3接收缓冲区长度
#define USART3_TX_BUF_LEN   256                     // USART3发送缓冲区长度

/* ===================== 全局变量声明 ===================== */
extern UART_HandleTypeDef huart3;                   // USART3句柄
extern DMA_HandleTypeDef hdma_usart3_rx;            // USART3接收DMA句柄
extern DMA_HandleTypeDef hdma_usart3_tx;            // USART3发送DMA句柄
extern uint8_t usart3_rx_buf[USART3_RX_BUF_LEN];    // USART3接收缓冲区
extern uint8_t usart3_tx_buf[USART3_TX_BUF_LEN];    // USART3发送缓冲区
extern uint16_t usart3_rx_len;                      // USART3接收数据长度

/* ===================== 函数声明 ===================== */
void USART3_DMA_Init(uint32_t baudrate); 			    // USART3+DMA初始化
void USART3_DMA_SendData(uint8_t *pData, uint16_t len); // DMA发送数据
void USART3_Parse_Data(uint8_t *pData, uint16_t len);   // 数据解析（解析传感器接收的数据）
void Error_Handler(void);               			    // 错误处理函数
int sensor_hw_init(uint32_t baudrate);                  // 传感器硬件初始化
void USART3_Process_Task(void *argument);               // USART3数据处理任务（FreeRTOS任务）
void USART3_Send_Commend_Task(void *argument);          // USART3指令发送任务（FreeRTOS任务）

#endif
