/*********************************************************************************************
**  Copyright (C), 2022, HT Technology INC.
**  All rights reserved.
**
**  File Name: 	    xxx .h
**  File Number: 
**  Author:		     lxk
**  Version: 	     V1.0    
**  Date:   		 2023.04.01    
**  Description:                    
**  Others:		 
**  Function List:      
**********************************************************************************************
**  Revision History           
**  1
**  Date:            
**  Author:          
**  Modification:      
**  2
**********************************************************************************************/


#pragma push
#pragma O0
/**********************************************************************************************
 *                             Include File
**********************************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "SifModule.h"
#include "n32wb43x_gpio.h"
#include "n32wb43x_tim.h"
#include "App.h"
/**********************************************************************************************
*							   Macro Define
**********************************************************************************************/
#define SIF_RX_PORT GPIOB
#define SIF_RX_PIN GPIO_PIN_11
#define SIF_TX_PORT GPIOB
#define SIF_TX_PIN GPIO_PIN_10

  
#define SIF_INPUT_EXTI_LINE EXTI_LINE11
#define SIF_INPUT_IRQn      EXTI15_10_IRQn

#define SIF_BASE_T2         (500)


#define sif_turn_off() GPIO_ResetBits(SIF_TX_PORT, SIF_TX_PIN)
#define sif_turn_on()  GPIO_SetBits(SIF_TX_PORT, SIF_TX_PIN)
/* 宏定义 ---------------------------------------------------------------------*/
#define DATA_REV_PIN        GPIO_ReadInputDataBit(SIF_RX_PORT,SIF_RX_PIN)
#define LOW                     0       //低电平
#define HIGH                    1       //高电平

#define SYNC_L_TIME_NUM         1100    //同步信号低电平时间：50ms = 50000us / 50us = 1000
#define SYNC_H_TIME_NUM_MIN     8      //同步信号高电平最小时间：500-100us = 400us / 50us = 8  
#define SYNC_H_TIME_NUM_MAX     12     //同步信号高电平最大时间：500+100us = 600us / 50us = 12

#define SHORT_TIME_NUM_MIN      9     //一个逻辑周期中短的时间最小值：500-50us = 450us / 50us = 9
#define SHORT_TIME_NUM_MAX      11    //一个逻辑周期中短的时间最大值：500+50us = 550us / 50us = 11

#define LONG_TIME_NUM_MIN       18    //一个逻辑周期中长的时间最小值：1ms-100us = 900us / 50us = 18
#define LONG_TIME_NUM_MAX       22    //一个逻辑周期中长的时间最大值：1ms+100us = 1100us / 50us = 22

#define LOGIC_CYCLE_NUM_MIN     26    //一个逻辑周期最小时间：1.5ms-200us = 1300us / 50us = 26
#define LOGIC_CYCLE_NUM_MAX     34    //一个逻辑周期最大时间：1.5ms+200us = 1700us / 50us = 34

#define HALF_LOGIC_CYCLE_MIN    13    //一个逻辑周期的1/2最小时间：750-100us = 650us / 50us = 13
#define HALF_LOGIC_CYCLE_MAX    17    //一个逻辑周期的1/2最大时间：750+100us = 850us / 50us = 17

#define END_SIGNAL_TIME_NUM     100   //结束信号电平时间：5ms低电平 + Nms高电平，实际检测5ms低电平就行，一帧数据发送完成后检测5ms低电平就代表完成了，不发数据的时候上拉电阻拉高了

#define REV_BIT_NUM             8     //接收的bit位个数，看是按字节接收还是按字接收，1字节=8bit，1字=2字节=16bit
#define REV_DATA_NUM            12    //接收的数据个数

/**********************************************************************************************
*							   		Type Define
**********************************************************************************************/

///**********************************************************************************************
//*							   Variable Declaration
//**********************************************************************************************/
uint8_t g_Sif_SendOrRecv =0;
stSifSendData g_stSifData;
///**********************************************************************************************
//*                              Function Declaration
//**********************************************************************************************/
void EXTI_DRV_Init(FunctionalState enState)
{
    NVIC_InitType NVIC_InitStructure;
     /*Set key input interrupt priority*/
    NVIC_InitStructure.NVIC_IRQChannel                   = SIF_INPUT_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x06;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = enState;
    NVIC_Init(&NVIC_InitStructure);    
    
}
void Sif_Port_Init(void)
{
    //port初始化
    GPIO_InitType GPIO_InitStructure;
    EXTI_InitType EXTI_InitStructure;   
 
    GPIO_InitStruct(&GPIO_InitStructure);
    
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB , ENABLE);

    //rx
    GPIO_InitStructure.Pin        = SIF_RX_PIN ;
    GPIO_InitStructure.GPIO_Pull = GPIO_No_Pull;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Input;
    GPIO_InitPeripheral(SIF_RX_PORT, &GPIO_InitStructure);
    //tx
    GPIO_InitStructure.Pin = SIF_TX_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitPeripheral(SIF_TX_PORT, &GPIO_InitStructure);
    
    
    /*Configure key EXTI Line to key input Pin*/
    GPIO_ConfigEXTILine(GPIOB_PORT_SOURCE, GPIO_PIN_SOURCE11);

    /*Configure key EXTI line*/
    EXTI_InitStructure.EXTI_Line    = SIF_INPUT_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStructure);

    EXTI_DRV_Init(ENABLE);
}

/************************************************************************************
 功 能：初始化时钟
 参 数：无
 返 回：无
*************************************************************************************/
void Sif_Tim2_Init(void)
{
    TIM_TimeBaseInitType TIM_TimeBaseStructure;
    uint16_t PrescalerValue = 0;
    NVIC_InitType NVIC_InitStructure;

    /* PCLK1= HCLK/4 */
    RCC_ConfigPclk1(RCC_HCLK_DIV4);

      /* TIM2 clock enable */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM2, ENABLE);
    
 /* Enable the TIM2 global Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel                   = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;

    NVIC_Init(&NVIC_InitStructure);
    

    /* Compute the prescaler value */
    PrescalerValue = 0;

    /* Time base configuration */
    TIM_TimeBaseStructure.Period    = 65535;
    TIM_TimeBaseStructure.Prescaler = 0;
    TIM_TimeBaseStructure.ClkDiv    = 0;
    TIM_TimeBaseStructure.CntMode   = TIM_CNT_MODE_UP;

    TIM_InitTimeBase(TIM2, &TIM_TimeBaseStructure);

    /* Prescaler configuration */
    TIM_ConfigPrescaler(TIM2, PrescalerValue, TIM_PSC_RELOAD_MODE_IMMEDIATE);

    /* TIM1 enable update irq */
    TIM_ConfigInt(TIM2, TIM_INT_UPDATE, ENABLE);

    /* TIM1 enable counter */
    TIM_Enable(TIM2, ENABLE);    
}
void UpdateTimeUs(uint16_t u16UsTime)
{
    RCC_ClocksType RCC_ClocksStatus;
    //SetSysClockToHSE();
    RCC_GetClocksFreqValue(&RCC_ClocksStatus);
    uint32_t u32HClkFreq =RCC_ClocksStatus.HclkFreq;
    uint16_t u16CntValue =  (u32HClkFreq/1000000)*u16UsTime;
    
    //Bt_M0_ARRSet(eTimUnit, u16ArrValue);				//设置重载值(周期 = 0x10000 - ARR)
    TIM_SetAutoReload(TIM1, u16CntValue);				//设置计数初值
}
/************************************************************************************
 功 能：初始化SIF总线状态
 参 数：无
 返 回：无
*************************************************************************************/
void Sif_Init(void)
{
    Sif_Port_Init();
    Sif_Tim2_Init();
    UpdateTimeUs(SIF_BASE_T2);
}

/**********************************************************************************************
 * EOF
**********************************************************************************************/
#define SIF_VERSION 1
#define SIF_SYNC  31
#define SIF_SEND_COUNT 3
#define SIF_COMPLETE  12

uint8_t result[64] = { 0 };//需要发送的数据
uint8_t length = 0;//数据的长度
void sif_send_data_handle()
{    
    static uint8_t sif_sync_tosc = 0;
    static uint8_t sif_send_tosc = 0;
    static uint8_t state_mode = SYNC_SIGNAL;
    static int8_t bit_cnt = 7;
    static uint8_t byte_cnt = 0;
    uint8_t count,nums;
    uint8_t *p = (uint8_t *)result;
	switch (state_mode)
	{
	case SYNC_SIGNAL://同步模式
		if (sif_sync_tosc < SIF_SYNC - 1)
        {
			sif_turn_off();
		}
		else {
			sif_turn_on();
		}
		sif_sync_tosc++;
		if (sif_sync_tosc >= SIF_SYNC)
		{
			state_mode = SEND_DATA;
			sif_sync_tosc = 0;
			bit_cnt = 7;
			byte_cnt = 0;
			sif_send_tosc = 0;
		}
		break;
	case SEND_DATA:    //发送数据
		//static uint8_t res;
		count = SIF_SEND_COUNT;
		nums = sizeof(uint8_t)* 8;
		
		sif_send_tosc = sif_send_tosc % count;

		uint8_t data = (p[byte_cnt] >> bit_cnt) & 0x1;

		if (data)
		{
			if (sif_send_tosc == 0)
			{
				sif_turn_off();
				sif_send_tosc++;
			}
			else if (sif_send_tosc == 1)
			{
				sif_turn_on();
				sif_send_tosc++;
			}
			else {
				sif_send_tosc = 0;
			}
		}
		else
		{
			if (sif_send_tosc == 0)
			{
				sif_turn_off();
				sif_send_tosc++;
			}
			else if (sif_send_tosc == 2)
			{
				sif_turn_on();
				sif_send_tosc = 0;
			}
			else
			{
				sif_send_tosc++;
			}
		}
		if (sif_send_tosc == 0)
		{
			if (--bit_cnt < 0)
			{
				byte_cnt++;
				bit_cnt = 7;
			}
			if (byte_cnt >= length)
			{
				state_mode = SEND_DATA_COMPLETE;
				break;
			}
		}
		break;
    case SEND_DATA_COMPLETE://数据发送完成，将标志位清0
		if(sif_send_tosc++<SIF_COMPLETE)
            sif_turn_off();
        else
        {
            sif_turn_on();
            state_mode=SEND_END;
        }

		break;    
        
	case SEND_END://数据发送完成，将标志位清0
		state_mode = SYNC_SIGNAL;
		sif_turn_off();
		length = 0;
		memset(result, 0, sizeof(result));
         g_Sif_SendOrRecv=0; 
		break;
	default:
		break;
	}
}

/*******************************************************************************
*函数名称 : Check_Sum_Handle
*函数功能 : 校验和处理
*输入参数 : void
*输出返回 : void
*******************************************************************************/
unsigned char receive_data_buf[REV_DATA_NUM] = { 0 };
uint8_t read_success = 0;                 //一帧数据是否读取成功，0-不成功，1-成功
bool Check_Sum_Handle(void)
{
	unsigned char i = 0, checkByte = 0;
	unsigned long checkXor = 0;
    bool check_OK=0;
	for (i = 0; i < (REV_DATA_NUM); i++)
	{
		checkXor = checkXor ^ receive_data_buf[i];
	}

	checkByte = (unsigned char)checkXor;

	if (checkByte == receive_data_buf[REV_DATA_NUM - 1])  //校验和正确
	{
		check_OK = 1;           //标记校验成功
	}
	else
	{
		check_OK = 0;           //标记校验失败
	}
  return  check_OK; 
}

/*******************************************************************************
*函数名称 : GetSIFData
*函数功能 : SIF 解析函数，在主循环中调用
*输入参数 : void
*输出返回 : void
*******************************************************************************/
uint8_t GetSifData(uint8_t* pSifData)
{
    
	if (read_success == 1)              //如果成功读取一帧数据
	{
		//如果数据正确，根据接收的数据进行分析获取需要的内容
		if (Check_Sum_Handle()==true)
		{
			memcpy(pSifData, receive_data_buf, REV_DATA_NUM);
		}

		read_success = 0;               //读取一帧数据清0
        
	}
    return REV_DATA_NUM;
}

/*******************************************************************************
*函数名称 : sif_receive_data_handle
*函数功能 : 接收数据处理
*输入参数 : void
*输出返回 : void
*******************************************************************************/
void sif_receive_data_handle(void)
{
   static unsigned char receive_state = 0;      //接收数据状态
   static unsigned char receive_bit_num = 0;    //接收的bit位个数
   static unsigned char receive_data_num = 0;   //接收的数据个数
   static unsigned int  H_L_Level_time_cnt = 0; //高低电平时间计数
   static uint8_t start_H_L_Level_timming_flag = 0; //开始高低电平计时标记
   static uint8_t has_read_bit = 0;               //1-已经读取一个bit位
	switch (receive_state)                          //检测当前接收数据状态
	{
	case INITIAL_STATE:                         //初始状态，未接收到同步信息，进行同步判断
		if (DATA_REV_PIN == LOW)                //判断接收引脚的电平状态，当读到低电平时，开始计时
		{
			receive_bit_num = 0;                //重置bit位计数器
			receive_data_num = 0;               //重置接收数据个数
			H_L_Level_time_cnt = 0;             //高低电平计时变量清0
			start_H_L_Level_timming_flag = 1;   //开始高低电平计时
			receive_state = SYNC_L_STATE;       //进入读取同步低电平信号状态

			memset(receive_data_buf, 0, REV_DATA_NUM);
		}
		break;

	case SYNC_L_STATE:                          //在读取同步低电平信号期间
		if (DATA_REV_PIN == HIGH)               //同步信号低电平检测期间读到高电平
		{
			if (H_L_Level_time_cnt >= SYNC_L_TIME_NUM)//如果同步信号低电平时间>=SYNC_L_TIME_NUM
			{                                       //同步信号低电平时间要>=10ms
				H_L_Level_time_cnt = 0;         //高低电平计时变量清0
				receive_state = SYNC_H_STATE;   //进入读取同步信号高电平状态
			}
			else
			{
				receive_state = RESTART_REV_STATE;      //进入重新接收状态  
			}
		}
		break;

	case SYNC_H_STATE:                          //在读取同步信号高电平期间
		if (DATA_REV_PIN == LOW)                //同步信号高电平检测期间读到低电平
		{
			//判断同步信号高电平时间是否在1ms±100us之间
			if (H_L_Level_time_cnt >= SYNC_H_TIME_NUM_MIN && H_L_Level_time_cnt <= SYNC_H_TIME_NUM_MAX)
			{
				H_L_Level_time_cnt = 0;         //高低电平计时变量清0
				receive_state = DATA_REV_STATE; //进入读取数据状态
			}
			else
			{
				receive_state = RESTART_REV_STATE;      //进入重新接收状态
			}
		}
		else            //如果在同步信号高电平检测期间，时间超过2ms±200us，认为超时
		{
			//判断时间是否超时 2ms±200us
			if (H_L_Level_time_cnt >= LOGIC_CYCLE_NUM_MAX)
			{
				receive_state = RESTART_REV_STATE;      //进入重新接收状态
			}
		}
		break;

	case DATA_REV_STATE:          //在读取数据码电平期间
		if ((has_read_bit == 0) && (H_L_Level_time_cnt >= HALF_LOGIC_CYCLE_MIN && H_L_Level_time_cnt <= HALF_LOGIC_CYCLE_MAX))
		{
			receive_data_buf[receive_data_num] |= DATA_REV_PIN;

			has_read_bit = 1;
		}

		//如果已经读取一个bit位，且时间计数已经>=2ms±200us，说明一个逻辑周期过去了
		if ((has_read_bit == 1) && (H_L_Level_time_cnt >= LOGIC_CYCLE_NUM_MIN && H_L_Level_time_cnt <= LOGIC_CYCLE_NUM_MAX))
			//if ((has_read_bit==1) && (1 == BitFinish_Flag))
		{
			H_L_Level_time_cnt = 0;             //高低电平计时变量清0
			has_read_bit = 0;                   //清0，读取下一个bit位
			receive_bit_num++;                  //接收的bit数++

			if (receive_bit_num == REV_BIT_NUM)   //如果一个字节8个bit位接收完成
			{
				//receive_data[receive_data_num] = receive_data_buf[receive_data_num];

				receive_data_num++;             //接收的数据个数++
				receive_bit_num = 0;            //接收bit位个数清0重新接收

				if (receive_data_num == REV_DATA_NUM)   //如果数据采集完毕
				{
					receive_state = END_SIGNAL_STATE;   //进入接收结束低电平信号状态
				}
			}
			else                                //如果一个字节8个bit位还没有接收完成
			{
				//将接收数据缓存左移一位，数据从低bit位开始接收
				receive_data_buf[receive_data_num] = receive_data_buf[receive_data_num] >> 1;
			}
		}
		break;

	case END_SIGNAL_STATE:                              //在接收结束信号低电平期间
		if (DATA_REV_PIN == LOW)
		{
			if (H_L_Level_time_cnt >= END_SIGNAL_TIME_NUM)  //如果读到低电平时间>=5ms
			{
				read_success = 1;                   //一帧数据读取成功
				start_H_L_Level_timming_flag = 0;   //停止高低电平计时
				H_L_Level_time_cnt = 0;             //定时器计数值清0
				receive_state = INITIAL_STATE;      //接收状态清0  
			}
		}
		else    //结束信号低电平检测期间一直为低
		{
			//if (H_L_Level_time_cnt >= SYNC_L_TIME_NUM)  //如果读到低电平时间>=10ms，认为超时
			{                                           //一帧数据发送完成后需要间隔50ms才发送第二帧数据，期间肯定会被拉高
				receive_state = RESTART_REV_STATE;      //进入重新接收状态
			}
		}
		break;

	case RESTART_REV_STATE:                     //重新接收数据状态
		start_H_L_Level_timming_flag = 0;       //停止高低电平计时
		H_L_Level_time_cnt = 0;                 //定时器计数值清0
		receive_state = INITIAL_STATE;          //接收状态清0   
        g_Sif_SendOrRecv=0;    
		break;
	}
}

///*******************************************************************************
// *函数名称 : Timer0_isr
// *函数功能 : 定时器0中断处理函数
// *输入参数 : void
// *输出返回 : void
//*******************************************************************************/
void Sif_Timer_isr() //SIF定时器
{
    if (g_Sif_SendOrRecv==1)
    {
        sif_send_data_handle();     //发送数据处理
    }   
    else if(g_Sif_SendOrRecv==2)
    {
        sif_receive_data_handle();      //接收数据处理，波特率自适应
    }
}
 //定时中断
void TIM2_IRQHandler(void)
{
    if (TIM_GetIntStatus(TIM2, TIM_INT_UPDATE) != RESET)
    {
        TIM_ClrIntPendingBit(TIM2, TIM_INT_UPDATE);

        //触发数据
        Sif_Timer_isr();
    }
}
extern bool g_bWakeBleUart;
//检测到一线通串口接收下降沿开启接收数据
void EXTI15_10_IRQHandler(void)
{
    if (RESET != EXTI_GetITStatus(SIF_INPUT_EXTI_LINE))
    {
        if(DATA_REV_PIN == RESET)
        {
            EXTI_DRV_Init(DISABLE);
						//g_bWakeBleUart = true;
        }	
        //EXTI_ClrITPendBit(SIF_INPUT_EXTI_LINE);				
    }
}

/************************************************************************************
 功 能：检验主机命令有效性
 参 数：无
 返 回：1:命令有效
		0:命令无效
*************************************************************************************/
uint8_t Check_command(uint8_t *pData,uint8_t byLength)
{
	uint8_t i, checksum = 0;
	
	for(i = 0; i < byLength; i++)
	{
		checksum += pData[i];
	}
	
	return checksum;
}

void SendSifData(void)
{   
    if (g_Sif_SendOrRecv>0) return;
    uint8_t byTemp;
		uint32_t wTemp;
    g_Sif_SendOrRecv =1;
    switch(g_tBatPackCtrl.tEESpec.byProtocolVer)
    {
        case 0:
        {
            g_stSifData.byCur = g_tBatPackInfo.wAbsCur/10;
            g_stSifData.byVol = g_tBatPackInfo.wBatVolt/10;
            g_stSifData.bySoc= g_tBatPackCtrl.tEERun.wSocShow/100;
					switch(g_tBatPackCtrl.tEESpec.byBatType)
					{
						case BAT_TYPE_NCM:
							wTemp=NCM_RATED_CELL_VOLT;
						break;
						case BAT_TYPE_FELI:
							wTemp=FELI_RATED_CELL_VOLT;
						break;
						default:
							wTemp=FELI_RATED_CELL_VOLT;
						break;							
					}
						byTemp=(g_tBatPackCtrl.tEESpec.byPackCell*wTemp+6000)/12000;
            g_stSifData.byState=(byTemp<<4)|g_tBatPackInfo.eCtrlMode;
            g_stSifData.byTemp=g_tBatPackInfo.tCell.byTempMin>40?g_tBatPackInfo.tCell.byTempMax:g_tBatPackInfo.tCell.byTempMin;
            g_stSifData.byCheck =Check_command((uint8_t*)&g_stSifData,sizeof(g_stSifData)-1);
            memcpy(result, &g_stSifData, sizeof(g_stSifData));  
            length=sizeof(g_stSifData);   
        } 
        break;
        case 1:
        {
            result[0]=0x03;
            result[1]=0x01;
            byTemp=0;
            byTemp|=(g_tBatPackInfo.tBmFltVal.tBits.u1ShortFlg<<2);
            byTemp|=((g_tBatPackInfo.tBmDiagVal.tBits.u1RelayAdhesionFlg||g_tBatPackInfo.tFaultLevel.byMosTempHigh)<<3);  
            byTemp|=((g_tBatPackInfo.tFaultLevel.byECellTempHigh>2)>>6);
            byTemp|=((g_tBatPackInfo.tFaultLevel.byECellTempLow>2)>>6);
            byTemp|=g_tBatPackCtrl.tEERun.dvBatCycleCnt*100/g_tBatPackCtrl.tEESpec.wHoldingTime>60;
            byTemp|=(1>>7);
            result[2]=byTemp; 
            result[3]=g_tBatPackInfo.tCell.wVolMax/10-185;
            result[4]=g_tBatPackInfo.wSocShow/100;
            result[5]=g_tBatPackInfo.wBatVolt/10;
            byTemp=g_tBatPackInfo.tCell.byTempMax;
            result[6]=byTemp>=40?byTemp-40:((40-byTemp)+(1<<7));
            byTemp=g_tBatPackInfo.tCell.byTempMin;
            result[7]=byTemp>=40?byTemp-40:((40-byTemp)+(1<<7));
            result[8]=g_tBatPackCtrl.tEERun.dvBatCycleCnt<<8;
            result[9]=g_tBatPackCtrl.tEERun.dvBatCycleCnt;    
            result[10]=g_tBatPackInfo.tCell.wVolMin/10-185;
            byTemp=0;
            for(int i =0;i<11;i++ )
                byTemp^=result[i];    
            result[11]=byTemp;
            length =12;
        }
        break;
        default:
         g_Sif_SendOrRecv =0;
         length=0;
        break;
    }
}

#pragma pop
