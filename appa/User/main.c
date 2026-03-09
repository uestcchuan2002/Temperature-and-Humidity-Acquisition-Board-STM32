#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/SENSOR_RS485/sensor.h"
#include "./BSP/RTC/rtc.h"
#include "./BSP/IWDG/iwdg.h"
#include "./BSP/KEY/key.h"
#include "./BSP/SRAM/sram.h"
#include "./MALLOC/malloc.h"
#include "./BSP/24CXX/24C02.h"
#include "system_config.h"
#include "freertos_demo.h"

int main(void)
{
	
    HAL_Init();                         /* 初始化HAL库 */
	
    sys_stm32_clock_init(336, 8, 2, 7); /* 设置时钟,168Mhz */
    delay_init(168);                    /* 延时初始化 */
    usart_init(115200);                 /* 串口初始化为115200 */
    led_init();                         /* 初始化LED */
    lcd_init();                         /* 初始化LCD */
	
    key_init();                         /* 初始化按键 */
    sram_init();                        /* SRAM初始化 */
    
    my_mem_init(SRAMIN);                /* 初始化内部SRAM内存池 */
    my_mem_init(SRAMEX);                /* 初始化外部SRAM内存池 */
    my_mem_init(SRAMCCM);               /* 初始化内部CCM内存池 */
	
	sensor_hw_init(115200);				/* 传感器初始化 */
	RTC_Init();							/* RTC初始化 */
	IWDG_Init(IWDG_PRESCALER_256,3000); /* 分频数为256,重载值为1000,溢出时间为4s */
	IWDG_Feed();
	AT24CXX_Init();				    	/* 初始化IIC */	
	while(AT24CXX_Check())
	{
		printf("can not check 24c02!!!\r\n");	/* 检测不到24c02 */
		delay_ms(1000);
	}

	system_config_to_printf();			/* 打印系统配置 */
	printf("this is app B\r\n");
	lcd_show_string(30,  170, 200, 40, 32, "APPB", BLUE);
    Ota_OprationAfterJump();            /* OTA跳转后操作 */

    freertos_demo();                    /* 创建lwIP的任务函数 */
	
	while (1) 
    {
		printf("error: can open task StartScheduler");
        delay_ms(2000);
	}
}
