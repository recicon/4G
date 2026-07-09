/**
 * @file Comm4G_uart.c
 * @brief 4G(A7670C)物理层:USART1 中断收字节入环形缓冲 + 轮询发送
 *
 * 参照 bsp/src/Usart_Com.c 的中断收发风格(与 485/UART5 一致),
 * 但接收改为写入私有环形缓冲,供 AT 层按行/URC 消费。
 * 选中断而非 DMA:不占 DMA 通道、无 remap 依赖,115200 下 M4 中断收字节无压力,
 * 环形缓冲同样抗溢出。
 */
#include "Comm4G_uart.h"

#if EN_4GCOMM

#include "n32wb43x.h"
#include <string.h>

/* ===== USART1 资源(按原理图:PB6=TX, PB7=RX) ===== */
#define G4U_USART            USART1
#define G4U_USART_CLK        RCC_APB2_PERIPH_USART1
#define G4U_GPIO_PORT        GPIOB
#define G4U_GPIO_CLK         RCC_APB2_PERIPH_GPIOB
#define G4U_TX_PIN           GPIO_PIN_6
#define G4U_RX_PIN           GPIO_PIN_7
#define G4U_IRQn             USART1_IRQn
/* !!! 待数据手册核对:PB6/PB7 上 USART1 的复用编号(候选 GPIO_AF0/AF1/AF4_USART1) */
#define G4U_GPIO_AF          GPIO_AF4_USART1

/* ===== 接收环形缓冲(中断写 head,主循环读 tail) ===== */
#define G4U_RX_RING_SIZE     512u
static volatile uint8_t  s_rxRing[G4U_RX_RING_SIZE];
static volatile uint16_t s_rxHead = 0;
static volatile uint16_t s_rxTail = 0;

void Comm4G_UartInit(void)
{
    GPIO_InitType  GPIO_InitStructure;
    USART_InitType USART_InitStructure;
    NVIC_InitType  NVIC_InitStructure;

    /* 时钟 */
    RCC_EnableAPB2PeriphClk(G4U_GPIO_CLK, ENABLE);
    RCC_EnableAPB2PeriphClk(G4U_USART_CLK, ENABLE);

    /* GPIO:TX = AF 推挽,RX = AF 上拉 */
    GPIO_InitStruct(&GPIO_InitStructure);
    GPIO_InitStructure.Pin            = G4U_TX_PIN;
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Alternate = G4U_GPIO_AF;
    GPIO_InitPeripheral(G4U_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.Pin            = G4U_RX_PIN;
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Alternate = G4U_GPIO_AF;
    GPIO_InitPeripheral(G4U_GPIO_PORT, &GPIO_InitStructure);

    /* NVIC */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
    NVIC_InitStructure.NVIC_IRQChannel                   = G4U_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* USART1:G4_UART_BAUD 8N1 */
    USART_InitStructure.BaudRate            = G4_UART_BAUD;
    USART_InitStructure.WordLength          = USART_WL_8B;
    USART_InitStructure.StopBits            = USART_STPB_1;
    USART_InitStructure.Parity              = USART_PE_NO;
    USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
    USART_InitStructure.Mode                = USART_MODE_RX | USART_MODE_TX;
    USART_Init(G4U_USART, &USART_InitStructure);

    s_rxHead = 0;
    s_rxTail = 0;
    USART_ConfigInt(G4U_USART, USART_INT_RXDNE, ENABLE);
    USART_Enable(G4U_USART, ENABLE);
}

void Comm4G_UartTxBytes(const uint8_t *buf, uint16_t len)
{
    uint16_t i;
    for (i = 0; i < len; i++)
    {
        USART_SendData(G4U_USART, buf[i]);
        while (USART_GetFlagStatus(G4U_USART, USART_FLAG_TXDE) == RESET) { }
    }
    /* 等待最后一字节移位完成,确保方向/时序干净 */
    while (USART_GetFlagStatus(G4U_USART, USART_FLAG_TXC) == RESET) { }
}

void Comm4G_UartTxStr(const char *s)
{
    Comm4G_UartTxBytes((const uint8_t *)s, (uint16_t)strlen(s));
}

uint16_t Comm4G_UartRxAvail(void)
{
    return (uint16_t)((s_rxHead + G4U_RX_RING_SIZE - s_rxTail) % G4U_RX_RING_SIZE);
}

uint16_t Comm4G_UartRxFree(void)
{
    return (uint16_t)((G4U_RX_RING_SIZE - 1u) - Comm4G_UartRxAvail());   /* 满判牺牲一格,有效容量 511 */
}

int Comm4G_UartRxGetByte(void)
{
    uint8_t b;
    if (s_rxHead == s_rxTail)
        return -1;
    b = s_rxRing[s_rxTail];
    s_rxTail = (uint16_t)((s_rxTail + 1u) % G4U_RX_RING_SIZE);
    return (int)b;
}

uint16_t Comm4G_UartRxFetch(uint8_t *dst, uint16_t max)
{
    uint16_t n = 0;
    int c;
    while (n < max && (c = Comm4G_UartRxGetByte()) >= 0)
        dst[n++] = (uint8_t)c;
    return n;
}

void Comm4G_UartRxFlush(void)
{
    s_rxTail = s_rxHead;
}

/* USART1 接收中断:逐字节写环形缓冲,满则丢弃最新字节(保护不覆盖未读) */
void USART1_IRQHandler(void)
{
    if (USART_GetIntStatus(G4U_USART, USART_INT_RXDNE) != RESET)
    {
        uint8_t  d    = (uint8_t)USART_ReceiveData(G4U_USART);   /* 读 DAT 清 RXDNE */
        uint16_t next = (uint16_t)((s_rxHead + 1u) % G4U_RX_RING_SIZE);
        if (next != s_rxTail)
        {
            s_rxRing[s_rxHead] = d;
            s_rxHead = next;
        }
    }
}

#endif /* EN_4GCOMM */
