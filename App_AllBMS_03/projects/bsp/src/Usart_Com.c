#include"Usart_Com.h"
#include <string.h>


sUARTMsgType g_sMainRtxMsg;							//UART2 模块接收缓冲区
sUARTMsgType g_sBleRtxMsg;
sUARTMsgType g_sUartRtxMsg;

/**
 * @brief  Configures the different system clocks.
 */
void RCC_Configuration(void)
{
    /* Enable GPIO clock */
    RCC_EnableAPB2PeriphClk(USART485_GPIO_CLK, ENABLE);
    /* Enable USART485 and USARTz Clock */
    USART485_APBxClkCmd(USART485_CLK, ENABLE);

}
/**
 * @brief  Configures the different GPIO ports.
 */
void GPIO_Configuration(void)
{
GPIO_InitType GPIO_InitStructure;

    /* Initialize GPIO_InitStructure */
    GPIO_InitStruct(&GPIO_InitStructure);

    /* Configure USART485 Tx as alternate function push-pull */
    GPIO_InitStructure.Pin            = USART485_TxPin;    
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Alternate = USART485_Tx_GPIO_AF;
    GPIO_InitPeripheral(USART485_GPIO, &GPIO_InitStructure);

    /* Configure USART485 Rx as alternate function push-pull and pull-up */
    GPIO_InitStructure.Pin            = USART485_RxPin;
    GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Alternate = USART485_Rx_GPIO_AF;
    GPIO_InitPeripheral(USART485_GPIO, &GPIO_InitStructure);    

}

/**
 * @brief  Configures the nested vectored interrupt controller.
 */
void NVIC_Configuration(void)
{
    NVIC_InitType NVIC_InitStructure;

    /* Configure the NVIC Preemption Priority Bits */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

    /* Enable the USART485 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel            = USART485_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd         = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

}

void Uart_Init(void)
{
    USART_InitType UART_InitStructure;
    NVIC_InitType NVIC_InitStructure;
    GPIO_InitType GPIO_InitStructure;

    /* Enable GPIO clock */
    RCC_EnableAPB2PeriphClk(UART_GPIO_CLK, ENABLE);
    /* Enable USARTz Clock */
    RCC_EnableAPB2PeriphClk(UART_CLK, ENABLE);

    /* NVIC configuration */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
    NVIC_InitStructure.NVIC_IRQChannel            = UART_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd         = ENABLE;
    NVIC_Init(&NVIC_InitStructure);



    /* Initialize GPIO_InitStructure */
    GPIO_InitStruct(&GPIO_InitStructure);

    GPIO_InitStructure.Pin            = UART_TxPin;    
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Alternate = UART_Tx_GPIO_AF;
    GPIO_InitPeripheral(UART_TxPort, &GPIO_InitStructure);

    GPIO_InitStructure.Pin            = UART_RxPin;
    GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Alternate = UART_Rx_GPIO_AF;
    GPIO_InitPeripheral(UART_RxPort, &GPIO_InitStructure);    


    /* UART configuration ------------------------------------------------------*/
    UART_InitStructure.BaudRate            = 9600;
    UART_InitStructure.WordLength          = USART_WL_8B;
    UART_InitStructure.StopBits            = USART_STPB_1;
    UART_InitStructure.Parity              = USART_PE_NO;
    UART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
    UART_InitStructure.Mode                = USART_MODE_RX | USART_MODE_TX;
    USART_Init(UART, &UART_InitStructure);


    /* Enable UART Receive and Transmit interrupts */
    USART_ConfigInt(UART, USART_INT_RXDNE, ENABLE);

    USART_Enable(UART, ENABLE);
    g_sUartRtxMsg.Need_ms = (uint16_t)(1000.0f / (9600/12) + 3);

}


void Usart485_Init(void)
{
    USART_InitType USART_InitStructure;
    RCC_Configuration();

    /* NVIC configuration */
    NVIC_Configuration();

    /* Configure the GPIO ports */
    GPIO_Configuration();
    PortInit(UARTTxRxm_PORT, UARTTxRxm_PIN, GPIO_Mode_Out_PP); 

    /* USART485 configuration ------------------------------------------------------*/

    USART_InitStructure.BaudRate            = 9600;
    USART_InitStructure.WordLength          = USART_WL_8B;
    USART_InitStructure.StopBits            = USART_STPB_1;
    USART_InitStructure.Parity              = USART_PE_NO;
    USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
    USART_InitStructure.Mode                = USART_MODE_RX | USART_MODE_TX;
    /* Configure USART485 and USARTz */
    USART_Init(USART485, &USART_InitStructure);


    /* Enable USART485 Receive and Transmit interrupts */
    USART_ConfigInt(USART485, USART_INT_RXDNE, ENABLE);

    /* Enable the USART485 and USARTz */
    USART_Enable(USART485, ENABLE);

    g_sMainRtxMsg.Need_ms = (uint16_t)(1000.0f / (9600/12) + 3);
    GPIO_ResetBits(UARTTxRxm_PORT, UARTTxRxm_PIN);

}

void RS485_Send_Data(uint8_t *buf, uint16_t len)
{
    memcpy(g_sMainRtxMsg.wbuf, buf, len);
    g_sMainRtxMsg.w_num = len;
    g_sMainRtxMsg.w_index = 0;
    GPIO_SetBits(UARTTxRxm_PORT, UARTTxRxm_PIN);
    Delay_us(3);
    USART_ConfigInt(USART485, USART_INT_RXDNE, DISABLE);
    USART_ConfigInt(USART485, USART_INT_TXDE, ENABLE);

}

void USART485_IRQHandler(void)
{

    uint8_t received_data;

    if (USART_GetIntStatus(USART485, USART_INT_RXDNE) != RESET)
    {
        // 读取接收到的数据
        received_data = (uint8_t)USART_ReceiveData(USART485);
        if (g_sMainRtxMsg.r_index  < UART_RD_MAX_SIZE)
        {
            g_sMainRtxMsg.rbuf[g_sMainRtxMsg.r_index++] = received_data;
            g_sMainRtxMsg.Timeout_ms = g_sMainRtxMsg.Need_ms;

        }
    }

    if (USART_GetIntStatus(USART485, USART_INT_TXDE) != RESET)
    {
        if (g_sMainRtxMsg.w_index < g_sMainRtxMsg.w_num)
        {
            USART_SendData(USART485, g_sMainRtxMsg.wbuf[g_sMainRtxMsg.w_index++]);
        }
        else
        {
            USART_ConfigInt(USART485, USART_INT_TXC, ENABLE);
            USART_ConfigInt(USART485, USART_INT_TXDE, DISABLE);
            g_sMainRtxMsg.w_index = 0;
            g_sMainRtxMsg.w_num = 0;
        }
    }
    if (USART_GetIntStatus(USART485, USART_INT_TXC) == SET)
    {
        GPIO_ResetBits(UARTTxRxm_PORT, UARTTxRxm_PIN);
        USART_ClrIntPendingBit(USART485, USART_INT_TXC);
    USART_ConfigInt(USART485, USART_INT_TXC, DISABLE);
    USART_ConfigInt(USART485, USART_INT_RXDNE, ENABLE);
      

    }
}



extern volatile bool g_bWakeUart;

void UART_IRQHandler(void)
{

    uint8_t received_data;

    if (USART_GetIntStatus(UART, USART_INT_RXDNE) != RESET)
    {
        g_bWakeUart = true;
        // 读取接收到的数据
        received_data = (uint8_t)USART_ReceiveData(UART);
        if (g_sUartRtxMsg.r_index  < UART_RD_MAX_SIZE)
        {
            g_sUartRtxMsg.rbuf[g_sUartRtxMsg.r_index++] = received_data;
            g_sUartRtxMsg.Timeout_ms = g_sUartRtxMsg.Need_ms;

        }
    }

    if (USART_GetIntStatus(UART, USART_INT_TXDE) != RESET)
    {
        if (g_sUartRtxMsg.w_index < g_sUartRtxMsg.w_num)
        {
            USART_SendData(UART, g_sUartRtxMsg.wbuf[g_sUartRtxMsg.w_index++]);
        }
        else
        {
            USART_ConfigInt(UART, USART_INT_TXC, ENABLE);
            USART_ConfigInt(UART, USART_INT_TXDE, DISABLE);
            g_sUartRtxMsg.w_index = 0;
            g_sUartRtxMsg.w_num = 0;
        }
    }
    if (USART_GetIntStatus(UART, USART_INT_TXC) == SET)
    {
        USART_ClrIntPendingBit(UART, USART_INT_TXC);
        USART_ConfigInt(UART, USART_INT_RXDNE, ENABLE);
    }
}
