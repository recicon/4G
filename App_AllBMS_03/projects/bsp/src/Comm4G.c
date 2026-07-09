/**
 * @file Comm4G.c
 * @brief 4G(A7670C)顶层:上电/联网非阻塞状态机 + TCP 收发桥接
 *
 * 承载协议与 485 完全一致:收到的 socket 原始字节直接进 g_s4GRtxMsg.rbuf,
 * 由 Exec4GMsgProcess()(App.c)交给 ExecModbusMasterProcess(USART1,...) 解析,
 * 与 ExecBleMsgProcess 同构。发送由 SCI_GeneralSendParamOut 的 USART1 分支
 * 调 Comm4G_SocketSend() 置待发,本文件的状态机异步用 AT+CIPSEND 发出。
 *
 * A7670C 采用缓冲收(CIPRXGET=1):URC "+CIPRXGET: 1,0" 通知有数据,
 * 再用 "AT+CIPRXGET=2,0,<n>" 主动取回原始字节。相比 +++ 透传态,URC 通道常驻,
 * 断线检测更可靠。
 */
#include "Comm4G.h"

#if EN_4GCOMM

#include "Comm4G_at.h"
#include "Comm4G_uart.h"
#include "n32wb43x.h"
#include "App.h"                 /* g_tBatPackInfo / g_tBatPackCtrl / sUARTMsgType */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern uint64_t GetTick_ms(void);
extern void     Delay_ms(uint32_t ms);

/* g_s4GRtxMsg 的定义(声明在 Usart_Com.h),仅 EN_4GCOMM 时占 RAM */
sUARTMsgType g_s4GRtxMsg;

/* ===== 控制脚(PC13/14/15) ===== */
#define G4_PWREN_PORT    GPIOC
#define G4_PWREN_PIN     GPIO_PIN_14     /* LTEPWON: 高=使能4G电源 */
#define G4_PWRKEY_PORT   GPIOC
#define G4_PWRKEY_PIN    GPIO_PIN_13     /* →PWRKEY: 高=PWRKEY拉低(开机低脉冲) */
#define G4_RESET_PORT    GPIOC
#define G4_RESET_PIN     GPIO_PIN_15     /* →RESET_N: 高=复位 */

/* ===== 主状态 ===== */
typedef enum {
    G4_OFF = 0,
    G4_PWR_ON,      /* 电源使能 + PWRKEY 开机脉冲 */
    G4_WAIT_READY,  /* 等模块启动 */
    G4_SYNC,        /* ATE0 探活+关回显 */
    G4_SIM,         /* CPIN? */
    G4_NET,         /* CEREG? 注册 */
    G4_PDP,         /* CGDCONT + NETOPEN */
    G4_SOCK,        /* CIPRXGET=1 + CIPOPEN */
    G4_ONLINE,      /* 在线收发/心跳 */
    G4_ERROR        /* 退避重连 */
} eG4State;

/* ONLINE 内收发子状态 */
typedef enum {
    IO_IDLE = 0,
    IO_TX,      /* 发 CIPSEND, 等 '>' */
    IO_TXWAIT,  /* 等 SEND OK */
    IO_RXREQ,   /* 发 CIPRXGET=2 */
    IO_RXDATA,  /* 读定长 payload */
    IO_PING     /* 心跳查询 */
} eG4Io;

static eG4State s_state = G4_OFF;
static eG4Io    s_io    = IO_IDLE;
static uint64_t s_tsEnter = 0;      /* 进入当前主状态时刻 */
static uint64_t s_tsCmd   = 0;      /* 上次发命令时刻 */
static uint8_t  s_step    = 0;      /* 状态内子步骤 */
static uint8_t  s_sent    = 0;      /* 当前命令已发标志 */
static uint8_t  s_retry   = 0;      /* 重试计数 */
static uint32_t s_reconnDelayS = G4_RECONNECT_MIN_S;
static uint64_t s_tsHeartbeat = 0;
static uint64_t s_tsCommOk    = 0;  /* 上次收到 socket 数据(诊断/心跳基准) */

static uint8_t  s_rxNotify = 0;     /* 收到 +CIPRXGET:1 通知 */
static uint16_t s_rxReq    = 0;     /* 本次待读 payload 长度 */
static uint16_t s_rxGot    = 0;

static uint8_t  s_txPending = 0;
static uint16_t s_txLen     = 0;
static uint8_t  s_txBuf[UART_TX_MAX_SIZE];
static uint8_t  s_txPrompt  = 0;

#define G4_SET_STATE(st)  do { s_state=(st); s_tsEnter=GetTick_ms(); s_step=0; s_sent=0; s_retry=0; } while(0)
#define G4_MAX_RETRY      5

/* ===== 控制脚 IO ===== */
static void g4_power(uint8_t on)   { if(on) GPIO_SetBits(G4_PWREN_PORT,G4_PWREN_PIN);  else GPIO_ResetBits(G4_PWREN_PORT,G4_PWREN_PIN); }
static void g4_pwrkey(uint8_t act) { if(act) GPIO_SetBits(G4_PWRKEY_PORT,G4_PWRKEY_PIN); else GPIO_ResetBits(G4_PWRKEY_PORT,G4_PWRKEY_PIN); }
static void g4_reset(uint8_t act)  { if(act) GPIO_SetBits(G4_RESET_PORT,G4_RESET_PIN);  else GPIO_ResetBits(G4_RESET_PORT,G4_RESET_PIN); }

static void g4_io_init(void)
{
    GPIO_InitType io;
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOC, ENABLE);
    /* PC13/14/15 是 RTC/OSC32/TAMPER(backup 域)复用脚:确保 backup 域可写并关掉 LSE,
       避免曾被其它固件/厂测使能过 LSE/RTC 时这三脚被占用,导致 4G 控制脚静默失效 */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
    PWR_BackupAccessEnable(ENABLE);
    RCC_ConfigLse(RCC_LSE_DISABLE, 0);
    GPIO_InitStruct(&io);
    io.GPIO_Mode = GPIO_Mode_Out_PP;
    io.GPIO_Pull = GPIO_No_Pull;
    io.Pin = G4_PWREN_PIN;  GPIO_InitPeripheral(G4_PWREN_PORT,  &io);
    io.Pin = G4_PWRKEY_PIN; GPIO_InitPeripheral(G4_PWRKEY_PORT, &io);
    io.Pin = G4_RESET_PIN;  GPIO_InitPeripheral(G4_RESET_PORT,  &io);
    g4_power(0);
    g4_pwrkey(0);
    g4_reset(0);
}

/* 通用命令执行:返回 1=收到期望token, -1=ERROR/超时, 0=进行中。跨多次Loop推进。 */
static int8_t g4_cmd(const char *cmd, const char *ok, uint32_t toMs)
{
    char line[G4_AT_LINE_MAX];
    if (!s_sent) { Comm4G_AtReset(); Comm4G_AtSendCmd(cmd); s_tsCmd = GetTick_ms(); s_sent = 1; }
    while (Comm4G_AtGetLine(line, sizeof line) > 0)
    {
        if (ok && strstr(line, ok)) { s_sent = 0; return 1; }
        if (strstr(line, "ERROR"))  { s_sent = 0; return -1; }
    }
    if (GetTick_ms() - s_tsCmd > toMs) { s_sent = 0; return -1; }
    return 0;
}

static void online_service(void);

void Comm4G_Init(void)
{
    Comm4G_UartInit();
    g4_io_init();

    memset(&g_s4GRtxMsg, 0, sizeof g_s4GRtxMsg);
    g_s4GRtxMsg.Need_ms = 20;     /* TCP 分段可能到达,字节超时放宽到 20ms 完帧 */

    s_txPending  = 0;
    s_rxNotify   = 0;
    s_reconnDelayS = G4_RECONNECT_MIN_S;
    s_tsCommOk   = GetTick_ms();
    s_tsHeartbeat= GetTick_ms();
    G4_SET_STATE(G4_PWR_ON);
}

void Comm4G_Loop(void)
{
    char     line[G4_AT_LINE_MAX];
    char     cmd[96];
    uint64_t now = GetTick_ms();

    switch (s_state)
    {
    case G4_OFF:
        break;

    case G4_PWR_ON:
        /* 上电时序:使能电源→100ms后拉PWRKEY(低脉冲)→保持~1.3s→释放 */
        if (s_step == 0) { g4_power(1); g4_reset(0); s_step = 1; }
        else if (s_step == 1) { if (now - s_tsEnter > 100)  { g4_pwrkey(1); s_step = 2; } }
        else                  { if (now - s_tsEnter > 1400) { g4_pwrkey(0); G4_SET_STATE(G4_WAIT_READY); } }
        break;

    case G4_WAIT_READY:
        while (Comm4G_AtGetLine(line, sizeof line) > 0)
        {
            if (strstr(line, "RDY") || strstr(line, "PB DONE") || strstr(line, "PB DONE"))
            { G4_SET_STATE(G4_SYNC); break; }
        }
        if (now - s_tsEnter > 8000) G4_SET_STATE(G4_SYNC);   /* 超时也试探 */
        break;

    case G4_SYNC:
    {
        int8_t r = g4_cmd("ATE0", "OK", 1000);
        if (r == 1)      G4_SET_STATE(G4_SIM);
        else if (r == -1){ if (++s_retry > 10) G4_SET_STATE(G4_ERROR); }
        break;
    }

    case G4_SIM:
    {
        int8_t r = g4_cmd("AT+CPIN?", "READY", 3000);
        if (r == 1)      G4_SET_STATE(G4_NET);
        else if (r == -1){ if (++s_retry > G4_MAX_RETRY) G4_SET_STATE(G4_ERROR); }
        break;
    }

    case G4_NET:
        /* 轮询注册状态 +CEREG: x,1(已注册) 或 x,5(漫游) */
        if (!s_sent) { Comm4G_AtReset(); Comm4G_AtSendCmd("AT+CEREG?"); s_tsCmd = now; s_sent = 1; }
        while (Comm4G_AtGetLine(line, sizeof line) > 0)
        {
            if (strstr(line, "+CEREG:") && (strstr(line, ",1") || strstr(line, ",5")))
            { s_sent = 0; G4_SET_STATE(G4_PDP); break; }
        }
        if (s_state == G4_NET)
        {
            if (now - s_tsCmd > 2000) s_sent = 0;             /* 重发查询 */
            if (now - s_tsEnter > 90000) G4_SET_STATE(G4_ERROR); /* 90s 未注册 */
        }
        break;

    case G4_PDP:
        if (s_step == 0)
        {
            sprintf(cmd, "AT+CGDCONT=1,\"IP\",\"%s\"", G4_APN);
            int8_t r = g4_cmd(cmd, "OK", 3000);
            if (r == 1) s_step = 1;
            else if (r == -1) G4_SET_STATE(G4_ERROR);
        }
        else
        {
            int8_t r = g4_cmd("AT+NETOPEN", "OK", 60000);
            /* NETOPEN 返回 OK/+NETOPEN:0;若已打开会回 ERROR,同样视为可用 */
            if (r == 1 || r == -1) G4_SET_STATE(G4_SOCK);
        }
        break;

    case G4_SOCK:
        if (s_step == 0)
        {
            int8_t r = g4_cmd("AT+CIPRXGET=1", "OK", 3000);
            if (r == 1) s_step = 1;
            else if (r == -1) G4_SET_STATE(G4_ERROR);
        }
        else
        {
            if (!s_sent)
            {
                sprintf(cmd, "AT+CIPOPEN=0,\"TCP\",\"%s\",%d", G4_SERVER_ADDR, (int)G4_SERVER_PORT);
                Comm4G_AtReset();
                Comm4G_AtSendCmd(cmd);
                s_tsCmd = now; s_sent = 1;
            }
            while (Comm4G_AtGetLine(line, sizeof line) > 0)
            {
                if (strstr(line, "+CIPOPEN: 0,0"))
                {
                    s_sent = 0;
                    s_reconnDelayS = G4_RECONNECT_MIN_S;     /* 连成功,退避复位 */
                    s_tsCommOk = now; s_tsHeartbeat = now;
                    s_io = IO_IDLE;
                    G4_SET_STATE(G4_ONLINE);
                    break;
                }
                if (strstr(line, "ERROR")) { s_sent = 0; G4_SET_STATE(G4_ERROR); break; }
            }
            if (s_state == G4_SOCK && now - s_tsCmd > 30000) G4_SET_STATE(G4_ERROR);
        }
        break;

    case G4_ONLINE:
        online_service();
        break;

    case G4_ERROR:
        /* 断电省电,退避后重上电重连 */
        if (!s_sent) { g4_power(0); s_io = IO_IDLE; s_tsCmd = now; s_sent = 1; }
        if (now - s_tsCmd > (uint64_t)s_reconnDelayS * 1000)
        {
            s_reconnDelayS <<= 1;
            if (s_reconnDelayS > G4_RECONNECT_MAX_S) s_reconnDelayS = G4_RECONNECT_MAX_S;
            G4_SET_STATE(G4_PWR_ON);
        }
        break;

    default:
        G4_SET_STATE(G4_ERROR);
        break;
    }

    /* 4G 通信诊断:长时间无有效通信则置故障位(好帧由 Exec4GMsgProcess 清0) */
    if (GetTick_ms() - s_tsCommOk > G4_COMM_TIMEOUT_MS)
        g_tBatPackInfo.tBmDiagVal.tBits.u14GCommFlg = 1;
}

/* ONLINE 收发/心跳子状态机 */
static void online_service(void)
{
    char     line[G4_AT_LINE_MAX];
    char     cmd[48];
    int      b;
    uint64_t now = GetTick_ms();

    switch (s_io)
    {
    case IO_IDLE:
        /* 处理 URC:数据到达通知 / 连接关闭 */
        while (Comm4G_AtGetLine(line, sizeof line) > 0)
        {
            if (strstr(line, "+CIPRXGET: 1")) s_rxNotify = 1;
            if (strstr(line, "CLOSED") || strstr(line, "+IPCLOSE") || strstr(line, "PDP DEACT"))
            { G4_SET_STATE(G4_ERROR); return; }
        }
        if (s_txPending)       { s_io = IO_TX;    s_sent = 0; }
        else if (s_rxNotify)   { s_rxNotify = 0; s_io = IO_RXREQ; s_sent = 0; }
        else if (now - s_tsHeartbeat > (uint64_t)G4_KEEPALIVE_S * 1000)
        { s_tsHeartbeat = now; s_io = IO_PING; s_sent = 0; }
        break;

    case IO_TX:
        if (!s_sent)
        {
            sprintf(cmd, "AT+CIPSEND=0,%u", (unsigned)s_txLen);
            Comm4G_AtSendCmd(cmd);
            s_tsCmd = now; s_sent = 1; s_txPrompt = 0;
        }
        while ((b = Comm4G_UartRxGetByte()) >= 0) { if (b == '>') { s_txPrompt = 1; break; } }
        if (s_txPrompt)
        {
            Comm4G_UartTxBytes(s_txBuf, s_txLen);
            s_txPrompt = 0; s_sent = 0; s_tsCmd = now;
            s_io = IO_TXWAIT;
        }
        else if (now - s_tsCmd > 3000) { s_txPending = 0; s_io = IO_IDLE; }
        break;

    case IO_TXWAIT:
        while (Comm4G_AtGetLine(line, sizeof line) > 0)
        {
            if (strstr(line, "+CIPRXGET: 1")) s_rxNotify = 1;   /* 收发窗口内数据到达通知,latch 待 IDLE 处理 */
            if (strstr(line, "SEND OK"))                        /* 严格只认 SEND OK,裸 OK 不算发送完成 */
            { s_txPending = 0; s_tsHeartbeat = now; s_io = IO_IDLE; return; }
            if (strstr(line, "ERROR"))
            { s_txPending = 0; s_io = IO_IDLE; return; }
        }
        if (now - s_tsCmd > 5000) { s_txPending = 0; s_io = IO_IDLE; }
        break;

    case IO_RXREQ:
        if (!s_sent)
        {
            uint16_t rbufFree = (uint16_t)(UART_RD_MAX_SIZE - g_s4GRtxMsg.r_index);   /* rbuf 空闲 */
            uint16_t ringFree = Comm4G_UartRxFree();                                  /* 环空闲(≤511) */
            uint16_t ringCap  = (ringFree > 40u) ? (uint16_t)(ringFree - 40u) : 0u;   /* 留 ~40B 给 AT 头尾 */
            uint16_t reqLen   = 400;                                                  /* 单次上限 */
            if (reqLen > ringCap)  reqLen = ringCap;
            if (reqLen > rbufFree) reqLen = rbufFree;
            sprintf(cmd, "AT+CIPRXGET=2,0,%u", (unsigned)reqLen);   /* 动态钳位 min(400,环空闲,rbuf空闲),防粘包溢出 */
            Comm4G_AtSendCmd(cmd); s_tsCmd = now; s_sent = 1;
        }
        while (Comm4G_AtGetLine(line, sizeof line) > 0)
        {
            char *p = strstr(line, "+CIPRXGET: 2,0,");
            if (strstr(line, "+CIPRXGET: 1")) s_rxNotify = 1;   /* 又一批数据到达,latch 待读完后 IDLE 再取 */
            if (p)
            {
                s_rxReq = (uint16_t)atoi(p + 15);   /* 跳过 "+CIPRXGET: 2,0," 取 reqlen */
                s_rxGot = 0; s_sent = 0;
                s_tsCmd = now;
                s_io = (s_rxReq > 0) ? IO_RXDATA : IO_IDLE;
                break;
            }
            if (strstr(line, "ERROR")) { s_sent = 0; s_io = IO_IDLE; break; }
        }
        if (s_io == IO_RXREQ && now - s_tsCmd > 3000) { s_sent = 0; s_io = IO_IDLE; }
        break;

    case IO_RXDATA:
    {
        uint16_t avail = Comm4G_UartRxAvail();
        uint16_t need  = (uint16_t)(s_rxReq - s_rxGot);
        uint16_t space = (uint16_t)(UART_RD_MAX_SIZE - g_s4GRtxMsg.r_index);
        uint16_t take  = avail < need ? avail : need;
        if (take > space) take = space;
        if (take > 0)
        {
            uint16_t got = Comm4G_UartRxFetch(&g_s4GRtxMsg.rbuf[g_s4GRtxMsg.r_index], take);
            g_s4GRtxMsg.r_index    = (int16_t)(g_s4GRtxMsg.r_index + got);
            g_s4GRtxMsg.Timeout_ms = g_s4GRtxMsg.Need_ms;
            s_rxGot = (uint16_t)(s_rxGot + got);
            s_tsCommOk = now;
        }
        if (s_rxGot >= s_rxReq)          s_io = IO_IDLE;   /* 后随 \r\nOK 由下拍 IDLE 消费 */
        else if (now - s_tsCmd > 3000)   s_io = IO_IDLE;   /* 数据未收全,放弃 */
        break;
    }

    case IO_PING:
        if (!s_sent) { Comm4G_AtSendCmd("AT+CIPOPEN?"); s_tsCmd = now; s_sent = 1; }
        while (Comm4G_AtGetLine(line, sizeof line) > 0)
        {
            if (strstr(line, "+CIPRXGET: 1")) s_rxNotify = 1;   /* 心跳窗口内数据到达通知,latch */
            if (strstr(line, "+CIPOPEN: 0,\"TCP\"")) { s_tsCommOk = now; s_io = IO_IDLE; return; } /* 在线:刷新通信基准,免 u14GCommFlg 误报 */
            if (strstr(line, "CLOSED") || strstr(line, "+IPCLOSE"))
            { G4_SET_STATE(G4_ERROR); return; }
            if (strstr(line, "OK")) { s_io = IO_IDLE; return; }
        }
        if (now - s_tsCmd > 3000) { s_io = IO_IDLE; }
        break;

    default:
        s_io = IO_IDLE;
        break;
    }
}

void Comm4G_SocketSend(const uint8_t *buf, uint16_t len)
{
    if (s_state != G4_ONLINE) return;         /* 离线时应答直接丢弃 */
    if (len == 0) return;
    if (len > sizeof s_txBuf) len = sizeof s_txBuf;
    memcpy(s_txBuf, buf, len);
    s_txLen = len;
    s_txPending = 1;                          /* 由 online_service 异步 CIPSEND */
}

uint8_t Comm4G_IsOnline(void)
{
    return (uint8_t)(s_state == G4_ONLINE);
}

void Comm4G_SleepDisable(void)
{
    g4_power(0);                              /* 拉低 LTEPWON,4G 整机断电省电 */
    USART_Enable(USART1, DISABLE);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_USART1, DISABLE);
    s_state = G4_OFF;
    s_io    = IO_IDLE;
}

void Comm4G_SleepRestore(void)
{
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_USART1, ENABLE);
    Comm4G_UartInit();
    s_tsCommOk    = GetTick_ms();
    s_tsHeartbeat = GetTick_ms();
    s_reconnDelayS = G4_RECONNECT_MIN_S;
    G4_SET_STATE(G4_PWR_ON);                  /* 重新上电重连 */
}

#endif /* EN_4GCOMM */
