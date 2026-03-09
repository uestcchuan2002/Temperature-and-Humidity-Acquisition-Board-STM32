#include "./BSP/24CXX/24c02.h"
#include "./SYSTEM/delay/delay.h"

/**
 * @brief       初始化AT24CXX的IIC通信接口
 * @param       无
 * @retval      无
 */
void AT24CXX_Init(void)
{
	IIC_Init();//IIC初始化
}

/**
 * @brief       从AT24CXX指定地址读取一个字节的数据
 * @param       ReadAddr: 开始读取数据的地址（0~255 for AT24C02）
 * @retval      读到的单字节数据
 */
u8 AT24CXX_ReadOneByte(u16 ReadAddr)
{				  
	u8 temp=0;		  	    																 
    IIC_Start();  
	if(EE_TYPE>AT24C16)
	{
		IIC_Send_Byte(0XA0);	   //发送写命令
		IIC_Wait_Ack();
		IIC_Send_Byte(ReadAddr>>8);//发送高地址	    
	}else IIC_Send_Byte(0XA0+((ReadAddr/256)<<1));   //发送器件地址0XA0,写数据 	   
	IIC_Wait_Ack(); 
    IIC_Send_Byte(ReadAddr%256);   //发送低地址
	IIC_Wait_Ack();	    
	IIC_Start();  	 	   
	IIC_Send_Byte(0XA1);           //进入接收模式			   
	IIC_Wait_Ack();	 
    temp=IIC_Read_Byte(0);		   
    IIC_Stop();//产生一个停止条件	    
	return temp;
}

/**
 * @brief       向AT24CXX指定地址写入一个字节的数据
 * @param       WriteAddr: 写入数据的目标地址（0~255 for AT24C02）
 * @param       DataToWrite: 要写入的单字节数据
 * @retval      无
 */
void AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite)
{				   	  	    																 
    IIC_Start();  
	if(EE_TYPE>AT24C16)
	{
		IIC_Send_Byte(0XA0);	    //发送写命令
		IIC_Wait_Ack();
		IIC_Send_Byte(WriteAddr>>8);//发送高地址	  
	}else IIC_Send_Byte(0XA0+((WriteAddr/256)<<1));   //发送器件地址0XA0,写数据 	 
	IIC_Wait_Ack();	   
    IIC_Send_Byte(WriteAddr%256);   //发送低地址
	IIC_Wait_Ack(); 	 										  		   
	IIC_Send_Byte(DataToWrite);     //发送字节							   
	IIC_Wait_Ack();  		    	   
    IIC_Stop();//产生一个停止条件 
	delay_ms(10);	 
}

/**
 * @brief       向AT24CXX指定地址写入多字节数据（支持16/32位数据）
 * @param       WriteAddr: 开始写入的地址
 * @param       DataToWrite: 要写入的多字节数据（32位整数）
 * @param       Len: 要写入的数据长度（2表示16位，4表示32位）
 * @retval      无
 */
void AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len)
{  	
	u8 t;
	for(t=0;t<Len;t++)
	{
		AT24CXX_WriteOneByte(WriteAddr+t,(DataToWrite>>(8*t))&0xff);
	}												    
}

/**
 * @brief       从AT24CXX指定地址读取多字节数据（支持16/32位数据）
 * @param       ReadAddr: 开始读取的地址
 * @param       Len: 要读取的数据长度（2表示16位，4表示32位）
 * @retval      读取到的32位数据（低字节对应低地址）
 */
u32 AT24CXX_ReadLenByte(u16 ReadAddr,u8 Len)
{  	
	u8 t;
	u32 temp=0;
	for(t=0;t<Len;t++)
	{
		temp<<=8;
		temp+=AT24CXX_ReadOneByte(ReadAddr+Len-t-1); 	 				   
	}
	return temp;												    
}

/**
 * @brief       检测AT24CXX芯片是否正常工作
 * @note        通过读写芯片最后一个地址(255)的标志字(0X55)来检测
 * @param       无
 * @retval      0: 检测成功（芯片正常）; 1: 检测失败（芯片异常）
 */
u8 AT24CXX_Check(void)
{
	u8 temp;
	temp=AT24CXX_ReadOneByte(255);//避免每次开机都写AT24CXX			   
	if(temp==0X55)return 0;		   
	else//排除第一次初始化的情况
	{
		AT24CXX_WriteOneByte(255,0X55);
	    temp=AT24CXX_ReadOneByte(255);	  
		if(temp==0X55)return 0;
	}
	return 1;											  
}

/**
 * @brief       从AT24CXX指定地址批量读取多个字节数据
 * @param       ReadAddr: 开始读取的地址（AT24C02范围：0~255）
 * @param       pBuffer: 存储读取数据的数组首地址（输出参数）
 * @param       NumToRead: 要读取的数据个数
 * @retval      无
 */
void AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead)
{
	while(NumToRead)
	{
		*pBuffer++=AT24CXX_ReadOneByte(ReadAddr++);	
		NumToRead--;
	}
}  

/**
 * @brief       向AT24CXX指定地址批量写入多个字节数据
 * @param       WriteAddr: 开始写入的地址（AT24C02范围：0~255）
 * @param       pBuffer: 要写入的数据数组首地址
 * @param       NumToWrite: 要写入的数据个数
 * @retval      无
 */
void AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite)
{
	while(NumToWrite--)
	{
		AT24CXX_WriteOneByte(WriteAddr,*pBuffer);
		WriteAddr++;
		pBuffer++;
	}
}


