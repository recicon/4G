/**
 * @file Comm4G.h
 * @brief 4G(A7670C)顶层:上电/联网状态机 + TCP 收发桥接(承载与485一致的Modbus)
 *
 * 控制脚(依据原理图,极性按 NPN 驱动推断,待硬件复核):
 *   LTEPWON   = PC14  4G 整机电源使能(高=上电,低=整模块掉电)
 *   PWRKEY    = PC13  经三极管驱动模块 PWRKEY(MCU高→PWRKEY低,开机低脉冲)
 *   RESET_N   = PC15  经三极管驱动模块 RESET_N(MCU高→复位)
 * UART = USART1(PB6=TX/PB7=RX),见 Comm4G_uart。
 */
#ifndef __COMM4G_H__
#define __COMM4G_H__

#include "bms_config.h"

#if EN_4GCOMM

#include <stdint.h>

void    Comm4G_Init(void);                                 /* 上电初始化 + 进入联网状态机 */
void    Comm4G_Loop(void);                                 /* 主循环每拍驱动状态机与收发 */
void    Comm4G_SocketSend(const uint8_t *buf, uint16_t len);/* 置待发(实际CIPSEND由Loop异步完成) */
uint8_t Comm4G_IsOnline(void);                             /* TCP 是否在线(供休眠判据) */
void    Comm4G_SleepDisable(void);                         /* 休眠前:断4G电源 + 停 USART1 */
void    Comm4G_SleepRestore(void);                         /* 唤醒后:重上电重连 */

#endif /* EN_4GCOMM */
#endif /* __COMM4G_H__ */
