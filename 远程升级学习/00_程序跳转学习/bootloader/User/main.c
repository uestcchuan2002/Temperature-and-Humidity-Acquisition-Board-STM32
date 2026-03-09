#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./USMART/usmart.h"
#include "./BSP/KEY/key.h"
#include "./BSP/STMFLASH/stmflash.h"
#include "bootloader.h"
#include "./BSP/24CXX/24C02.h"


IWDG_HandleTypeDef IWDG_Handler; //独立看门狗句柄

/**
 * @breif       初始化独立看门狗 时间计算(大概):Tout=((4*2^prer)*rlr)/32 (ms).
 * @param       prer:分频数:IWDG_PRESCALER_4~IWDG_PRESCALER_256
 * @param       rlr:自动重装载值,0~0XFFF.
 * @retval      无
 */
void IWDG_Init(uint8_t prer,uint16_t rlr)
{
    IWDG_Handler.Instance=IWDG;
    IWDG_Handler.Init.Prescaler=prer;	//设置IWDG分频系数
    IWDG_Handler.Init.Reload=rlr;		//重装载值
    HAL_IWDG_Init(&IWDG_Handler);		//初始化IWDG,默认会开启独立看门狗	
}
    
/**
 * @breif       独立看门狗喂狗
 * @param       无
 * @retval      无
 */
void IWDG_Feed(void)
{   
    HAL_IWDG_Refresh(&IWDG_Handler); 	//喂狗
}


int main(void)
{

    HAL_Init();                             /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7);     /* 设置时钟,168Mhz */
    delay_init(168);                        /* 延时初始化 */
    usart_init(115200);                     /* 串口初始化为115200 */
    usmart_dev.init(84);                    /* 初始化USMART */
    led_init();                             /* 初始化LED */
    lcd_init();                             /* 初始化LCD */
    key_init();                             /* 初始化按键 */
	
	AT24CXX_Init();				    		/* 初始化IIC */	
	while(AT24CXX_Check())
	{
		printf("检测不到24c02!!!\r\n");		/* 检测不到24c02 */
		delay_ms(1000);
	}
	
	IWDG_Init(IWDG_PRESCALER_256,2000);		/* 延迟喂狗时间为8s */
	IWDG_Feed();							/* 立即喂狗 */
	
//	bootInfoStruct_Init();
//	bootInfoStruct_storageTo24C02();
//	bootInfoStruct_readFrom24C02();
//	bootInfoStruct_printf();
	
    lcd_show_string(30,  20, 200, 16, 24, "STM32_OTA_TEST", RED);
    lcd_show_string(30,  70, 200, 16, 24, "bootloader", BLUE);

    while (1)
    {
		lcd_show_string(30, 120, 200, 16, 24, "waiting for jump...", BLUE);
		delay_ms(1000);
		bootloader_start();
    }
}


