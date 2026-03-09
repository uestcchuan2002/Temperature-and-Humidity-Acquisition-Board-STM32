#include "system_config.h"
#include "string.h"

system_config_t system_config;

/**
 * @brief       系统参数配置初始化
 * @param       无
 * @retval      无
 */
void system_config_init(void)
{
	char version[16] = "版本：1.0.0";
	network_config_t     network_config;
	channel_config_t     channel_config;
	acquisition_config_t acquisition_config;
	
	/* 版本号初始化 */
	strncpy(system_config.version, version, sizeof(system_config.version) - 1);
	system_config.version[sizeof(system_config.version) - 1] = '\0';
	
	/* 结构体长度 */
	system_config.length = sizeof(system_config_t);
	
	/* 网络配置参数 */
    network_config.ip[0] = 192;
    network_config.ip[1] = 168;
    network_config.ip[2] = 1;
    network_config.ip[3] = 30;
	network_config.port = 8080;
	network_config.tcp_mode = 1;
	system_config.network = network_config;
	
	/* 单通道参数配置 */
	channel_config.alarm_delay_s = 60;
	channel_config.temp_high = 35;
	channel_config.temp_low = 20;
	channel_config.humi_high = 60;
	channel_config.humi_low = 30;
	system_config.channel = channel_config;
	
    /* 采集系统配置 */
	acquisition_config.channel_open &= 1111111111111111;
	acquisition_config.filter_window = 10;
	acquisition_config.median_enable = 0;
	acquisition_config.sample_interval_ms = 2000;
	system_config.acquisition = acquisition_config;
}

/**
 * @brief       将系统配置参数存储到24c02中
 * @param       system_config: 系统配置参数结构体
 * @retval      无
 */
void system_config_to_storage(system_config_t system_config)
{
	AT24CXX_Write(0,(u8*)(&system_config), sizeof(system_config_t));
}

/**
 * @brief       打印存储的参数配置
 * @param       无
 * @retval      无
 */
void system_config_to_printf(void)
{
	u8 datatemp[sizeof(system_config_t)];	
	
	/* 1. 读取参数配置 */
	AT24CXX_Read(0, datatemp, sizeof(system_config_t));
	
	/* 2. 格式转换 */
	system_config_t *rev = (system_config_t *)datatemp;
	
	/* 3. 打印存储的参数配置 */
	printf("=====================================\r\n");
	printf("        系统配置参数打印\r\n");
	printf("=====================================\r\n");
	
	/* 基础信息 */
	printf("（1）版本号：%s\r\n", rev->version);
	printf("（2）结构体长度：%d 字节\r\n", rev->length);
	
	/* 网络配置 */
	printf("-------------------------------------\r\n");
	printf("【网络配置】\r\n");
	printf("（3）IP地址：%d.%d.%d.%d\r\n", 
		   rev->network.ip[0], rev->network.ip[1], rev->network.ip[2], rev->network.ip[3]);
	
	printf("（4）TCP端口号：%d\r\n", rev->network.port);
	printf("（5）TCP模式：%s\r\n", rev->network.tcp_mode == 0 ? "客户端(Client)" : "服务器(Server)");
	
	
	/* 采集配置 */
	printf("-------------------------------------\r\n");
	printf("【采集系统配置】\r\n");
	printf("（6）采样周期：%d ms\r\n", rev->acquisition.sample_interval_ms);
	printf("（7）滑动平均滤波窗口：%d\r\n", rev->acquisition.filter_window);
	printf("（8）中值滤波使能：%s\r\n", rev->acquisition.median_enable == 0 ? "关闭" : "开启");
	
	/* 通道配置（示例：打印第0通道，可循环打印所有启用的通道） */

	printf("（9）温度上限：%.d ℃\r\n", rev->channel.temp_high);
	printf("（10）温度下限：%.d ℃\r\n", rev->channel.temp_low);
	printf("（11）湿度上限：%.d %%RH\r\n", rev->channel.humi_high);
	printf("（12）湿度下限：%.d %%RH\r\n", rev->channel.humi_low);
	printf("（13）报警延迟：%d 秒\r\n", rev->channel.alarm_delay_s);
		
	printf("=====================================\r\n");
	printf("        配置参数打印完成\r\n");
	printf("=====================================\r\n\r\n");
}

/**
 * @brief       存储系统版本号
 * @param       varson: 需要修改的版本号字符：“版本：1.0.1”
 * @retval      无
 */
void system_config_to_version(char *version)
{
	/* 1. 修改版本号 */
	strncpy(system_config.version, version, sizeof(system_config.version) - 1);
	system_config.version[sizeof(system_config.version) - 1] = '\0';
	
	/* 2. 版本号存储 */
	system_config_to_storage(system_config);
}




