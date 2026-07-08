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
 * @file app_io.c
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
 
#include "app_io.h"
#include "n32wb43x_it.h"

#define RESET_PORT      GPIOC
#define RESET_PIN       GPIO_PIN_6

#define WAKEUP_PORT     GPIOC
#define WAKEUP_PIN      GPIO_PIN_9

#define STATUS_PORT     GPIOC
#define STATUS_PIN      GPIO_PIN_8

#define IRQ_PORT        GPIOC
#define IRQ_PIN         GPIO_PIN_7


#define KEY_INPUT_EXTI_LINE     EXTI_LINE7
#define KEY_INPUT_PORT_SOURCE   GPIOC_PORT_SOURCE
#define KEY_INPUT_PIN_SOURCE    GPIO_PIN_SOURCE7
#define KEY_INPUT_IRQn          EXTI9_5_IRQn


void app_reset_ble(void)
{
    GPIO_SetBits(RESET_PORT, RESET_PIN);
    Delay_ms(10);
    GPIO_ResetBits(RESET_PORT, RESET_PIN);
    Delay_ms(10);
    GPIO_SetBits(RESET_PORT, RESET_PIN);    
}   


void app_pullup_wakeup_io(void)
{
    GPIO_SetBits(WAKEUP_PORT, WAKEUP_PIN);
}    

void app_pulldown_wakeup_io(void)
{
    GPIO_ResetBits(WAKEUP_PORT, WAKEUP_PIN);
}                

uint8_t app_get_irq_io(void)
{
    return GPIO_ReadInputDataBit(IRQ_PORT, IRQ_PIN);
}

uint8_t app_get_stauts_io(void)
{
    return GPIO_ReadInputDataBit(STATUS_PORT, STATUS_PIN);
}

void app_io_init(void)
{
    ///
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR,  ENABLE);

    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOC | RCC_APB2_PERIPH_AFIO, ENABLE);
    
    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    
    /// init reset io
    GPIO_InitStructure.Pin          = RESET_PIN;
    GPIO_InitStructure.GPIO_Pull    = GPIO_No_Pull;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_Out_PP;
    GPIO_InitPeripheral(RESET_PORT, &GPIO_InitStructure);
    
    /// init wakeup ble io
    GPIO_InitStructure.Pin          = WAKEUP_PIN;
    GPIO_InitStructure.GPIO_Pull    = GPIO_No_Pull;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_Out_PP;
    GPIO_InitPeripheral(WAKEUP_PORT, &GPIO_InitStructure);

    /// init ble state io
    GPIO_InitStructure.Pin          = STATUS_PIN;
    GPIO_InitStructure.GPIO_Pull    = GPIO_No_Pull;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_Input;
    GPIO_InitPeripheral(STATUS_PORT, &GPIO_InitStructure);
    
    /// init ble irq io;  wakeup host
    GPIO_InitStruct(&GPIO_InitStructure);
    GPIO_InitStructure.Pin        = IRQ_PIN;
    GPIO_InitStructure.GPIO_Pull    = GPIO_Pull_Down;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Input;
    GPIO_InitPeripheral(IRQ_PORT, &GPIO_InitStructure);
    
    
    GPIO_ConfigEXTILine(KEY_INPUT_PORT_SOURCE, KEY_INPUT_PIN_SOURCE);
    EXTI_InitType EXTI_InitStructure;
    
    /*Configure key EXTI line*/
    EXTI_InitStructure.EXTI_Line    = KEY_INPUT_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;//EXTI_Trigger_Falling; // ;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStructure);

    NVIC_InitType NVIC_InitStructure;
    /*Set key input interrupt priority*/
    NVIC_InitStructure.NVIC_IRQChannel                   = KEY_INPUT_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x05;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}   
