#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./USMART/usmart.h"
#include "./BSP/KEY/key.h"
#include "./BSP/STMFLASH/stmflash.h"
#include "./BSP/24CXX/24C02.h"

typedef enum				/* 启动状态机 */
{
	BOOT_NORMAL = 0,		/* 正常启动 */
	BOOT_SWITCH_REQUEST,    /* 请求切换分区 */
    BOOT_TESTING,           /* 新版本试运行 */
    BOOT_ROLLBACK           /* 回滚 */
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

boot_info_t boot_info;

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

int main(void)
{
	uint8_t key;
	
    HAL_Init();                             /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7);     /* 设置时钟,168Mhz */
    delay_init(168);                        /* 延时初始化 */
    usart_init(115200);                     /* 串口初始化为115200 */
    usmart_dev.init(84);                    /* 初始化USMART */
    led_init();                             /* 初始化LED */
    lcd_init();                             /* 初始化LCD */
    key_init();                             /* 初始化按键 */
    
    lcd_show_string(30,  20, 200, 16, 24, "STM32_OTA_TEST", RED);
    lcd_show_string(30,  70, 200, 16, 32, "APPB", BLUE);
	/* ota升级 */
	
	AT24CXX_Init();				    		/* 初始化IIC */	
	while(AT24CXX_Check())
	{
		printf("检测不到24c02!!!\r\n");		/* 检测不到24c02 */
		delay_ms(1000);
	}
	
	/* 1.读取boot_info */
	bootInfoStruct_readFrom24C02();
	
	switch(boot_info.state) {
		case BOOT_NORMAL:
			lcd_show_string(30,  130, 250, 16, 32, "BOOT_NORMAL", BLUE);
			printf("启动状态为：BOOT_NORMAL\r\n");
			if (boot_info.active_part == 0x0a) {
				printf("程序app_a正常运行中...\r\n");
			} else {
				printf("程序app_b正常运行中...\r\n");
			}
		break;
		
		case BOOT_TESTING:
			lcd_show_string(30,  130, 250, 16, 32, "BOOT_TESTING", BLUE);
			printf("启动状态为：BOOT_TESTING\r\n");
			// 按键模拟程序是否正常运行
			printf("通过按键模拟是否正常运行，KEY1:正常运行， KEY2:不正常运行\r\n");
			while (1) {
				key = key_scan(0);
				if (KEY1_PRES == key) {
					printf("程序正常运行..\r\n");
					boot_info.boot_count = 0;
					boot_info.state = BOOT_NORMAL;
					bootInfoStruct_storageTo24C02();
					break;
				} else if (KEY2_PRES == key) {
					printf("程序不正常运行, 请立即软件复位\r\n");
				}
			}
		break;
	}
	printf("通过按键模拟是否进行远程升级，KEY0进行升级\r\n");
    while (1)
    {
        // 采用外部按键来模拟升级，程序下载
		key = key_scan(0);
		
		if (KEY0_PRES == key) {
			// OTA升级
			boot_info.state = BOOT_SWITCH_REQUEST;									// 1.更新状态
			boot_info.target_part = boot_info.active_part == 0x0a ? 0x0b : 0x0a;	// 2.更新目标区域
			bootInfoStruct_storageTo24C02();										// 3.存储
			printf("程序已更新至A区，软件复位\r\n");									// 4.软件复位
		}
		delay_ms(100);
    }
}


