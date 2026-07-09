/**
 * @file Comm4G_at.h
 * @brief A7670C AT 命令层 —— 命令发送 + 非阻塞行组装
 *
 * 字节流来自 Comm4G_uart 的环形缓冲;本层把字节组装成以 \r\n 结束的"行"
 * (AT 响应与 URC 均为行式),供顶层状态机用 strstr 匹配。
 * socket 的二进制 payload 是定长的,由顶层拿到 +CIPRXGET 头后用
 * Comm4G_UartRxFetch 直接读取,不经过本层行组装。
 */
#ifndef __COMM4G_AT_H__
#define __COMM4G_AT_H__

#include "bms_config.h"

#if EN_4GCOMM

#include <stdint.h>

#define G4_AT_LINE_MAX   256

void    Comm4G_AtSend(const char *s);              /* 原样发送字符串 */
void    Comm4G_AtSendCmd(const char *cmd);         /* 发送 cmd 并自动追加 "\r\n" */
int16_t Comm4G_AtGetLine(char *buf, uint16_t max); /* 非阻塞:取一整行(去\r\n),无整行返回 -1 */
void    Comm4G_AtReset(void);                      /* 复位行组装状态 + 清串口环形缓冲 */

#endif /* EN_4GCOMM */
#endif /* __COMM4G_AT_H__ */
