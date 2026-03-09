#include "./BSP/IIC/myiic.h"

/**
 * @brief       初始化IIC通信的GPIO口
 * @note        配置PB8(SCL)、PB9(SDA)为推挽输出、上拉、高速模式，初始电平均为高
 * @param       无
 * @retval      无
 */
void IIC_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    __HAL_RCC_GPIOB_CLK_ENABLE();   // 使能GPIOB时钟
    
    // PB8、PB9初始化设置（修正原注释的PH4,5笔误）
    GPIO_Initure.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;  // 推挽输出
    GPIO_Initure.Pull = GPIO_PULLUP;          // 上拉
    GPIO_Initure.Speed = GPIO_SPEED_FAST;     // 快速
    HAL_GPIO_Init(GPIOB, &GPIO_Initure);
    
    IIC_SDA = 1;  // SDA引脚初始高电平
    IIC_SCL = 1;  // SCL引脚初始高电平
}

/**
 * @brief       产生IIC通信的起始信号
 * @note        起始信号时序：SCL高电平时，SDA从高电平拉低
 * @param       无
 * @retval      无
 */
void IIC_Start(void)
{
    SDA_OUT();     // SDA线设置为输出模式
    IIC_SDA = 1;	  	  
    IIC_SCL = 1;
    delay_us(4);   // 延时保证时序稳定
    IIC_SDA = 0;   // START: 当CLK为高时，DATA从高变低
    delay_us(4);
    IIC_SCL = 0;   // 钳住I2C总线，准备发送或接收数据
}	  

/**
 * @brief       产生IIC通信的停止信号
 * @note        停止信号时序：SCL高电平时，SDA从低电平拉高
 * @param       无
 * @retval      无
 */
void IIC_Stop(void)
{
    SDA_OUT();     // SDA线设置为输出模式
    IIC_SCL = 0;
    IIC_SDA = 0;   // STOP: 当CLK为高时，DATA从低变高
    delay_us(4);
    IIC_SCL = 1; 
    IIC_SDA = 1;   // 发送I2C总线结束信号
    delay_us(4);							   	
}

/**
 * @brief       等待IIC从机的应答信号
 * @note        超时时间250us，超时后自动发送停止信号
 * @param       无
 * @retval      0: 接收应答成功（从机正常响应）; 1: 接收应答失败（超时/从机无响应）
 */
u8 IIC_Wait_Ack(void)
{
    u8 ucErrTime = 0;
    SDA_IN();      // SDA设置为输入模式，读取从机应答
    // 注：输入模式下删除了原IIC_SDA=1赋值，避免#137错误
    delay_us(1);	   
    IIC_SCL = 1;
    delay_us(1);	 
    while(READ_SDA)  // 等待SDA拉低（应答信号）
    {
        ucErrTime++;
        if(ucErrTime > 250)  // 超时判定
        {
            IIC_Stop();      // 超时发送停止信号
            return 1;        // 应答失败
        }
    }
    IIC_SCL = 0;      // 时钟拉低，结束应答检测	   
    return 0;         // 应答成功
} 

/**
 * @brief       产生IIC通信的ACK应答信号
 * @note        主机发送ACK，告知从机继续传输数据
 * @param       无
 * @retval      无
 */
void IIC_Ack(void)
{
    IIC_SCL = 0;
    SDA_OUT();       // SDA设置为输出模式
    IIC_SDA = 0;     // SDA拉低表示ACK
    delay_us(2);
    IIC_SCL = 1;     // 时钟拉高，从机检测ACK
    delay_us(2);
    IIC_SCL = 0;     // 时钟拉低
}

/**
 * @brief       产生IIC通信的NACK非应答信号
 * @note        主机发送NACK，告知从机停止传输数据
 * @param       无
 * @retval      无
 */
void IIC_NAck(void)
{
    IIC_SCL = 0;
    SDA_OUT();       // SDA设置为输出模式
    IIC_SDA = 1;     // SDA拉高表示NACK
    delay_us(2);
    IIC_SCL = 1;     // 时钟拉高，从机检测NACK
    delay_us(2);
    IIC_SCL = 0;     // 时钟拉低
}					 				     

/**
 * @brief       IIC主机发送一个字节的数据
 * @note        数据从最高位(MSB)开始逐位发送，包含必要的时序延时
 * @param       txd: 要发送的单字节数据（8位）
 * @retval      无（应答检测需调用IIC_Wait_Ack()）
 */
void IIC_Send_Byte(u8 txd)
{                        
    u8 t;   
    SDA_OUT(); 	    
    IIC_SCL = 0;     // 拉低时钟开始数据传输
    for(t = 0; t < 8; t++)  // 逐位发送8位数据
    {              
        IIC_SDA = (txd & 0x80) >> 7;  // 发送当前最高位
        txd <<= 1; 	                 // 左移准备下一位
        delay_us(2);   // 延时保证时序（适配TEA5767等器件）
        IIC_SCL = 1;   // 时钟拉高，从机读取数据
        delay_us(2); 
        IIC_SCL = 0;	  // 时钟拉低，准备下一位
        delay_us(2);
    }	 
} 	

/**
 * @brief       IIC主机读取一个字节的数据
 * @note        数据从最高位(MSB)开始逐位读取，读取完成后发送ACK/NACK
 * @param       ack: 应答控制位（1: 发送ACK，0: 发送NACK）
 * @retval      读取到的单字节数据（8位）
 */
u8 IIC_Read_Byte(unsigned char ack)
{
    unsigned char i, receive = 0;
    SDA_IN();        // SDA设置为输入模式，读取从机数据
    for(i = 0; i < 8; i++ )
    {
        IIC_SCL = 0; 
        delay_us(2);
        IIC_SCL = 1;          // 时钟拉高，读取当前位数据
        receive <<= 1;        // 左移存储数据
        if(READ_SDA) receive++;   // 读取SDA电平，高电平则置1
        delay_us(1); 
    }					 
    if (!ack)
        IIC_NAck();  // 发送NACK，停止接收
    else
        IIC_Ack();   // 发送ACK，继续接收
    return receive;
}
