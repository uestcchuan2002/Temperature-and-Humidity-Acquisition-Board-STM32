/******************************************************************************
 * @file    system_config.h
 * @brief   系统参数配置结构定义（用于Flash存储）
 * @note
 *  1. 所有参数统一存储在 system_config_t 中
 *  2. 结构体必须保持固定长度
 *  3. 最后4字节为CRC32校验值
 *  4. 修改结构体必须同步更新 version
 ******************************************************************************/

#ifndef __SYSTEM_CONFIG_H
#define __SYSTEM_CONFIG_H

#include <stdint.h>
#include "./BSP/24CXX/24C02.h"
#include "./SYSTEM/usart/usart.h"

/* 最大通道数量 */
#define MAX_CHANNEL    16

/******************************************************************************
 * @brief 网络配置参数
 ******************************************************************************/
typedef struct
{
    uint8_t ip[4];            /**< 本地IP地址，例如 192.168.1.100 */
    uint8_t subnet[4];        /**< 子网掩码 */
    uint8_t gateway[4];       /**< 网关地址 */

    uint16_t port;            /**< TCP端口号 */
    uint8_t  tcp_mode;        /**< TCP模式: 0=Client, 1=Server */

} network_config_t;


/******************************************************************************
 * @brief 单通道参数配置
 ******************************************************************************/
typedef struct
{
    uint8_t temp_high;          /**< 温度上限报警值 */
    uint8_t temp_low;           /**< 温度下限报警值 */

    uint8_t humi_high;          /**< 湿度上限报警值 */
    uint8_t humi_low;           /**< 湿度下限报警值 */

    uint16_t alarm_delay_s;   /**< 报警延迟时间(秒) */

} channel_config_t;


/******************************************************************************
 * @brief 采集系统配置
 ******************************************************************************/
typedef struct
{
    uint16_t sample_interval_ms;   /**< 采样周期(ms) */

    uint8_t  filter_window;        /**< 滑动平均滤波窗口大小 */
    uint8_t  median_enable;        /**< 中值滤波使能: 0=关闭, 1=启用 */

    uint16_t channel_open; /**< 多通道开启参数 */

} acquisition_config_t;


/******************************************************************************
 * @brief 系统总配置结构（Flash存储结构）
 *
 * @note
 *  1. 必须保证 crc32 为结构体最后一个成员
 *  2. CRC计算时不包含 crc32 字段
 *  3. reserve 预留用于未来扩展
 ******************************************************************************/
typedef struct
{
    char version[16];          /**< 配置版本号 */
    uint32_t length;           /**< 结构体长度(sizeof(system_config_t)) */

    network_config_t     network;       /**< 网络参数 */
	channel_config_t  	 channel;		/**< 通道参数 */	
    acquisition_config_t acquisition;   /**< 采集参数 */
	
} system_config_t;

extern system_config_t system_config;

void system_config_init(void);
void system_config_to_storage(system_config_t system_config);
void system_config_to_printf(void);
void system_config_to_version(char *version);

#endif
