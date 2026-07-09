/**
 * @file Comm4G_at.c
 * @brief A7670C AT 命令层实现:命令发送 + 非阻塞行组装
 */
#include "Comm4G_at.h"

#if EN_4GCOMM

#include "Comm4G_uart.h"
#include <string.h>

static char     s_line[G4_AT_LINE_MAX];
static uint16_t s_len = 0;

void Comm4G_AtSend(const char *s)
{
    Comm4G_UartTxStr(s);
}

void Comm4G_AtSendCmd(const char *cmd)
{
    Comm4G_UartTxStr(cmd);
    Comm4G_UartTxStr("\r\n");
}

/*
 * 非阻塞:尽量消费串口环形里已到达的字节,遇 '\n' 返回一整行(去尾部 \r\n)。
 * 空行(仅 \r\n)自动跳过继续找下一行。行溢出则丢弃防越界。
 * 无完整行时返回 -1。
 */
int16_t Comm4G_AtGetLine(char *buf, uint16_t max)
{
    int c;
    while ((c = Comm4G_UartRxGetByte()) >= 0)
    {
        if (c == '\n')
        {
            uint16_t n = s_len;
            if (n > 0 && s_line[n - 1] == '\r')   /* 去掉结尾 \r */
                n--;
            s_len = 0;
            if (n == 0)
                continue;                          /* 空行:跳过,找下一行 */
            if (n >= max)
                n = max - 1;
            memcpy(buf, s_line, n);
            buf[n] = '\0';
            return (int16_t)n;
        }
        else
        {
            if (s_len < G4_AT_LINE_MAX - 1)
                s_line[s_len++] = (char)c;
            else
                s_len = 0;                         /* 行过长:丢弃重来 */
        }
    }
    return -1;
}

void Comm4G_AtReset(void)
{
    s_len = 0;
    Comm4G_UartRxFlush();
}

#endif /* EN_4GCOMM */
