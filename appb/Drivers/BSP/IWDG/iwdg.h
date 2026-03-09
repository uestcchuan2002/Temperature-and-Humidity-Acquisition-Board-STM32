#ifndef __IWDG_H
#define __IWDG_H
#include "./SYSTEM/sys/sys.h"

void IWDG_Init(uint8_t prer,uint16_t rlr);//初始化IWDG，并使能IWDG
void IWDG_Feed(void);//喂狗
#endif
