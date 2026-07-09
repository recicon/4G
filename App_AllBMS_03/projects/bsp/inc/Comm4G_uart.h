/**
 * @file Comm4G_uart.h
 * @brief 4G(A7670C)模块物理层串口 —— USART1(PB6=TX / PB7=RX)
 *
 * 中断收字节入环形缓冲 + 轮询发送。仅在 EN_4GCOMM=1 时编译。
 * 引脚依据原理图 HTBLEBMSALLCtrl_4G&AB:MBUTx=PB6, MBURx=PB7。
 */
#ifndef __COMM4G_UART_H__
#define __COMM4G_UART_H__

#include "bms_config.h"

#if EN_4GCOMM

#include <stdint.h>

/* USART1 初始化(时钟/GPIO/NVIC/中断收) */
void     Comm4G_UartInit(void);

/* 发送(轮询,阻塞至发完):AT 命令与 CIPSEND 数据量都很小 */
void     Comm4G_UartTxBytes(const uint8_t *buf, uint16_t len);
void     Comm4G_UartTxStr(const char *s);

/* 接收环形缓冲读取 */
uint16_t Comm4G_UartRxAvail(void);                        /* 可读字节数 */
uint16_t Comm4G_UartRxFree(void);                         /* 环形缓冲空闲(有效容量511) */
int      Comm4G_UartRxGetByte(void);                      /* 取一字节,空返回 -1 */
uint16_t Comm4G_UartRxFetch(uint8_t *dst, uint16_t max);  /* 批量取,返回实际字节数 */
void     Comm4G_UartRxFlush(void);                        /* 丢弃全部未读 */

#endif /* EN_4GCOMM */
#endif /* __COMM4G_UART_H__ */
