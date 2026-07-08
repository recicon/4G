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
 * @file app_flash.c
 * @author Nations
 * @version V1.2.2
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
 
#include "app_flash.h"



#define REG32(addr)                 (*(volatile uint32_t *)(addr))

#define CONFIG_BLE_END_FLAG_DATA    0xCDEF89AB
#define FLASH_WRITE_START_ADDR      ((uint32_t)0x0801FC00)

void  set_store_crc(uint16_t crc_val)
{    
    /* Configures the Internal High Speed oscillator */
    if(FLASH_HSICLOCK_DISABLE == FLASH_ClockInit())
    {
        while(1);
    }

    /* Unlocks the FLASH Program Erase Controller */
    FLASH_Unlock();
    /* Erase */
    if (FLASH_COMPL != FLASH_EraseOnePage(FLASH_WRITE_START_ADDR))
    {
       
        while(1);
    }


    if (FLASH_COMPL != FLASH_ProgramWord(FLASH_WRITE_START_ADDR, crc_val))
    {
       
        while(1);
    }


    /* Locks the FLASH Program Erase Controller */
    FLASH_Lock();
    
    
}

uint16_t  get_store_crc(void)
{
    uint16_t crc_value = REG32(FLASH_WRITE_START_ADDR) & 0xFFFF;
    return crc_value;
}    

