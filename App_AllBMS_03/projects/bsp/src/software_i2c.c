/*****************************************************************************
 * Copyright (c) 2019, Nations Technologies Inc.
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
 * @file software_i2c.c
 * @author Nations Firmware Team
 * @version v1.0.2
 *
 * @copyright Copyright (c) 2019, Nations Technologies Inc. All rights reserved.
 */
#include "software_i2c.h"

#include <stdio.h>




#define GPIOx_MODE_SET(GPIOx,bit,mode) do{     \
GPIOx->POTYPE &= ~(1U << bit);                 \
GPIOx->POTYPE |= (((mode >> 4U) & 1U) << bit); \
GPIOx->PMODE&= ~(03U<<(bit * 2U));             \
GPIOx->PMODE|= (mode& 03U)<<(bit*2U);          \
}while(0);

//IO control 
#define SDA_OUT_MODE(GPIOx,bit)  GPIOx_MODE_SET(GPIOx,bit,GPIO_Mode_Out_OD)    //SDA output
#define SDA_IN_MODE(GPIOx,bit)   GPIOx_MODE_SET(GPIOx,bit,GPIO_Mode_Input)        //SDA input
#define SCL_OUT_MODE(GPIOx,bit)  GPIOx_MODE_SET(GPIOx,bit,GPIO_Mode_Out_PP)    //SCL output
#define SDA_OUT_H(GPIOx,bit)     (GPIOx->PBSC = 1<<bit)                           //SDA output high
#define SDA_OUT_L(GPIOx,bit)     (GPIOx->PBC  = 1<<bit)                           //SDA output low
#define SCL_OUT_H(GPIOx,bit)     (GPIOx->PBSC = 1<<bit)                           //SCL output high
#define SCL_OUT_L(GPIOx,bit)     (GPIOx->PBC  = 1<<bit)                           //SCL output low
#define SDA_IN(GPIOx,bit)        (((GPIOx->PID) & (1<<bit))?1:0)                  //SDA input
 
/* Private constants ---------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/


/**
 * @brief  software i2c init
 * @param  
 * @return  true: success, false: fail.
 * @note   
 */
bool SI2C_Init(SI2C_HANDLE *pHandle, GPIO_Module *SDA_GPIOx, GPIO_Module *SCL_GPIOx,\
                uint32_t SDA_Pin, uint32_t SCL_Pin,uint8_t DelayUS)
{
    GPIO_InitType GPIO_InitStructure;
    uint8_t bit = 0;
    if(pHandle == NULL) 
    {
        return false;
    }
    if(DelayUS < 1) DelayUS = 1;
    if(DelayUS > 100) DelayUS = 100;
    pHandle->DelayUS = DelayUS;
    pHandle->SDA_GPIOx = SDA_GPIOx;                //SDA port
    pHandle->SCL_GPIOx = SCL_GPIOx;                //SCL port

    for(bit = 0; (SDA_Pin >> bit)!=1 ;bit++ )
    {
        if(bit>13)
            break;
    }
    pHandle->SDA_PINx = bit;        //SDA pin
    for(bit = 0; (SCL_Pin >> bit)!=1 ;bit++ )
    {
        if(bit>13)
        break;
    }
    pHandle->SCL_PINx = bit;        //SCL pin
    //init GPIO
    if (SDA_GPIOx == GPIOA)
    {
        RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA, ENABLE);
    }
    else if (SDA_GPIOx == GPIOB)
    {
        RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB, ENABLE);
    }
    if (SCL_GPIOx == GPIOA)
    {
        RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA, ENABLE);
    }
    else if (SCL_GPIOx == GPIOB)
    {
        RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB, ENABLE);
    }
    SDA_OUT_H(pHandle->SDA_GPIOx, pHandle->SDA_PINx);        //SDA=1
    SCL_OUT_H(pHandle->SCL_GPIOx, pHandle->SCL_PINx);        //SCL=1
    GPIO_InitStruct(&GPIO_InitStructure);
    GPIO_InitStructure.Pin = SDA_Pin;
    GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_High;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitPeripheral(SDA_GPIOx, &GPIO_InitStructure);
    GPIO_InitStructure.Pin = SCL_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitPeripheral(SCL_GPIOx, &GPIO_InitStructure);

    SDA_OUT_H(pHandle->SDA_GPIOx, pHandle->SDA_PINx);        //SDA=1
    SCL_OUT_H(pHandle->SCL_GPIOx, pHandle->SCL_PINx);        //SCL=1
    //fucntion init
    pHandle->Start = (void (*)(void *))SI2C_Start; 
    pHandle->Stop  = (void (*)(void *))SI2C_Stop; 
    pHandle->SendByte  = (bool (*)(void *,uint8_t))SI2C_SendByte;
    pHandle->ReadByte  = (uint8_t (*)(void *, bool))SI2C_ReadByte;
 
    return true;
}
 
 
 
/**
 * @brief  software i2c send start singal
 * @param  
 * @return
 * @note   
 */
void SI2C_Start(SI2C_HANDLE *pHandle)
{
    SDA_OUT_MODE(pHandle->SDA_GPIOx, pHandle->SDA_PINx); 
    SDA_OUT_H(pHandle->SDA_GPIOx, pHandle->SDA_PINx); //SDA=1 
    SCL_OUT_H(pHandle->SCL_GPIOx, pHandle->SCL_PINx);//SCL=1
    Delay_us(pHandle->DelayUS);
    SDA_OUT_L(pHandle->SDA_GPIOx, pHandle->SDA_PINx);//SDA=0 
    Delay_us(pHandle->DelayUS);
    SCL_OUT_L(pHandle->SCL_GPIOx, pHandle->SCL_PINx);//SCL=0
}    
 
 
/**
 * @brief  software i2c send stop singal
 * @param  
 * @return
 * @note   
 */
void SI2C_Stop(SI2C_HANDLE *pHandle)
{
    SCL_OUT_L(pHandle->SCL_GPIOx, pHandle->SCL_PINx);//SCL=0
    Delay_us(pHandle->DelayUS);
    SDA_OUT_L(pHandle->SDA_GPIOx, pHandle->SDA_PINx);//SDA=0
    Delay_us(pHandle->DelayUS);
    SCL_OUT_H(pHandle->SCL_GPIOx, pHandle->SCL_PINx);//SCL=1
    Delay_us(pHandle->DelayUS);
    SDA_OUT_H(pHandle->SDA_GPIOx, pHandle->SDA_PINx);//SDA=1    
    Delay_us(pHandle->DelayUS*2);
}
 
 
/**
 * @brief  software i2c wait ack signal
 * @param  
 * @return
 * @note   
 */
bool SI2C_WaitAck(SI2C_HANDLE *pHandle)
{
    uint8_t ucErrTime=0;
    
    SDA_OUT_H(pHandle->SDA_GPIOx, pHandle->SDA_PINx);//SDA=1    
    SDA_IN_MODE(pHandle->SDA_GPIOx, pHandle->SDA_PINx);//SDA input
    Delay_us(pHandle->DelayUS);
    SCL_OUT_H(pHandle->SCL_GPIOx, pHandle->SCL_PINx);//SCL=1
    Delay_us(pHandle->DelayUS);
    
    while(SDA_IN(pHandle->SDA_GPIOx, pHandle->SDA_PINx)) //wait low
    {
        ucErrTime++;
        if(ucErrTime>50)
        {
            SI2C_Stop(pHandle);
            return false;
        }
        Delay_us(5);
    }    
    SCL_OUT_L(pHandle->SCL_GPIOx, pHandle->SCL_PINx);//SCL=0      
    Delay_us(pHandle->DelayUS);
    SDA_OUT_MODE(pHandle->SDA_GPIOx, pHandle->SDA_PINx);//SDA output
    
    return true;  
} 
 
 
/**
 * @brief  software i2c send ack singal
 * @param  
 * @return
 * @note   
 */
void SI2C_Ack(SI2C_HANDLE *pHandle)
{
    SDA_OUT_L(pHandle->SDA_GPIOx, pHandle->SDA_PINx);//SDA=0
    Delay_us(pHandle->DelayUS);   
    SCL_OUT_H(pHandle->SCL_GPIOx, pHandle->SCL_PINx);//SCL=1
    Delay_us(pHandle->DelayUS); 
    SCL_OUT_L(pHandle->SCL_GPIOx, pHandle->SCL_PINx);//SCL=0
}
 
/**
 * @brief  software i2c send nack singal
 * @param  
 * @return
 * @note   
 */  
void SI2C_NAck(SI2C_HANDLE *pHandle)
{    
    SDA_OUT_H(pHandle->SDA_GPIOx, pHandle->SDA_PINx);//SDA=1
    Delay_us(pHandle->DelayUS);
    SCL_OUT_H(pHandle->SCL_GPIOx, pHandle->SCL_PINx);//SCL=1
    Delay_us(pHandle->DelayUS);
    SCL_OUT_L(pHandle->SCL_GPIOx, pHandle->SCL_PINx);//SCL=0      
}    
 
 
 
 
/**
 * @brief  software i2c send a byte
 * @param  
 * @return
 * @note   
 */       
bool SI2C_SendByte(SI2C_HANDLE *pHandle, uint8_t data)
{                        
    uint8_t t; 
    
    for(t=0;t<8;t++)
    {         
        if(data & 0X80)
        {
            SDA_OUT_H(pHandle->SDA_GPIOx, pHandle->SDA_PINx);//SDA=1
        }
        else
        {
            SDA_OUT_L(pHandle->SDA_GPIOx, pHandle->SDA_PINx);//SDA=0
        }
        data <<= 1; 
        Delay_us(pHandle->DelayUS);
        SCL_OUT_H(pHandle->SCL_GPIOx, pHandle->SCL_PINx);//SCL=1
        Delay_us(pHandle->DelayUS);
        SCL_OUT_L(pHandle->SCL_GPIOx, pHandle->SCL_PINx);//SCL=0    
    }    
    return SI2C_WaitAck(pHandle);
}     
 
 
 
/**
 * @brief  software i2c read a byte
 * @param  
 * @return
 * @note   
 */
uint8_t SI2C_ReadByte(SI2C_HANDLE *pHandle,bool isAck)
{
    uint8_t i,receive=0;
    SDA_OUT_H(pHandle->SDA_GPIOx, pHandle->SDA_PINx);//SDA=1 pull up
    SDA_IN_MODE(pHandle->SDA_GPIOx, pHandle->SDA_PINx);//SDA input
    for(i=0;i<8;i++ )
    {
        receive<<=1;
        Delay_us(pHandle->DelayUS); 
        SCL_OUT_H(pHandle->SCL_GPIOx, pHandle->SCL_PINx);//SCL=1
        Delay_us(pHandle->DelayUS);
        if(SDA_IN(pHandle->SDA_GPIOx, pHandle->SDA_PINx))
        {
            receive++;
        }
        __nop();
        SCL_OUT_L(pHandle->SCL_GPIOx, pHandle->SCL_PINx);//SCL=0    
    }
    __nop();__nop();__nop();
    
    SDA_OUT_MODE(pHandle->SDA_GPIOx, pHandle->SDA_PINx);//SDA output 
    if (!isAck)
        SI2C_NAck(pHandle);//send nack
    else
        SI2C_Ack(pHandle); //send ack  
    
    return receive;
}
 
 
/**
 * @brief  software i2c read a reg
 * @param  
 * @return
 * @note   
 */
en_result_t SI2C_ReadReg(SI2C_HANDLE *pHandle, uint8_t SlaveAddr, uint16_t RegAddr,\
                  bool is8bitRegAddr, uint8_t *pDataBuff, uint16_t ReadByteNum)
{
    uint16_t i;
    
    SI2C_Start(pHandle);
    if(SI2C_SendByte(pHandle, SlaveAddr) == false)
    {
 
        return Error;
    }
    if(is8bitRegAddr == false)
    {
        if(SI2C_SendByte(pHandle, RegAddr>>8) == false)
        {
   
            return Error;
        }
    }
    if(SI2C_SendByte(pHandle, RegAddr) == false)
    {

        return Error;
    }

    SI2C_Start(pHandle);
    if(SI2C_SendByte(pHandle, SlaveAddr|1) == false)
    {

        return Error;
    }
    for(i = 0;i < ReadByteNum;i ++)
    {
        if(i == (ReadByteNum-1))
        {
            pDataBuff[i] = SI2C_ReadByte(pHandle, false);
        }
        else
        {
            pDataBuff[i] = SI2C_ReadByte(pHandle, true);
        }    
    }
    SI2C_Stop(pHandle);
    
    return Ok;
}
 
 
/**
 * @brief  software i2c write a reg
 * @param  
 * @return
 * @note   
 */
en_result_t SI2C_WriteReg(SI2C_HANDLE *pHandle, uint8_t SlaveAddr, uint16_t RegAddr,\
                   bool is8bitRegAddr, uint8_t *pDataBuff, uint16_t WriteByteNum)
{
    uint16_t i;
    SI2C_Start(pHandle);
    if(SI2C_SendByte(pHandle, SlaveAddr) == false)
    {

        return Error;
    }
    if(is8bitRegAddr == false)
    {
        if(SI2C_SendByte(pHandle, RegAddr>>8) == false)
        {
     
            return Error;
        }
    }
    if(SI2C_SendByte(pHandle, RegAddr) == false)
    {

        return Error;
    }
    for(i = 0;i < WriteByteNum;i++)
    {
        if(SI2C_SendByte(pHandle, pDataBuff[i]) == false) 
        {

            return Error;
        }
        
    }
    SI2C_Stop(pHandle);
    
    return Ok;
}


/**
 * @brief  software i2c master read
 * @param  
 * @return
 * @note   
 */
bool SI2C_MasterRead(SI2C_HANDLE *pHandle, uint8_t SlaveAddr, uint8_t *pDataBuff, uint16_t ReadByteNum)
{
    uint16_t i;

    SI2C_Start(pHandle);
    if(SI2C_SendByte(pHandle, SlaveAddr|0x01) == false)
    {
    
        return false;
    }
    for(i = 0;i < ReadByteNum;i ++)
    {
        if(i == (ReadByteNum-1))
        {
            pDataBuff[i] = SI2C_ReadByte(pHandle, false);
        }
        else
        {
            pDataBuff[i] = SI2C_ReadByte(pHandle, true);
        }    
    }
    SI2C_Stop(pHandle);
    
    return true;
}
/**
 * @brief  software i2c master write
 * @param  
 * @return
 * @note   
 */
bool SI2C_MasterWrite(SI2C_HANDLE *pHandle, uint8_t SlaveAddr, uint8_t *pDataBuff, uint16_t WriteByteNum)
{
    uint16_t i;
    
    SI2C_Start(pHandle);
    if(SI2C_SendByte(pHandle, SlaveAddr&0xFE) == false)
    {

        return false;
    }

    for(i = 0;i < WriteByteNum;i ++)
    {
        if(SI2C_SendByte(pHandle, pDataBuff[i]) == false) 
        {
    
            return false;
        }
    }
    SI2C_Stop(pHandle);
    
    return true;
}


/**
 * @}
 */
