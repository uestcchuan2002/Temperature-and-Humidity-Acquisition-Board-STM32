#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/24CXX/24C02.h"

#define APPA_ADDR	0x0800C000			/* APPA区起始地址 */
#define APPB_ADDR	0x08080000			/* APPB区起始地址 */

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

void bootInfoStruct_Init(void);					/* 初始化升级存储信息结构体 */
void bootInfoStruct_printf(void);				/* 打印boot_info的信息 */
void bootInfoStruct_readFrom24C02(void);		/* 读取boot_info结构体信息 */
void bootInfoStruct_storageTo24C02(void);		/* 存储boot_info结构体信息 */

void IAP_ExecuteApp(uint32_t App_Addr);			/* 跳转至指定APP执行 */
void bootloader_start(void);					/* bootloader启动 */
#endif
