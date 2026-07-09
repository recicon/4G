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
 * @file n32wb43x_it.c
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#include "n32wb43x_it.h"
#include "n32wb43x.h"
#include"Usart_Com.h"
#include "User_Can_Config.h"


/** @addtogroup n32wb43x_StdPeriph_Template
 * @{
 */

volatile uint64_t Tick_us = 0;  
volatile uint64_t Tick_ms = 0;    



/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief  This function handles NMI exception.
 */
void NMI_Handler(void)
{
}

/**
 * @brief  This function handles Hard Fault exception.
 */
void HardFault_Handler(void)
{
    /* Go to infinite loop when Hard Fault exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles Memory Manage exception.
 */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles Bus Fault exception.
 */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles Usage Fault exception.
 */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles SVCall exception.
 */
void SVC_Handler(void)
{
}

/**
 * @brief  This function handles Debug Monitor exception.
 */
void DebugMon_Handler(void)
{
}

/**
 * @brief  This function handles SysTick Handler.
 */
void SysTick_Handler(void)
{
    Tick_ms++;
    if (g_sMainRtxMsg.Timeout_ms > 0)
        g_sMainRtxMsg.Timeout_ms--;
    if (g_sBleRtxMsg.Timeout_ms > 0)
        g_sBleRtxMsg.Timeout_ms--;
    if (g_sUartRtxMsg.Timeout_ms > 0)
        g_sUartRtxMsg.Timeout_ms--;
#if EN_4GCOMM
    if (g_s4GRtxMsg.Timeout_ms > 0)
        g_s4GRtxMsg.Timeout_ms--;
#endif
}

 void EXTI9_5_IRQHandler(void)
{
    /* CAN/RS485 唤醒已移除;仅清 EXTI7/EXTI8 pending
     * (EXTI7 与 BLE 的 PC7 IRQ 共用 app_io.c,不清 pending 会中断风暴) */
    if (RESET != EXTI_GetITStatus(EXTI_LINE7))
        EXTI_ClrITPendBit(EXTI_LINE7);
    if (RESET != EXTI_GetITStatus(EXTI_LINE8))
        EXTI_ClrITPendBit(EXTI_LINE8);
}

extern volatile bool g_bWakeAfe;

void EXTI4_IRQHandler(void)
{
    if(RESET != EXTI_GetITStatus(EXTI_LINE4))
    {
        EXTI_ClrITPendBit(EXTI_LINE4);
			if(!GPIO_ReadInputDataBit(WAKE_AFE_PORT, WAKE_AFE_PIN))
        g_bWakeAfe = true;
    }
}

/*
 */
void DMA_IRQ_HANDLER(void)
{
}

/**
 * @brief  This function handles CAN RX0 Handler.
 */
void CAN_RX0_IRQHandler(void)
{
    CAN_ReceiveMessage(CAN, CAN_FIFO0, &stcRxFrame);
    u8RxFlag = true;
}

void Delay_us(uint32_t us)
{
    volatile uint32_t loop_count;
    us *= 10; 
    for(loop_count = 0; loop_count < us; loop_count++)
    {
        __NOP(); // 执行空操作，消耗一个周期，使循环更稳定
    }

}

uint64_t GetTick_ms(void)
{
    return Tick_ms;
}


void Delay_ms(uint32_t ms)
{
     uint64_t start = GetTick_ms();
    while (GetTick_ms()  < start + ms) {
     
    }
}

