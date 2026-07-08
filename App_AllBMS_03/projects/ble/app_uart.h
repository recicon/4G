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
 * @file app_uart.h
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */

#include "n32wb43x.h"

#ifndef APP_UART_H_
#define APP_UART_H_

//#define           UART5_USED           1

#if (UART5_USED)
///use for hci 
#define USARTy            UART5
#define USARTy_GPIO       GPIOB
#define USARTy_CLK        RCC_APB2_PERIPH_UART5
#define USARTy_GPIO_CLK   (RCC_APB2_PERIPH_GPIOD | RCC_APB2_PERIPH_GPIOC)
#define USARTy_RxPin      GPIO_PIN_2
#define USARTy_RxPort     GPIOD
#define USARTy_TxPin      GPIO_PIN_12
#define USARTy_TxPort     GPIOC

#define USARTy_Rx_GPIO_AF GPIO_AF6_UART5
#define USARTy_Tx_GPIO_AF GPIO_AF6_UART5
#define USARTy_APBxClkCmd RCC_EnableAPB2PeriphClk
#define USARTy_IRQn       UART5_IRQn
#define USARTy_IRQHandler UART5_IRQHandler 

#define GPIO_APBxClkCmd   RCC_EnableAPB2PeriphClk
#define USART_APBxClkCmd  RCC_EnableAPB2PeriphClk

#define USARTy_DAT_Base       (UART5_BASE + 0x04)
#define USARTy_Rx_DMA_Channel DMA_CH1
#define USARTy_Rx_DMA_REMAP   DMA_REMAP_UART5_RX
#else
///use for hci 
#define USARTy            UART4
#define USARTy_CLK        RCC_APB2_PERIPH_UART4
#define USARTy_GPIO_CLK   RCC_APB2_PERIPH_GPIOB
#define USARTy_RxPin      GPIO_PIN_15
#define USARTy_RxPort     GPIOB
#define USARTy_TxPin      GPIO_PIN_14
#define USARTy_TxPort     GPIOB

#define USARTy_Rx_GPIO_AF GPIO_AF6_UART4
#define USARTy_Tx_GPIO_AF GPIO_AF6_UART4
#define USARTy_APBxClkCmd RCC_EnableAPB2PeriphClk
#define USARTy_IRQn       UART4_IRQn
#define USARTy_IRQHandler UART4_IRQHandler

#define GPIO_APBxClkCmd   RCC_EnableAPB2PeriphClk
#define USART_APBxClkCmd  RCC_EnableAPB2PeriphClk

#define USARTy_DAT_Base       (UART4_BASE + 0x04)
#define USARTy_Rx_DMA_Channel DMA_CH1
#define USARTy_Rx_DMA_REMAP   DMA_REMAP_UART4_RX

#endif

void app_uart_init(void);
uint16_t get_unhdl_data_len(void);  //store_data_pos;
void app_send_byte_array(uint8_t * buf, uint16_t length);
void fetch_data(uint8_t recv_len , uint8_t * recv_data);
uint8_t fetch_one_byte_data(void);

#endif //AHI_H_
