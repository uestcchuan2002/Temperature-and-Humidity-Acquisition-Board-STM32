#include "./BSP/SENSOR_RS485/sensor.h"
#include <string.h>
#include <stdio.h>

/* ===================== 宏定义 ===================== */


/* ===================== 全局变量 ===================== */
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart3_rx;
DMA_HandleTypeDef hdma_usart3_tx;

uint8_t  usart3_rx_buf[USART3_RX_BUF_LEN];
uint8_t  usart3_tx_buf[USART3_TX_BUF_LEN];

uint16_t usart3_rx_len = 0;

uint8_t modbus_read_cmd_01[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x02, 0x71, 0xCB};
uint8_t modbus_read_cmd_02[] = {0x02, 0x04, 0x00, 0x00, 0x00, 0x02, 0x71, 0xF8};
uint8_t modbus_read_cmd_03[] = {0x03, 0x04, 0x00, 0x00, 0x00, 0x02, 0x70, 0x29};

/* FreeRTOS */
SemaphoreHandle_t xUSart3_RxDoneSemaphore = NULL;

/* ===================== GPIO ===================== */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin       = GPIO_PIN_10;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_11;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* ===================== DMA ===================== */
static void MX_DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* -------- RX DMA -------- */
    hdma_usart3_rx.Instance                 = DMA1_Stream1;
    hdma_usart3_rx.Init.Channel             = DMA_CHANNEL_4;
    hdma_usart3_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_usart3_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart3_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart3_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart3_rx.Init.Mode 								= DMA_NORMAL;
    hdma_usart3_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_usart3_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_usart3_rx) != HAL_OK)
        Error_Handler();

    __HAL_LINKDMA(&huart3, hdmarx, hdma_usart3_rx);

    /* -------- TX DMA -------- */
    hdma_usart3_tx.Instance                 = DMA1_Stream3;
    hdma_usart3_tx.Init.Channel             = DMA_CHANNEL_4;
    hdma_usart3_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_usart3_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart3_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart3_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart3_tx.Init.Mode                = DMA_NORMAL;
    hdma_usart3_tx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    hdma_usart3_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_usart3_tx) != HAL_OK)
        Error_Handler();

    __HAL_LINKDMA(&huart3, hdmatx, hdma_usart3_tx);

    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);

    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
}

/* ===================== USART3 ===================== */
static void MX_USART3_UART_Init(uint32_t baudrate)
{
    __HAL_RCC_USART3_CLK_ENABLE();

    huart3.Instance          = USART3;
    huart3.Init.BaudRate     = baudrate;
    huart3.Init.WordLength   = UART_WORDLENGTH_8B;
    huart3.Init.StopBits     = UART_STOPBITS_1;
    huart3.Init.Parity       = UART_PARITY_NONE;
    huart3.Init.Mode         = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart3) != HAL_OK)
        Error_Handler();

    /* 开启空闲中断 */
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);

    HAL_NVIC_SetPriority(USART3_IRQn, 6                                                                       , 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
}

/* ===================== 初始化入口 ===================== */
void USART3_DMA_Init(uint32_t baudrate)
{
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART3_UART_Init(baudrate);

    xUSart3_RxDoneSemaphore = xSemaphoreCreateBinary();

    /* 启动循环DMA接收 */
    HAL_UART_Receive_DMA(&huart3, usart3_rx_buf, USART3_RX_BUF_LEN);
}

/* ===================== 发送函数 ===================== */
void USART3_DMA_SendData(uint8_t *pData, uint16_t len)
{
    if (len == 0 || len > USART3_TX_BUF_LEN) return;

    memcpy(usart3_tx_buf, pData, len);
    HAL_UART_Transmit_DMA(&huart3, usart3_tx_buf, len);
}

/* ===================== IDLE中断 ===================== */
void USART3_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart3);

        HAL_UART_DMAStop(&huart3);

        usart3_rx_len = USART3_RX_BUF_LEN -
                        __HAL_DMA_GET_COUNTER(&hdma_usart3_rx);

        if (usart3_rx_len > 0)
        {
            xSemaphoreGiveFromISR(xUSart3_RxDoneSemaphore,
                                  &xHigherPriorityTaskWoken);
        }

        HAL_UART_Receive_DMA(&huart3,
                             usart3_rx_buf,
                             USART3_RX_BUF_LEN);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    HAL_UART_IRQHandler(&huart3);
}

/* ===================== DMA中断 ===================== */
void DMA1_Stream1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart3_rx);
}

void DMA1_Stream3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart3_tx);
}


/* ===================== 传感器发送指令 ===================== */
void USART3_Send_Commend_Task(void *argument) 
{
    uint8_t flag_read_cmd = 1;
	for (;;)
	{
        if (++flag_read_cmd == 4) {
            flag_read_cmd = 1;
        }

        switch (flag_read_cmd)
        {
        case 1:
            USART3_DMA_SendData(modbus_read_cmd_01, 8);
            break;
        case 2:
            USART3_DMA_SendData(modbus_read_cmd_02, 8);
            break;
        case 3:
            USART3_DMA_SendData(modbus_read_cmd_03, 8);
            break;

        default:
            break;
        }
		
		xTaskNotify(IWDG_FRESHTask_Handler, 0x08, eSetBits);
		vTaskDelay(1000);
	}
}

/* ===================== 数据解析 ===================== */
/*
(1)function:crc16校验，用于modbus rtu通信
(2)add:需要校验的数字
(3)num:需要校验的数组长度
*/
uint16_t crc16(uint8_t *addr, uint8_t num) {
	int i,j,temp;
	uint16_t crc=0xFFFF;	
	for(i=0;i<num;i++) {
		crc=crc^(*addr);
		for(j=0;j<8;j++) {
			temp=crc&0x0001;
			crc=crc>>1;
			if(temp) {
				crc=crc^0xA001;
			}
		}
		addr++;
	}
	return crc;
}

void USART3_Parse_Data(uint8_t *pData, uint16_t len)
{
    if (len < 8) return;

    if (pData[1] == 0x04)
    {
		// crc校验
		uint16_t crc16_result;
		crc16_result = crc16(pData, len - 2);
		uint8_t high_bits = ( crc16_result >> 8) & 0xFF; 		//获取crc的高16位
		uint8_t low_bits =  crc16_result & 0xFF;				//获取crc的低16位
		if (high_bits == pData[len - 1] && low_bits == pData[len - 2]) {
			float temp = ((pData[3] << 8) | pData[4]) / 10.0f;
			float humi = ((pData[5] << 8) | pData[6]) / 10.0f;
			printf("温度: %.1f 湿度: %.1f\r\n", temp, humi);
		} else {
			printf("数据crc错误!!!\r\n");
		}        
    }
}

/* ===================== 传感器数据解析任务 ===================== */


void USART3_Process_Task(void *argument)
{
	RTC_TimeTypeDef RTC_TimeStruct;
	RTC_DateTypeDef RTC_DateStruct;
	SensorData_t sensor_pkg;
    for (;;)
    {
		// 1. 等待串口 IDLE 中断释放信号量
        if (xSemaphoreTake(xUSart3_RxDoneSemaphore, portMAX_DELAY) == pdTRUE)
        {

			// 2. 协议解析（校验 CRC）
			if (usart3_rx_len >= 8 && crc16(usart3_rx_buf, usart3_rx_len-2) == 
                ((usart3_rx_buf[usart3_rx_len-1] << 8) | usart3_rx_buf[usart3_rx_len-2])) 
            {
				taskENTER_CRITICAL();
                sensor_pkg.device_id   = usart3_rx_buf[0];
                sensor_pkg.temperature = ((usart3_rx_buf[3] << 8) | usart3_rx_buf[4]) / 10.0f;
                sensor_pkg.humidity    = ((usart3_rx_buf[5] << 8) | usart3_rx_buf[6]) / 10.0f;
				// 记录当前系统时间
				HAL_RTC_GetTime(&RTC_Handler,&RTC_TimeStruct,RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&RTC_Handler,&RTC_DateStruct,RTC_FORMAT_BIN);
				
				sensor_pkg.time[0] = RTC_DateStruct.Year;
				sensor_pkg.time[1] = RTC_DateStruct.Month;
				sensor_pkg.time[2] = RTC_DateStruct.Date;
				sensor_pkg.time[3] = RTC_TimeStruct.Hours;
				sensor_pkg.time[4] = RTC_TimeStruct.Minutes;
				sensor_pkg.time[5] = RTC_TimeStruct.Seconds;
				

                // 3. 将结构体数据推入队列，不等待（因为它是生产者）
                xQueueSend(xSensorDataQueue, &sensor_pkg, 0);
				taskEXIT_CRITICAL();
            }
			
            memset(usart3_rx_buf, 0, USART3_RX_BUF_LEN);
            usart3_rx_len = 0;
        }
    }
}

/* ===================== 错误处理 ===================== */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
			printf("error\r\n");
		}
}

/* ===================== 传感器硬件初始化 ===================== */
int sensor_hw_init(uint32_t baudrate)
{
    USART3_DMA_Init(baudrate);
    return 0;
}
