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

/**********************************************************************************************
 *                             Include File
**********************************************************************************************/
#include "MemoryModule.h"
#include "UserDef.h"
#include "Soc.h"
//#include "PowerEstimation.h"
#include "n32wb43x_it.h"
#include "App.h"
#include <stdlib.h>

extern bool g_bCellCountDone;

/**********************************************************************************************
*							   Macro Define
**********************************************************************************************/
#define  TIME_WAIT_CONNECT          (1000)     // 等待通信的时间 
#define  SOC_REF_CORRLIM            (2000u)     //SOC启动修正阈值(0.01%/bit) -- 准确应用应结合时间因素
#define  SOC_SHOW_CORRLIM           (4000u)     //SOC误差太大 直接修正
/**********************************************************************************************
*							   		Type Define
**********************************************************************************************/

typedef enum
{
    eSocEE = 0,
    eSocTable,
}eSocStep;

typedef struct
{
    float fRefFactor;
    float fShowFactor;
}tSocParam;

///**********************************************************************************************
//*							   Variable Declaration
//**********************************************************************************************/

tSocParam g_tSocParam;

//static uint32_t s_dwTimeOutChk = 0;

// 只在前段部分查表标定，后面平台区，太平缓，不查表
//#if (BAT_TYPE == FeLi90Ah)
    #define FELI_N_OCV_SOC               21
    #define FELI_N_LINEAR_OCV_SOC        11      // 非平台区，查表值基本准确

    #define FELI_OCV_SOC_PRECISE_SATRT   19      // 精准平台区起始
    #define FELI_N_OCV_SOC_PRECISE       3

    const uint16_t c_szOcvFeli[FELI_N_OCV_SOC] =
    {
        2499, 2798, 3002, 3121, 3219, 3234, 3250, 3258, 3264, 3270,
        3275, 3283, 3289, 3295, 3302, 3311, 3320, 3326, 3341, 3382,
        3430
    };

    const uint16_t c_szSocFeli[FELI_N_OCV_SOC] =
        {
            0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500,
            5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 9000, 9500,
            10000
        };

//#endif

//#if (BAT_TYPE == NcmLi90Ah)
     #define NCM_N_OCV_SOC               21
    #define NCM_N_LINEAR_OCV_SOC        11      // 非平台区，查表值基本准确

    #define NCM_OCV_SOC_PRECISE_SATRT   19      // 精准平台区起始
    #define NCM_N_OCV_SOC_PRECISE       3

    const uint16_t c_szOcvNcm[NCM_N_OCV_SOC] =
    {
        2750, 3215, 3348, 3436, 3501, 3552, 3595, 3631, 3663, 3692,
        3744, 3789, 3829, 3867, 3903, 3939, 3976, 4017, 4065, 4126,
        4198
    };

    const uint16_t c_szSocNcm[NCM_N_OCV_SOC] =
    {
        0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500,
        5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 9000, 9500,
        10000
    };

//#endif


tOcvSocTable g_tOcvSocTable;
float g_fRefSoc;                     //SOC计算值
float g_fShowSoc;                    //SOC计算值
static bool s_bFullLatch  = false;   //SOC locked at 100% (full), suppress float-charge creep
static bool s_bEmptyLatch = false;   //SOC locked at 0% (empty), suppress bounce-back
///**********************************************************************************************
//*                              Function Declaration
//*********************************************************************************************/
    //动态修正SOC因子参数
void SetFactor(uint16_t wBatCapacity)
{
     g_tSocParam.fRefFactor = 5.0f / wBatCapacity / 3600;
     g_tSocParam.fShowFactor = g_tSocParam.fRefFactor;
}
void SetSoc(uint16_t wSoc)
{
   g_fRefSoc = wSoc;
   g_fShowSoc = wSoc;
}

uint8_t LookUpTable16Bit(const uint16_t *pData, uint8_t byLen, uint16_t byVal, int8_t *byPosi)
{
    uint8_t i;

    for (i = 1; i < byLen; i++)
    {
        if (byVal >= pData[i - 1] && byVal < pData[i])
        {
            *byPosi = i - 1;
            return SUCCESS;
        }
    }
    if (byVal < pData[i - 1])
    {
        *byPosi = 0;
        return ERROR;
    }
    else if (byVal >= pData[byLen - 1])
    {
        *byPosi = byLen - 1;
        return ERROR;
    }
    else
    {
        *byPosi = -1;
        return ERROR;
    }
}

/*
 * 根据电池类型和OCV曲线整体特征，判断当前电压是否落在可做OCV校准的区间
 * 磷酸铁锂：中间平台段(3200-3340mV, SOC 20%-90%)电压极其平坦，ADC噪声即可导致
 *           数%的SOC偏差，此区间不启用OCV校准；仅两端陡坡段可做校准
 * 三元锂：  全段斜率较好，均可做OCV校准
 */
static bool l_bAfeInit=false;
  
static uint64_t g_u64SocTickMs=0;
uint8_t SOC_Init(uint8_t *pSocInit)
{
    uint8_t l_byResult;
    int8_t l_byIndex = 0;
    uint16_t wSocRef;
    static eSocStep l_eSocStep = eSocEE; 
    tEERunParam l_tEERunParam;
    if(0xFFFF == g_tBatPackInfo.tCell.wVolMin) { return ERROR; }//无效温度和单体电压则退出
	  if(0xFF == g_tBatPackInfo.tCell.byTempMin) { return ERROR; }
    
    if(GetTick_ms()>g_u64SocTickMs+TIME_WAIT_CONNECT&&!g_tBatPackInfo.tBmDiagVal.tBits.u1AfeCommFlg)
        l_bAfeInit  = true;     
    switch (l_eSocStep)
    {
        case eSocEE:
            l_byResult = byEEMemoryRead(EE_SOV_ADDR, (uint8_t *)&g_tOcvSocTable, EE_SOV_LEN);
            if(Ok != l_byResult)
            {
                if(g_tBatPackCtrl.tEESpec.byBatType == FeLi90Ah)
                {
                    g_tOcvSocTable.byLinearOcv = FELI_N_LINEAR_OCV_SOC;
                    g_tOcvSocTable.byOcvCount = FELI_N_OCV_SOC;
                    g_tOcvSocTable.byOcvPrecise = FELI_N_OCV_SOC_PRECISE;
                    g_tOcvSocTable.byOcvStart =FELI_OCV_SOC_PRECISE_SATRT;
                    for(uint8_t i=0;i<N_OCV_SOC_MAX && i < g_tOcvSocTable.byOcvCount;i++)
                    {
                        g_tOcvSocTable.szOcv[i] = c_szOcvFeli[i];
                        g_tOcvSocTable.szSoc[i] = c_szSocFeli[i];
                    }
                }
                else
                {
                    g_tOcvSocTable.byLinearOcv = NCM_N_LINEAR_OCV_SOC;
                    g_tOcvSocTable.byOcvCount = NCM_N_OCV_SOC;
                    g_tOcvSocTable.byOcvPrecise = NCM_N_OCV_SOC_PRECISE;
                    g_tOcvSocTable.byOcvStart = NCM_OCV_SOC_PRECISE_SATRT;
                    for(uint8_t i=0;i<N_OCV_SOC_MAX && i < g_tOcvSocTable.byOcvCount;i++)
                    {
                        g_tOcvSocTable.szOcv[i] = c_szOcvNcm[i];
                        g_tOcvSocTable.szSoc[i] = c_szSocNcm[i];
                    }
                }
            }
            g_tBatPackInfo.wSocRef =  g_tBatPackCtrl.tEERun.wSocRef;
            g_tBatPackInfo.wSocShow =  g_tBatPackCtrl.tEERun.wSocShow;
            l_eSocStep = eSocTable;
            break;

        case eSocTable:
            if (l_bAfeInit)
            {
                if (!(g_tBatPackInfo.tBmDiagVal.tBits.u1AfeCommFlg))
                {
                    if (g_bCellCountDone && g_fSampleCur > -0.4f && g_fSampleCur < 0.4f
                        /*&& IsOcvCalibReliable(g_tBatPackInfo.tCell.wVolMin, g_tBatPackCtrl.tEESpec.byBatType)*/)
                    {
                        /* 电压在OCV曲线陡坡段，查表可靠，执行OCV校准 */
											l_byResult = LookUpTable16Bit(g_tOcvSocTable.szOcv, g_tOcvSocTable.byOcvCount<N_OCV_SOC_MAX?g_tOcvSocTable.byOcvCount:N_OCV_SOC_MAX, g_tBatPackInfo.tCell.wVolMin, &l_byIndex);
											if (SUCCESS == l_byResult)
											{
													 if (g_tOcvSocTable.szOcv[l_byIndex + 1] != g_tOcvSocTable.szOcv[l_byIndex])
													{
															wSocRef = g_tOcvSocTable.szSoc[l_byIndex] + ((uint32_t)(g_tOcvSocTable.szSoc[l_byIndex + 1] - g_tOcvSocTable.szSoc[l_byIndex]) * (uint32_t)(g_tBatPackInfo.tCell.wVolMin - g_tOcvSocTable.szOcv[l_byIndex]) \
																	/ (uint32_t)(g_tOcvSocTable.szOcv[l_byIndex + 1] - g_tOcvSocTable.szOcv[l_byIndex])); 
													}
													else
													{
															 wSocRef = g_tOcvSocTable.szOcv[l_byIndex];
													} 
													if(g_tBatPackCtrl.tEESpec.byBatType == FeLi90Ah)
													{
														if(wSocRef<=2000&&wSocRef>0)
														{															
															g_tBatPackInfo.wSocRef = wSocRef;
															if(wSocRef < g_tBatPackInfo.wSocShow &&(g_tBatPackInfo.wSocShow-wSocRef) > 1000)
																g_tBatPackInfo.wSocShow = wSocRef;
														}
														else if(wSocRef<=9000&&wSocRef>2000)
														{
															if(ABS((int16_t)wSocRef - (int16_t)g_tBatPackInfo.wSocRef) >= SOC_REF_CORRLIM)
																g_tBatPackInfo.wSocRef = wSocRef;
															if(ABS((int16_t)wSocRef - (int16_t)g_tBatPackInfo.wSocShow) >= SOC_SHOW_CORRLIM)
																g_tBatPackInfo.wSocShow = wSocRef;
														}
														else if(wSocRef>9000&&wSocRef<10000)
														{															
															g_tBatPackInfo.wSocRef = wSocRef;
															if(wSocRef>g_tBatPackInfo.wSocShow&& (wSocRef-g_tBatPackInfo.wSocShow)>= 500)
																g_tBatPackInfo.wSocShow = wSocRef;
														}
													}
													else
													{
														if(wSocRef<=2000&&wSocRef>0)
														{
															g_tBatPackInfo.wSocRef = wSocRef;
															if(wSocRef < g_tBatPackInfo.wSocShow &&(g_tBatPackInfo.wSocShow-wSocRef) > 500)
																g_tBatPackInfo.wSocShow = wSocRef;
														}
														else if(wSocRef<=8000&&wSocRef>2000)
														{
															if(ABS((int16_t)wSocRef - (int16_t)g_tBatPackInfo.wSocRef) >= SOC_REF_CORRLIM)
																g_tBatPackInfo.wSocRef = wSocRef;
															if(ABS((int16_t)wSocRef - (int16_t)g_tBatPackInfo.wSocShow) >= SOC_SHOW_CORRLIM)
																g_tBatPackInfo.wSocShow = wSocRef;
														}
														else if(wSocRef>8000&&wSocRef<10000)
														{
																g_tBatPackInfo.wSocRef = wSocRef;
															if(wSocRef>g_tBatPackInfo.wSocShow&& (wSocRef-g_tBatPackInfo.wSocShow)>= 500)
																g_tBatPackInfo.wSocShow = wSocRef;
														}
													}																	
													*pSocInit = 1;		
											}		
											else
											{
                       // 无法初始化,
													*pSocInit = 2;
													if(l_byIndex>=0 && l_byIndex <g_tOcvSocTable.byOcvCount && l_byIndex <N_OCV_SOC_MAX)
													{
														g_tBatPackInfo.wSocRef =  g_tOcvSocTable.szSoc[l_byIndex];
														g_tBatPackInfo.wSocShow = g_tOcvSocTable.szSoc[l_byIndex];
													}                        
											}											
		                }  /* end: no current + cell count done */
                    else if (g_bCellCountDone)
                    {
                        *pSocInit = 3;
                    }
                }
                else
                {
                    *pSocInit = 4;
                    g_tBatPackInfo.wSocRef = 0;
                    g_tBatPackInfo.wSocShow = 0;
                }
            }
            break;

        default:

            break;

    }

    // 初始化成功, 重新写入到EEPROM
    if (1 == (*pSocInit))
    {
        g_tBatPackCtrl.tEERun.wSocRef = g_tBatPackInfo.wSocRef;
        g_tBatPackCtrl.tEERun.wSocShow = g_tBatPackInfo.wSocShow;
        // l_byResult = byEEMemoryWrite(EE_SOC_ADDR, (uint8_t *)&l_tEESoc, EE_SOC_LEN);
        g_tBatPackCtrl.bySocStep = l_eSocStep;
    }
    // 无论失败成功， 初始化参数表
    if ((*pSocInit))
    {
        // SOC参数初始化
        SetFactor(BatCapacityCacl());
        g_fRefSoc = g_tBatPackInfo.wSocRef;
        g_fShowSoc = g_tBatPackInfo.wSocShow;

        //DS3231_ReadTime(&l_tEECurTime.tCurTime);
        //l_byResult = byEEMemoryWrite(EE_CUR_TIME_ADDR, (uint8_t *)&l_tEECurTime, EE_TIME_LEN);
    }

    return l_byResult;
}

#define PRECISION_FACTOR 1000L

void SohAccAhCacl(void)
{
    static uint32_t s_fChgAhrs = 0;
    static uint32_t s_fDisChgAhrs = 0;
    static  uint8_t bInitChg =0,bInitDsg=0;
    if (g_tBatPackInfo.wCur < -2)
    {
        // 充电
        s_fChgAhrs += (0.05f * 10/3600) * g_tBatPackInfo.wAbsCur*PRECISION_FACTOR;
        if (s_fChgAhrs >= PRECISION_FACTOR)
        {
            s_fChgAhrs -= PRECISION_FACTOR;
            g_tBatPackCtrl.wSingleChgAhrs++;
            g_tBatPackCtrl.wSingleDisChgAhrs--;
            g_tBatPackCtrl.tEERun.dwChgAhrs++;
            // 充满 放电清0
            if (s_bFullLatch)
            {
                g_tBatPackCtrl.wSingleDisChgAhrs = 0;
                bInitChg = 1;
                if(bInitDsg)
                {
                    SetFactor(g_tBatPackCtrl.wSingleChgAhrs);//放空充满一次的容量
                    bInitDsg =0;
                }
            }
        }
    }
    else if (g_tBatPackInfo.wCur > 2)
    {
        // 放电
        s_fDisChgAhrs += (0.05f * 10/3600) * g_tBatPackInfo.wAbsCur*PRECISION_FACTOR;
        if (s_fDisChgAhrs >= PRECISION_FACTOR)
        {
            s_fDisChgAhrs -= PRECISION_FACTOR;
            g_tBatPackCtrl.wSingleDisChgAhrs++;
            g_tBatPackCtrl.wSingleChgAhrs--;
            g_tBatPackCtrl.tEERun.dwDisChgAhrs++;
            // 放空 充电清0
            if (s_bEmptyLatch)
            {
                g_tBatPackCtrl.wSingleChgAhrs = 0;
                bInitDsg = 1;
                if(bInitChg)
                {
                    SetFactor( g_tBatPackCtrl.wSingleDisChgAhrs);//充满放空一次的容量
                    bInitChg =0;
                }
            }
        }
    }

    g_tBatPackCtrl.tEERun.dvBatCycleCnt =  (g_tBatPackCtrl.tEERun.dwChgAhrs+g_tBatPackCtrl.tEERun.dwDisChgAhrs)/g_tBatPackCtrl.tEESpec.wBatCapacity/20;
}

uint16_t BatCapacityCacl(void)
{
    uint16_t wBaseCap = g_tBatPackCtrl.tEESpec.byBatParallel*g_tBatPackCtrl.tEESpec.wBatCapacity;
    uint16_t wChgFullTime = g_tBatPackCtrl.tEERun.dwChgAhrs/wBaseCap;
    return  wBaseCap*(1-N_PACK_LOOP_DEC *wChgFullTime /g_tBatPackCtrl.tEESpec.wHoldingTime);

}

void SOC_WakeupCalibrate(void)
{
    /* Reset SOC init state to force OCV-SOC table lookup on next estimate.
     * Used after wake-up from sleep since coulomb counter was stopped. */
    g_tBatPackCtrl.bySocInit = 0;
    g_tBatPackCtrl.bySocStep = 0;
    g_fRefSoc = g_tBatPackInfo.wSocRef;
    g_fShowSoc = g_tBatPackInfo.wSocShow;
    s_bFullLatch  = false;
    s_bEmptyLatch = false;
}

void SocEstimation(void)
{
    static uint16_t s_wLastSoc = 0;
    static uint8_t hcnt, lcnt= 0;
    static bool soc=false;
		
    if (!g_tBatPackCtrl.bySocInit)
    {
				if(!soc)
				{
					l_bAfeInit=false;
					g_u64SocTickMs=GetTick_ms();
					soc=true;
					s_bFullLatch  = false;  /* clear endpoint latches on re-init */
					s_bEmptyLatch = false;
				}
        SOC_Init(&g_tBatPackCtrl.bySocInit);         // 初始化SOC

        return;
    }
		if(soc)
			soc=false;

        SohAccAhCacl();
        // 在SocShow 和 SocRef接近时，两个因子一致
        if (abs(g_tBatPackInfo.wSocRef - g_tBatPackInfo.wSocShow) < 300)
        {
            g_tSocParam.fShowFactor = g_tSocParam.fRefFactor;
        }
        else
        {
            // 放电
            if (g_tBatPackInfo.wCur > 0)
            {
                if (g_tBatPackInfo.wSocShow > g_tBatPackInfo.wSocRef)
                {
                    // 放电时，需要SocShow快速下降接近SocRef
                    g_tSocParam.fShowFactor = g_tSocParam.fRefFactor * 1.5f;
                }
                else
                {
                    // 放电时，需要SocShow缓慢下降接近SocRef
                    g_tSocParam.fShowFactor = g_tSocParam.fRefFactor * 0.5f;
                }
            }
            else
            {
                if (g_tBatPackInfo.wSocShow > g_tBatPackInfo.wSocRef)
                {
                    g_tSocParam.fShowFactor = g_tSocParam.fRefFactor * 0.5f;
                }
                else
                {
                    // 充电时，需要SocShow快速上升接近SocRef
                    g_tSocParam.fShowFactor = g_tSocParam.fRefFactor * 1.5f;
                }
            }
        }


        // 电流积分计算SOC        50ms计算一次
        g_fRefSoc += g_fSampleCur * g_tSocParam.fRefFactor * -1000;   // 充电为负 SOC增加
        g_fShowSoc += g_fSampleCur * g_tSocParam.fShowFactor * -1000; // 放电为正 SOC减少


       

        if (s_bFullLatch && g_tBatPackInfo.eCtrlMode == eBmQdsg
            && g_tBatPackInfo.tCell.wVolMax < g_tFaultDataParam.tCellVolt[2].wLowerVal)
        {
            s_bFullLatch = false;   // 放电且最高单体回落，满充解除，恢复向下计量
        }
        if (s_bEmptyLatch && g_tBatPackInfo.eCtrlMode == eBmQchg
            && g_tBatPackInfo.tCell.wVolMin > g_tFaultDataParam.tCellVolt[5].wLowerVal)
        {
            s_bEmptyLatch = false;  // 充电且最低单体回升，放空解除，恢复向上计量
        }
        if (g_tBatPackInfo.eCtrlMode == eBmQchg)
        {
            if (g_tBatPackInfo.tCell.wVolMax > g_tBatPackCtrl.tEESpec.wCellVolMax|| g_tBatPackInfo.tCell.wVolMax >  g_tFaultDataParam.tCellVolt[2].wUpperVal)
            {
                if (hcnt++ > 20)
                {
                    hcnt = 0;
                    s_bFullLatch  = true;
                    s_bEmptyLatch = false;
                }
            }
            else
            {
                hcnt = 0;
                if (!s_bFullLatch && g_fShowSoc >= 9900)
            {
                g_fShowSoc = 9900;
                g_fRefSoc = 9900;
                }
            }
        }
        else if (g_tBatPackInfo.eCtrlMode == eBmQdsg)
        {
            if (g_tBatPackInfo.tCell.wVolMin < g_tBatPackCtrl.tEESpec.wCellVolMin || g_tBatPackInfo.tCell.wVolMin <g_tFaultDataParam.tCellVolt[5].wUpperVal )
            {
                if (lcnt++ > 20)
                {
                    lcnt = 0;
                    s_bEmptyLatch = true;
                    s_bFullLatch  = false;
                }
            }
            else
            {
                lcnt = 0;
                if (!s_bEmptyLatch && g_fShowSoc <= 100)
            {
                g_fShowSoc = 100;
                g_fRefSoc = 100;
                }
            }
        }
        else
        {
					if (g_fRefSoc > 10000)
					{
							g_fRefSoc = 10000;
					}
					else if (g_fRefSoc < 0)
					{
							g_fRefSoc = 0;
					}
					if (g_fShowSoc > 10000)
					{
							g_fShowSoc = 10000;
					}
					else if (g_fShowSoc < 0)
					{
							g_fShowSoc = 0;
					}
				}
        if (s_bFullLatch)
        {
            g_fShowSoc = 10000;     // 充满后不允许浮充：恒定100%
            g_fRefSoc  = 10000;
        }
        else if (s_bEmptyLatch)
        {
            g_fShowSoc = 0;         // 放空后不允许回退：恒定0%
            g_fRefSoc  = 0;
        }
        else
        {
            if (g_fRefSoc  > 10000) g_fRefSoc  = 10000; else if (g_fRefSoc  < 0) g_fRefSoc  = 0;
            if (g_fShowSoc > 10000) g_fShowSoc = 10000; else if (g_fShowSoc < 0) g_fShowSoc = 0;
        }
        g_tBatPackInfo.wSocRef = g_fRefSoc;
        g_tBatPackInfo.wSocShow = g_fShowSoc;
				g_tBatPackCtrl.tEERun.wSocRef = g_tBatPackInfo.wSocRef;
				g_tBatPackCtrl.tEERun.wSocShow = g_tBatPackInfo.wSocShow;
        // SOC每变化3%保存一次
        if (abs(s_wLastSoc - g_tBatPackInfo.wSocRef) > 300)
        {
            s_wLastSoc = g_tBatPackInfo.wSocRef;
            // byEEMemoryWrite(EE_RUN_ADDR, (uint8_t *)& g_tBatPackCtrl.tEERun, EE_RUN_LEN);
        }
    }

/**********************************************************************************************
 * EOF
**********************************************************************************************/
