/*****************************************************************************
 * Copyright (c) 2022, Nations Technologies Inc.
 *
 * All rights reserved.
 * ****************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Nations' name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY NATIONS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL NATIONS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************/

/**
 * @file app_uart.c
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
 
#include "app_uart.h"
#include "n32wb43x.h"
#include "string.h"

#define MAX_DMA_RX_SIZE    1024
static uint8_t DmaRxBuffer[MAX_DMA_RX_SIZE];
static uint16_t  rx_out_pos = 0;

void uart_send_byte(uint8_t byte)
{
    USART_SendData(USARTy, byte);
    while (USART_GetFlagStatus(USARTy, USART_FLAG_TXDE) == RESET)
    {
    }
}  

void app_send_byte_array(uint8_t * buf, uint16_t length)
{
    for(int i=0; i<length; i++)
    {
	    //send a byte
	    uart_send_byte(buf[i]);	
    }
}    
 

#if 0
void uart_NVIC_Configuration(void)
{
    NVIC_InitType NVIC_InitStructure;
    /* Configure the NVIC Preemption Priority Bits */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
    /* Enable the USARTy Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel            = USARTy_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd         = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}
#endif

void uart_DMA_Configuration(void)
{
    DMA_InitType DMA_InitStructure;
    DMA_DeInit(USARTy_Rx_DMA_Channel);
    /* USARTy RX DMA1 Channel (triggered by USARTy Rx event) Config */
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.CircularMode   = DMA_MODE_CIRCULAR;
    DMA_InitStructure.Priority       = DMA_PRIORITY_VERY_HIGH;
    DMA_InitStructure.Mem2Mem        = DMA_M2M_DISABLE;

    DMA_InitStructure.PeriphAddr = USARTy_DAT_Base;
    DMA_InitStructure.MemAddr    = (uint32_t)DmaRxBuffer;
    DMA_InitStructure.Direction  = DMA_DIR_PERIPH_SRC;
    DMA_InitStructure.BufSize    = MAX_DMA_RX_SIZE;
    DMA_Init(USARTy_Rx_DMA_Channel, &DMA_InitStructure);
    DMA_RequestRemap(USARTy_Rx_DMA_REMAP, DMA, USARTy_Rx_DMA_Channel, ENABLE);
}

uint16_t get_in_pos(void)
{
    return  MAX_DMA_RX_SIZE - DMA_GetCurrDataCounter(USARTy_Rx_DMA_Channel); 
}

uint16_t get_unhdl_data_len(void)
{  
    uint16_t rx_in_pos =  get_in_pos(); 
    if(rx_in_pos >=  rx_out_pos)
    {
        return rx_in_pos - rx_out_pos;
    }
    else
    {
        return rx_in_pos +  MAX_DMA_RX_SIZE - rx_out_pos;
    }   
}  



void fetch_data(uint8_t recv_len , uint8_t * recv_data)
{
    if( (rx_out_pos + recv_len) >= MAX_DMA_RX_SIZE)
    {
        memcpy(recv_data, &DmaRxBuffer[rx_out_pos], MAX_DMA_RX_SIZE - rx_out_pos);
        memset( &DmaRxBuffer[rx_out_pos],0, MAX_DMA_RX_SIZE - rx_out_pos);
        memcpy(recv_data  + MAX_DMA_RX_SIZE - rx_out_pos, &DmaRxBuffer[0], rx_out_pos + recv_len - MAX_DMA_RX_SIZE);
        memset( &DmaRxBuffer[0] ,0, rx_out_pos + recv_len - MAX_DMA_RX_SIZE);
        rx_out_pos = rx_out_pos + recv_len - MAX_DMA_RX_SIZE;
    }
    else
    {
        memcpy(recv_data, &DmaRxBuffer[rx_out_pos], recv_len);
        memset(&DmaRxBuffer[rx_out_pos] ,0, recv_len);
        rx_out_pos = rx_out_pos + recv_len;   
    }
}  


uint8_t fetch_one_byte_data(void)
{
    uint8_t byte = 0;
    fetch_data(1,  &byte);
    return byte;
}

#if 0
void USARTy_IRQHandler(void)
{
    if (USART_GetIntStatus(USARTy, USART_INT_RXDNE) != RESET)
    {
        NS_LOG_DEBUG("%02x\r\n", USART_ReceiveData(USARTy));
    }
}    
#endif

void app_uart_init(void)
{
     /* Enable Dma clock */
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);

    /* Enable GPIO clock */
    GPIO_APBxClkCmd(USARTy_GPIO_CLK, ENABLE);

    /* Enable USARTx Clock */
    USART_APBxClkCmd(USARTy_CLK, ENABLE);
    
    GPIO_InitType GPIO_InitStructure;

    /* Initialize GPIO_InitStructure */
    GPIO_InitStruct(&GPIO_InitStructure);
    
    /* Configure USARTy Rx as alternate function push-pull and pull-up */
    GPIO_InitStructure.Pin            = USARTy_RxPin;
    GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Alternate = USARTy_Rx_GPIO_AF;
    GPIO_InitPeripheral(USARTy_RxPort, &GPIO_InitStructure);   
     
     /* Configure USARTy Tx as alternate function push-pull */
    GPIO_InitStructure.Pin            = USARTy_TxPin;    
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Alternate = USARTy_Tx_GPIO_AF;
    GPIO_InitPeripheral(USARTy_TxPort, &GPIO_InitStructure);
    
    uart_DMA_Configuration();

    USART_InitType USART_InitStructure;

    /* USARTy configuration ------------------------------------------------------*/
    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.BaudRate            = 115200;
    USART_InitStructure.WordLength          = USART_WL_8B;
    USART_InitStructure.StopBits            = USART_STPB_1;
    USART_InitStructure.Parity              = USART_PE_NO;
    USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
    USART_InitStructure.Mode                = USART_MODE_RX | USART_MODE_TX;

    /* Configure USARTy */
    USART_Init(USARTy, &USART_InitStructure);
    
    USART_EnableDMA(USARTy, USART_DMAREQ_RX , ENABLE);    
    DMA_EnableChannel(USARTy_Rx_DMA_Channel, ENABLE);

    /* Enable the USARTy */
    USART_Enable(USARTy, ENABLE);
    
    rx_out_pos = 0;
}

