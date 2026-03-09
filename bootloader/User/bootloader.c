#include "bootloader.h"

boot_info_t boot_info;		/* 升级存储信息 */

/**
 * @brief       初始化升级存储信息结构体，第一次使用
 * @param       无
 * @retval      无
 */
void bootInfoStruct_Init(void)
{
	boot_info.magic = 0x5A5A5A5A;
	boot_info.active_part = 0x0a;
	boot_info.target_part = 0x0a;
	boot_info.state = BOOT_NORMAL;
	boot_info.boot_count = 0;
	/* crc校验暂无 */
}

/**
 * @brief       打印boot_info的信息
 * @param       无
 * @retval      无
 */
void bootInfoStruct_printf(void)
{
	printf("魔术字为:0x%X\r\n", boot_info.magic);
	
	if (boot_info.active_part == 0x0a) {
		printf("活跃APP为: APPA\r\n");
	} else {
		printf("活跃APP为: APPB\r\n");
	}
	
	if (boot_info.target_part == 0x0a) {
		printf("目标APP为: APPA\r\n");
	} else {
		printf("目标APP为: APPB\r\n");
	}

	switch (boot_info.state) {
		case BOOT_NORMAL:
			printf("升级状态为: 正常启动\r\n");
		break;
		case BOOT_SWITCH_REQUEST:
			printf("升级状态为: 请求切换分区\r\n");
		break;
		case BOOT_TESTING:
			printf("升级状态为: 新版本试运行\r\n");
		break;
	}
	
	printf("当前启动次数为:%d\r\n", boot_info.boot_count);
}

/**
 * @brief       读取boot_info结构体信息
 * @param       无
 * @retval      无
 */
void bootInfoStruct_readFrom24C02(void)
{
	AT24CXX_Read(255 - sizeof(boot_info_t), (u8 *)&boot_info, sizeof(boot_info_t));
}

/**
 * @brief       存储boot_info结构体信息
 * @param       无
 * @retval      无
 */
void bootInfoStruct_storageTo24C02(void)
{
	AT24CXX_Write(255 - 12,(u8 *)&boot_info,sizeof(boot_info_t));
}

/**
 * @brief   跳转到指定 APP 地址执行（Bootloader → Application）
 * @param   App_Addr  应用程序起始地址（即应用程序向量表起始地址）
 * @note
 *          Cortex-M 启动时会：
 *          1. 读取向量表第 1 个字作为 MSP
 *          2. 读取向量表第 2 个字作为 Reset_Handler 地址
 *          本函数手动模拟一次上电复位过程，实现安全跳转。
 */
void IAP_ExecuteApp(uint32_t App_Addr)
{
    uint32_t appStack = *(uint32_t*)App_Addr;
    uint32_t appEntry = *(uint32_t*)(App_Addr + 4);

    if ((appStack & 0x2FFE0000) == 0x20000000)
    {
        void (*app_reset_handler)(void);

        __disable_irq();

        /* 1. 关闭 SysTick */
        SysTick->CTRL = 0;
        SysTick->LOAD = 0;
        SysTick->VAL  = 0;

        /* 2. 关闭所有中断 */
        for (int i = 0; i < 8; i++)
        {
            NVIC->ICER[i] = 0xFFFFFFFF;
            NVIC->ICPR[i] = 0xFFFFFFFF;
        }

        /* 3. 重定位向量表 */
        SCB->VTOR = App_Addr;

        __DSB();
        __ISB();

        /* 4. 设置主栈 */
        __set_MSP(appStack);

        /* 5. 跳转 */
        app_reset_handler = (void (*)(void))appEntry;
        app_reset_handler();
    }
}

/**
 * @brief       bootloader启动函数
 * @param       无
 * @retval      无
 */
void bootloader_start(void)
{
	/* 1.上电读取boot_info */
	bootInfoStruct_readFrom24C02();	
	
	/* 2.打印boot_info */
	bootInfoStruct_printf();
	
	/* 3.判读是不是初次上电 */
	if (boot_info.magic != 0x5a5a5a5a) {	/* 如果是第一次上电，则需进行boot_init初始化 */
		bootInfoStruct_Init();
		bootInfoStruct_storageTo24C02();
	}
	
	
	/* 4.根据状态进行程序跳转 */
	switch(boot_info.state) 
	{
		case BOOT_NORMAL:
			/* 双区跳转测试 */
//			boot_info.active_part = 0x0b;
		
			/* 正常模式下，读取活跃app，进行跳转 */
			if (boot_info.active_part == 0x0a) {
	
				IAP_ExecuteApp(APPA_ADDR);		
			} else {
				IAP_ExecuteApp(APPB_ADDR);	
			}
			break;
		case BOOT_SWITCH_REQUEST:
			/*
				校验 target: 
					成功->state = TESTING
					失败->state = NORMAL
			*/
			// 校验
			
			boot_info.state = BOOT_TESTING;
			boot_info.active_part = boot_info.target_part;
			bootInfoStruct_storageTo24C02();
		
			if (boot_info.active_part == 0x0a) {	
				IAP_ExecuteApp(APPA_ADDR);		
			} else {
				IAP_ExecuteApp(APPB_ADDR);	
			}
			
			break;
		case BOOT_TESTING:
			boot_info.boot_count++;
			// 如何测试状态超过三次，则进行回滚
			if (boot_info.boot_count >= 3) {
				// 回滚
				printf("程序回滚...\r\n");
				boot_info.active_part = boot_info.active_part == 0x0a ? 0x0b : 0x0a;
				boot_info.state = BOOT_NORMAL;
				boot_info.boot_count = 0;
			}
			
			bootInfoStruct_storageTo24C02();
			
			if (boot_info.active_part == 0x0a) {	
				IAP_ExecuteApp(APPA_ADDR);		
			} else {
				IAP_ExecuteApp(APPB_ADDR);	
			}
			break;
	}
}

