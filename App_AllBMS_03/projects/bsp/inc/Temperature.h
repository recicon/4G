
#ifndef __TEMPERATURE_H__
#define __TEMPERATURE_H__

#include "DVC1124.h"
#include "bms_config.h"
#include"n32wb43x_adc.h"


#define N_PT100_TABLE_MAX          (161)

#define REGVOLFLT_CNT					(8)     //Reg 电压滤波计数
#define TSFLT_CNT						(8)     //温度传感器滤波计数
#define TEMP_UPRES			       (10000)//(10000) 		//温度电阻上拉电阻
#define TEMP_REGAI                  TEMP_REGAI_VOLT

typedef struct
{
		 uint16_t	Buf[20];	//数据缓存
	const uint8_t	u8DatLen;	//计数宽度(不超过数据缓存深度)
		  uint8_t 	u8Pt;		//数据位置
}tag_maf;

typedef struct
{
    uint16_t wARegAi;                 //AFE电源电源
    uint16_t wPackAi;                 //Pack电压
    uint16_t wETsAi;                  // 温度1
    uint16_t wMTsAi;                  // 温度2
    uint16_t wIRefAin;                // 低功耗电流
    uint16_t wLPDAin;                 // 电流供电
    uint16_t wTRegAi;                 // 温度电源
}tDiagnoseSample;

void GPM_Setup (void);
void Temperature_Handle(void);

void ADC_Initial(void);

extern int uiTempVmin,uiTempVmax;

#endif
