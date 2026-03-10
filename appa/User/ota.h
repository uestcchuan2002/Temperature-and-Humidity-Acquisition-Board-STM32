#ifndef __OTA_H
#define __OTA_H

#include "./SYSTEM/sys/sys.h"
#include "freertos_demo.h"
#include "string.h"
#include "stm32f4xx_hal_flash_ex.h"
#include <stdint.h>
#include "./BSP/24CXX/24C02.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define OTA_HEAD1 0x55
#define OTA_HEAD2 0xAA
#define OTA_BUFFER_SIZE 4096

#define FLASH_SECTOR_0     0U  /*!< Sector Number 0   */
#define FLASH_SECTOR_1     1U  /*!< Sector Number 1   */
#define FLASH_SECTOR_2     2U  /*!< Sector Number 2   */
#define FLASH_SECTOR_3     3U  /*!< Sector Number 3   */
#define FLASH_SECTOR_4     4U  /*!< Sector Number 4   */
#define FLASH_SECTOR_5     5U  /*!< Sector Number 5   */
#define FLASH_SECTOR_6     6U  /*!< Sector Number 6   */
#define FLASH_SECTOR_7     7U  /*!< Sector Number 7   */
#define FLASH_SECTOR_8     8U  /*!< Sector Number 8   */
#define FLASH_SECTOR_9     9U  /*!< Sector Number 9   */
#define FLASH_SECTOR_10    10U /*!< Sector Number 10  */
#define FLASH_SECTOR_11    11U /*!< Sector Number 11  */

/* FLASH 扇区的起始地址（适配STM32F4系列，1MB Flash） */
#define ADDR_FLASH_SECTOR_0     ((uint32_t )0x08000000)     /* 扇区0起始地址, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t )0x08004000)     /* 扇区1起始地址, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t )0x08008000)     /* 扇区2起始地址, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t )0x0800C000)     /* 扇区3起始地址, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t )0x08010000)     /* 扇区4起始地址, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t )0x08020000)     /* 扇区5起始地址, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t )0x08040000)     /* 扇区6起始地址, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t )0x08060000)     /* 扇区7起始地址, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t )0x08080000)     /* 扇区8起始地址, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t )0x080A0000)     /* 扇区9起始地址, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t )0x080C0000)     /* 扇区10起始地址, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t )0x080E0000)     /* 扇区11起始地址, 128 Kbytes */

/* OTA双分区地址定义 */
#define APPA_ADDR	0x0800C000			/* APPA分区起始地址（对应扇区3） */
#define APPB_ADDR	0x08080000			/* APPB分区起始地址（对应扇区8） */

/* OTA队列配置 */
#define OTA_QUEUE_DEPTH      8               /* OTA消息队列深度 */
#define OTA_PACKET_MAX_SIZE  1024            /* 单个OTA数据包最大长度（字节） */
#define OTA_SEND_QUEUE_DEPTH      8          /* OTA消息队列深度 */
#define OTA_SEND_MAX_SIZE  128              /* 单个OTA数据包最大长度（字节） */

/* OTA指令定义 */
#define OTA_CMD_START   0xA0                /* OTA升级开始指令 */
#define OTA_CMD_DATA    0xA1                /* OTA数据传输指令 */
#define OTA_CMD_END     0xA2                /* OTA升级结束指令 */
#define OTA_CMD_ACK     0x55                /* OTA应答指令 */

typedef enum				/* 启动状态机 */
{
	BOOT_NORMAL = 0,		/* 正常启动 */
	BOOT_SWITCH_REQUEST,    /* 请求切换分区 */
    BOOT_TESTING,           /* 新版本试运行 */
} boot_state_t;

#pragma pack(1)				/* 1字节对齐 */
typedef struct
{
    uint32_t magic;           /* 魔术字 */
    uint8_t  active_part;     /* 当前活动分区 A/B */
    uint8_t  target_part;     /* 目标分区 */
    uint8_t  state;           /* boot_state_t */
    uint8_t  boot_count;      /* 测试启动次数 */
    uint32_t app_crc;		  /* APP的crc校验 */
} boot_info_t;
#pragma pack()

extern boot_info_t boot_info;
extern uint8_t flag_ota_state_testing;
extern uint8_t ack[3];

/**
 * @brief OTA数据包结构体
 * @note  用于FreeRTOS队列传输OTA数据
 */
typedef struct
{
    uint16_t length;                        /* 实际数据长度 */
    uint8_t  data[OTA_PACKET_MAX_SIZE];     /* 数据缓冲区 */
} ota_packet_t;

/* 外部声明OTA消息队列句柄 */
extern QueueHandle_t  xOtaQueue;
extern SemaphoreHandle_t xOtaSendSemaphore;

uint16_t CRC16_Modbus(uint8_t *data, uint16_t len); /* CRC16校验函数声明 */
int Erase_flash_sector(uint32_t sector_start_addr, uint32_t sector_num); /* Flash扇区擦除函数声明 */
void WriteFlash(uint32_t offset, uint8_t *data, uint16_t len); /* Flash写入函数声明 */
void Ota_Start(uint32_t firmware_size); /* OTA开始处理函数声明 */
void Ota_ProcessPacket(uint8_t *data, uint16_t len); /* OTA数据处理函数声明 */
void IAP_ExecuteApp(uint32_t App_Addr);	// OTA程序跳转
void Ota_OprationAfterJump(void); /* OTA跳转后操作函数声明 */
uint16_t CRC16_Modbus(uint8_t *data, uint16_t len); /* OTA CRC16校验函数声明 */
void bootInfoStruct_storageTo24C02(void); /* 存储boot_info结构体信息到24C02函数声明 */
void bootInfoStruct_readFrom24C02(void); /* 从24C02读取boot_info结构体信息函数声明 */
#endif // !__OTA_H
