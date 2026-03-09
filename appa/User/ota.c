#include "ota.h"

QueueHandle_t xOtaQueue;
SemaphoreHandle_t xOtaSendSemaphore;

uint8_t ota_rx_buffer[OTA_BUFFER_SIZE];
uint32_t ota_rx_len = 0;
uint32_t ota_firmware_size = 0;
uint32_t ota_received_size = 0;
uint32_t ota_write_offset = 0;
uint8_t ack[3] = {0x55,0xAA,0xB0};

/**
 * @brief       获取某个地址所在的flash扇区
 * @param       addr    : Flash地址（需在Flash有效范围内）
 * @retval      0~11：addr所在的扇区；0xFF：地址超出Flash范围（错误）
 * @note        适配STM32F4系列（其他型号需调整扇区地址宏）
 */
uint8_t stmflash_get_flash_sector(uint32_t addr)
{
    // 增加地址合法性校验（以STM32F407为例，Flash最大地址0x080FFFFF）
    // 需根据实际芯片型号修改Flash起始/结束地址
    #define FLASH_BASE_ADDR    0x08000000 // STM32 Flash起始地址（通用）
    #define FLASH_MAX_ADDR     0x080FFFFF // STM32F407 Flash结束地址（1MB）
    
    if (addr < FLASH_BASE_ADDR || addr > FLASH_MAX_ADDR)
    {
        return 0xFF; // 地址非法
    }

    if (addr < ADDR_FLASH_SECTOR_1) return FLASH_SECTOR_0;
    else if (addr < ADDR_FLASH_SECTOR_2) return FLASH_SECTOR_1;
    else if (addr < ADDR_FLASH_SECTOR_3) return FLASH_SECTOR_2;
    else if (addr < ADDR_FLASH_SECTOR_4) return FLASH_SECTOR_3;
    else if (addr < ADDR_FLASH_SECTOR_5) return FLASH_SECTOR_4;
    else if (addr < ADDR_FLASH_SECTOR_6) return FLASH_SECTOR_5;
    else if (addr < ADDR_FLASH_SECTOR_7) return FLASH_SECTOR_6;
    else if (addr < ADDR_FLASH_SECTOR_8) return FLASH_SECTOR_7;
    else if (addr < ADDR_FLASH_SECTOR_9) return FLASH_SECTOR_8;
    else if (addr < ADDR_FLASH_SECTOR_10) return FLASH_SECTOR_9;
    else if (addr < ADDR_FLASH_SECTOR_11) return FLASH_SECTOR_10;
    return FLASH_SECTOR_11;
}

/**
 * @brief       擦除Flash扇区（修正函数名，匹配实际功能）
 * @param       sector_start_addr  : 擦除起始地址（必须是Flash有效地址）
 * @param       sector_num        : 要擦除的扇区数量（1~12，根据芯片总扇区数调整）
 * @retval      0：擦除成功；非0：擦除失败（返回HAL库错误码）
 * @note        适配STM32F4系列，电压范围默认2.7~3.6V
 */
int Erase_flash_sector(uint32_t sector_start_addr, uint32_t sector_num)
{
    HAL_StatusTypeDef erase_status; // 接收擦除结果
    uint32_t PageError = 0;         // 接收擦除失败的扇区编号

    // 1. 参数校验
    if (sector_num == 0 || sector_num > 12) // STM32F407共12个扇区（0~11）
    {
        return -1; // 扇区数量非法
    }

    // 2. 获取起始扇区并校验地址
    uint8_t start_sector = stmflash_get_flash_sector(sector_start_addr);
    if (start_sector == 0xFF)
    {
        return -2; // 起始地址非法
    }

    // 3. 解锁Flash
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return -3; // 解锁失败
    }

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR); // 清除可能的错误标志
    // 4. 配置擦除参数
    FLASH_EraseInitTypeDef flash_erase_init;
    flash_erase_init.TypeErase    = FLASH_TYPEERASE_SECTORS; // 扇区擦除
    flash_erase_init.Sector       = start_sector;            // 起始扇区
    flash_erase_init.NbSectors    = sector_num;              // 擦除扇区数量
    flash_erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;   // 2.7~3.6V


    // 5. 执行擦除
    erase_status = HAL_FLASHEx_Erase(&flash_erase_init, &PageError);

    // 6. 锁定Flash（无论擦除成功/失败，都要锁定）
    HAL_FLASH_Lock();

    // 7. 返回擦除结果
    if (erase_status == HAL_OK)
    {
        return 0; // 擦除成功
    }
    else
    {
        return erase_status; // 返回HAL错误码（如HAL_ERROR/HAL_BUSY等）
    }
}

/**
 * @brief  按字写入Flash数据（每次写入4字节）
 *
 * @param  offset    相对于APP起始地址的偏移量
 * @param  data      要写入的数据起始地址
 * @param  len       要写入的数据长度（字节）
 * @note   1. 地址必须是4字节对齐；2. 写入前需确保对应扇区已擦除；3. 适配STM32F4系列
 */
void WriteFlash(uint32_t offset, uint8_t *data, uint16_t len)
{
    // 入参合法性校验
    if (data == NULL || len == 0)
    {
        return; // 空指针或长度为0，直接返回
    }
	
	uint32_t addr;
	if (boot_info.active_part == 0x0a) {
		addr = APPB_ADDR + offset;
	} else {
		addr = APPA_ADDR + offset;
	}
    
    uint32_t write_len = len;
    
    HAL_FLASH_Unlock();

    // 循环写入：每次写入4字节（1个Word）
    for (int i = 0; i < len; i += 4)
    {
        // 处理最后不足4字节的情况，避免数组越界
        uint8_t byte1 = (i < write_len) ? data[i] : 0;
        uint8_t byte2 = (i+1 < write_len) ? data[i+1] : 0;
        uint8_t byte3 = (i+2 < write_len) ? data[i+2] : 0;
        uint8_t byte4 = (i+3 < write_len) ? data[i+3] : 0;

        uint32_t word = byte1 | (byte2 << 8) | (byte3 << 16) | (byte4 << 24);

        // 执行Flash写入（字编程）
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, word);

        // 地址偏移4字节（对应1个Word）
        addr += 4;
    }

    HAL_FLASH_Lock();
}


/**
 * @brief  OTA升级CRC16校验函数（Modbus算法）
 * @param  data    要校验的数据起始地址
 * @param  len     要校验的数据长度（字节）
 * @retval   校验结果（16位CRC值）
 */
uint16_t CRC16_Modbus(uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;

    for(uint16_t i = 0; i < len; i++) {
        crc ^= data[i];

        for(uint8_t j = 0; j < 8; j++){
            if(crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

/**
 * @brief  Ota升级发送ACK函数
 * @retval   无
 */
void Ota_SendAck(void)
{
    /* 发送ACK信号量，通知TCP发送任务发送ACK数据包 */
	xSemaphoreGive(xOtaSendSemaphore);
}

/**
 * @brief  Ota升级开始处理函数
 * @param  firmware_size    固件大小（字节）
 * @retval   无
 */
void Ota_Start(uint32_t firmware_size)
{   
    printf("OTA START\r\n");
    // 1. 设置全局标志，接关看门狗
    g_ota_is_running = 1;

    // 2. 挂起所有不相干得任务（防止它们在 Flash 写入时读 Flash 或抢占 CPU）
    vTaskSuspend(LEDTask_Handler);
    vTaskSuspend(DISPLAYTask_Handler);
    vTaskSuspend(USART3__PROCESSTask_Handler);
    vTaskSuspend(USART3__SEND_COMMENDTask_Handler);
    vTaskSuspend(SENSOR_DATA_UPLOADTask_Handler);

    // 3. 初始化OTA相关变量
    ota_firmware_size = firmware_size;
    ota_received_size = 0;
    ota_write_offset = 0;
    printf("Firmware size: %d\r\n", firmware_size);
    printf("Erasing flash sectors...\r\n");

    /* 4. 擦除APPB分区 8 9 10 11 */
	
    int res;
	if (boot_info.active_part == 0x0a) {
		res = Erase_flash_sector(APPB_ADDR, 4);
		printf("擦除appb\r\n");
	} else {
		res = Erase_flash_sector(APPA_ADDR, 5);
		printf("擦除appa\r\n");
	}
		
    if(res != 0) {
        // 如果失败，记得恢复任务并重置标志
        g_ota_is_running = 0;
        // 这里的 res 可能是 -1, -2, -3 或者是 HAL_StatusTypeDef 的枚举值
        printf("OTA START ERASE FAILED, Error Code: %d\r\n", res);
        return;
    }
    printf("Flash erase completed.\r\n");

    /* 5. 通知看门狗任务 */
    xTaskNotify(IWDG_FRESHTask_Handler, 0x80, eSetBits); 

    /* 6. 发送ACK */
    Ota_SendAck();
}

void Ota_End(void)
{
    printf("OTA END\r\n");

    /* 1. 发送ACK */
    Ota_SendAck();
    vTaskDelay(pdMS_TO_TICKS(10)); // 确保ACK发送完成

    /* 2. 软件复位 */
	boot_info.state = BOOT_SWITCH_REQUEST;									// 1.更新状态
	boot_info.target_part = boot_info.active_part == 0x0a ? 0x0b : 0x0a;	// 2.更新目标区域
	bootInfoStruct_storageTo24C02();										// 3.存储
	vTaskDelay(100);
	
    printf("Rebooting to new firmware...\r\n");
    NVIC_SystemReset();
}

/**
 * @brief       ota数据处理函数
 * @param       data : 接收到的数据
 * @param       len : 数据长度
 * @note        数据包格式：
                | Head | CMD | LEN | OFFSET | DATA | CRC |
                | 2B   | 1B  | 2B  | 4B     | N    | 2B  |
 */
void Ota_ProcessPacket(uint8_t *data, uint16_t len)
{
    /* 1. 数据缓存 */
    if (ota_rx_len + len > OTA_BUFFER_SIZE) {
        ota_rx_len = 0; // 防止溢出
        return;
    }

    /* 2.将新数据追加到缓存末尾 */
    memcpy(&ota_rx_buffer[ota_rx_len], data, len);
    ota_rx_len += len;

    /* 3. 数据解析 */
    while (ota_rx_len >= 7)   // 最小包长度（Head1+Head2+CMD+LEN(2B)）
    {
        /* 查找帧头, 直到找到对应的帧头才开始解析 */
        if (ota_rx_buffer[0] != OTA_HEAD1 || ota_rx_buffer[1] != OTA_HEAD2) {
            memmove(ota_rx_buffer, ota_rx_buffer + 1, ota_rx_len - 1);
            ota_rx_len -= 1;
            continue;
        }

        uint8_t cmd = ota_rx_buffer[2];                                 /* 获取命令 */
        uint16_t data_len = ota_rx_buffer[3] | (ota_rx_buffer[4] << 8); /* 获取数据长度 */
        uint16_t packet_len = 5 + data_len + 2;                         /* 计算整个包的长度 */

        /* 数据还没接收完整 */
        if (ota_rx_len < packet_len) {
            break;
        }

        /* CRC校验 */
        uint16_t recv_crc = ota_rx_buffer[packet_len - 2] | (ota_rx_buffer[packet_len - 1] << 8);
        uint16_t calc_crc = CRC16_Modbus(ota_rx_buffer, packet_len - 2);
        if(recv_crc != calc_crc) {
            printf("OTA CRC ERROR\r\n");
            memmove(ota_rx_buffer, ota_rx_buffer + packet_len, ota_rx_len - packet_len);
            ota_rx_len -= packet_len;
            continue;
        }
        // printf("OTA CRC OK\r\n");

        uint8_t *payload = &ota_rx_buffer[5];

        switch (cmd)
        {
            case OTA_CMD_START: /* START */
            {
                if (data_len != 4) {
                    printf("OTA START LEN ERR\r\n");
                    break;
                }

                uint32_t firmware_size = payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24);
                Ota_Start(firmware_size);
            }
            break;
  
            case OTA_CMD_DATA:  /* DATA */
            {
                if (data_len < 4) {
                    printf("OTA DATA LEN ERR\r\n");
                    break;
                }

                uint32_t offset = payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24);
                uint8_t *fw_data = &payload[4];
                uint16_t fw_len = data_len - 4;
                printf("OTA DATA offset:%d len:%d\r\n", offset, fw_len);
                
                // 写入Flash
                WriteFlash(offset, fw_data, fw_len);

                // 喂独立看门狗，通知看门狗任务
                xTaskNotify(IWDG_FRESHTask_Handler, 0x80, eSetBits);

                ota_received_size += fw_len;
                Ota_SendAck();
            }
            break;

            case OTA_CMD_END:   /* END */
            {
                Ota_End();
            }
            break;

            default:
                printf("OTA UNKNOWN CMD %02X\r\n", cmd);
                break;
        }

        /* 移除已经处理的数据 */
        memmove(ota_rx_buffer,
                ota_rx_buffer + packet_len,
                ota_rx_len - packet_len);

        ota_rx_len -= packet_len;
    }
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

boot_info_t boot_info;
uint8_t flag_ota_state_testing = 0; // 0: 不需要进行测试，1: 需要进行测试

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

void Ota_OprationAfterJump(void)
{
    /* 1. 更新OTA状态 */
    bootInfoStruct_readFrom24C02();

    /* 2.根据状态显示信息，并通过按键模拟程序是否正常运行 */
    switch(boot_info.state) {
		case BOOT_NORMAL:
			printf("启动状态为:BOOT_NORMAL\r\n");
			if (boot_info.active_part == 0x0a) {
				printf("程序app_a正常运行中...\r\n");
			} else {
				printf("程序app_b正常运行中...\r\n");
			}
		break;
		
		case BOOT_TESTING:
			printf("启动状态为:BOOT_TESTING, 需要进行测试\r\n");
            flag_ota_state_testing = 1; // 标志需要进行测试-->在看门狗任务中根据测试结果更新状态
		break;
	}
}

