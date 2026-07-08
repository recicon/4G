#include"App.h"
#include "MemoryModule.h"
#include "TimeModule.h"
#include <string.h>
#include"Protection.h"
#include "n32wb43x_flash.h"
#include "app_proto_cmd.h"
#include "ModbusProEx.h"
#include "app_proto_cmn.h"
#include "app_uart.h"

#define YEAR ((((__DATE__ [7] - '0') * 10 + (__DATE__ [8] - '0')) * 10 + (__DATE__ [9] - '0')) * 10 + (__DATE__ [10] - '0'))
#define MONTH ( __DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 1 : 6) \
: __DATE__ [2] == 'b' ? 2 \
: __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 3 : 4) \
: __DATE__ [2] == 'y' ? 5 \
: __DATE__ [2] == 'n' ? 6 \
: __DATE__ [2] == 'l' ? 7 \
: __DATE__ [2] == 'g' ? 8 \
: __DATE__ [2] == 'p' ? 9 \
: __DATE__ [2] == 't' ? 10 \
: __DATE__ [2] == 'v' ? 11 : 12)
#define DAY ((__DATE__ [4] == ' ' ? 0 : ((__DATE__ [4] - '0') * 10 )) + (__DATE__ [5] - '0'))
#define HOUR ((__TIME__ [0] == ' ' ? 0 : ((__TIME__ [0] - '0') * 10 )) + (__TIME__ [1] - '0'))
#define MINUTE ((__TIME__ [3] == ' ' ? 0 : ((__TIME__ [3] - '0') * 10 )) + (__TIME__ [4] - '0'))
#define SECOND ((__TIME__ [6] == ' ' ? 0 : ((__TIME__ [6] - '0') * 10 )) + (__TIME__ [7] - '0'))

// 单体温度过高温度界限
#define TEMP_CELL_HIGH_UPPER_I        (56 + 40) 
#define TEMP_CELL_HIGH_LOWER_I        (50 + 40)
#define TEMP_CELL_HIGH_UPPER_II       (61 + 40)
#define TEMP_CELL_HIGH_LOWER_II       (55 + 40)
#define TEMP_CELL_DISCHG_HIGH_UPPER_III      (75 + 40)
#define TEMP_CELL_DISCHG_HIGH_LOWER_III      (65 + 40)
#define TEMP_CELL_CHG_HIGH_UPPER_III      (65 + 40)
#define TEMP_CELL_CHG_HIGH_LOWER_III      (55 + 40)

#define TEMP_AFE_HIGH_UPPER_III       (85 + 40)
#define TEMP_AFE_HIGH_LOWER_III       (80 + 40)
#define TEMP_MOS_HIGH_UPPER_III       (105 + 40)
#define TEMP_MOS_HIGH_LOWER_III       (80 + 40)


// 单体温度过低温度界限
#define TEMP_CELL_LOW_UPPER_I  			(-5 + 40)
#define TEMP_CELL_LOW_LOWER_I  			(0  + 40)
#define TEMP_CELL_LOW_UPPER_II 			(-15 + 40)
#define TEMP_CELL_LOW_LOWER_II 			(-5 + 40)
#define TEMP_CELL_DISCHG_LOW_UPPER_III	(-30 + 40)
#define TEMP_CELL_DISCHG_LOW_LOWER_III	(-20 + 40)
#define TEMP_CELL_CHG_LOW_UPPER_III     (-10   + 40)
#define TEMP_CELL_CHG_LOW_LOWER_III		(-5   + 40)

#define TEMP_CELL_HOTCHG_LOW_UPPER_III     (-5   + 40)
#define TEMP_CELL_HOTCHG_LOW_LOWER_III		(0   + 40)

// 单体温差过大温度界限
#define TEMP_CELL_DIFF_UPPER_I        15
#define TEMP_CELL_DIFF_LOWER_I        13
#define TEMP_CELL_DIFF_UPPER_II       25
#define TEMP_CELL_DIFF_LOWER_II       18     
#define TEMP_CELL_DIFF_UPPER_III      30
#define TEMP_CELL_DIFF_LOWER_III      23


// 单体电压过高电压界限
#define FELI_VOLT_CELL_HIGH_UPPER_I        3460
#define FELI_VOLT_CELL_HIGH_LOWER_I        3400
#define FELI_VOLT_CELL_HIGH_UPPER_II       3510
#define FELI_VOLT_CELL_HIGH_LOWER_II       3450
#define FELI_VOLT_CELL_HIGH_UPPER_III      3750
#define FELI_VOLT_CELL_HIGH_LOWER_III      3600

// 单体电压过低电压界限
#define FELI_VOLT_CELL_LOW_UPPER_I  		  2900
#define FELI_VOLT_CELL_LOW_LOWER_I  		  3050
#define FELI_VOLT_CELL_LOW_UPPER_II 		  2850
#define FELI_VOLT_CELL_LOW_LOWER_II 		  2950
#define FELI_VOLT_CELL_LOW_UPPER_III		  2300
#define FELI_VOLT_CELL_LOW_LOWER_III		  2400

// 单体温差过大电压界限
#define FELI_VOLT_CELL_DIFF_UPPER_I        300
#define FELI_VOLT_CELL_DIFF_LOWER_I        250
#define FELI_VOLT_CELL_DIFF_UPPER_II       400
#define FELI_VOLT_CELL_DIFF_LOWER_II       350
#define FELI_VOLT_CELL_DIFF_UPPER_III      500
#define FELI_VOLT_CELL_DIFF_LOWER_III      450


// 电池总电压过高电压界限
#define FELI_VOLT_BATT_HIGH_UPPER_I        (34.6)
#define FELI_VOLT_BATT_HIGH_LOWER_I        (34)
#define FELI_VOLT_BATT_HIGH_UPPER_II       (35)
#define FELI_VOLT_BATT_HIGH_LOWER_II       (34.5)
#define FELI_VOLT_BATT_HIGH_UPPER_III      (37.5)
#define FELI_VOLT_BATT_HIGH_LOWER_III      (36)

// 电池总电压过低电压界限
#define FELI_VOLT_BATT_LOW_UPPER_I  		(29)
#define FELI_VOLT_BATT_LOW_LOWER_I  		(30.5)
#define FELI_VOLT_BATT_LOW_UPPER_II 		(28.5)
#define FELI_VOLT_BATT_LOW_LOWER_II 		(29.5)
#define FELI_VOLT_BATT_LOW_UPPER_III		(23)
#define FELI_VOLT_BATT_LOW_LOWER_III		(24)

// 单体电压过高电压界限
#define NCM_VOLT_CELL_HIGH_UPPER_I        3900
#define NCM_VOLT_CELL_HIGH_LOWER_I        3800
#define NCM_VOLT_CELL_HIGH_UPPER_II       4060
#define NCM_VOLT_CELL_HIGH_LOWER_II       3950
#define NCM_VOLT_CELL_HIGH_UPPER_III      4200
#define NCM_VOLT_CELL_HIGH_LOWER_III      4150

// 单体电压过低电压界限
#define NCM_VOLT_CELL_LOW_UPPER_I  		  3450
#define NCM_VOLT_CELL_LOW_LOWER_I  		  3550
#define NCM_VOLT_CELL_LOW_UPPER_II 		  3150
#define NCM_VOLT_CELL_LOW_LOWER_II 		  3250
#define NCM_VOLT_CELL_LOW_UPPER_III		  2800
#define NCM_VOLT_CELL_LOW_LOWER_III		  2850

// 单体温差过大电压界限
#define NCM_VOLT_CELL_DIFF_UPPER_I        300
#define NCM_VOLT_CELL_DIFF_LOWER_I        250
#define NCM_VOLT_CELL_DIFF_UPPER_II       400
#define NCM_VOLT_CELL_DIFF_LOWER_II       350
#define NCM_VOLT_CELL_DIFF_UPPER_III      500
#define NCM_VOLT_CELL_DIFF_LOWER_III      450


// 电池总电压过高电压界限
#define NCM_VOLT_BATT_HIGH_UPPER_I        (39 )
#define NCM_VOLT_BATT_HIGH_LOWER_I        (39 )
#define NCM_VOLT_BATT_HIGH_UPPER_II       (40 )
#define NCM_VOLT_BATT_HIGH_LOWER_II       (40 )
#define NCM_VOLT_BATT_HIGH_UPPER_III      (41.5 )
#define NCM_VOLT_BATT_HIGH_LOWER_III      (41.5 )

// 电池总电压过低电压界限
#define NCM_VOLT_BATT_LOW_UPPER_I  		(35.5 )
#define NCM_VOLT_BATT_LOW_LOWER_I  		(35.5 )
#define NCM_VOLT_BATT_LOW_UPPER_II 		(30.5 )
#define NCM_VOLT_BATT_LOW_LOWER_II 		(30.5 )
#define NCM_VOLT_BATT_LOW_UPPER_III		(28.5 )
#define NCM_VOLT_BATT_LOW_LOWER_III		(28.5 )

// 电池充电电流过大
#define CURR_BATT_CHG_UPPER_I        ((float)0.55)
#define CURR_BATT_CHG_LOWER_I        ((float)0.5)
#define CURR_BATT_CHG_UPPER_II       ((float)0.6)
#define CURR_BATT_CHG_LOWER_II       ((float)0.5)
#define CURR_BATT_CHG_UPPER_III      ((float)1.87)
#define CURR_BATT_CHG_LOWER_III      ((float)0.5)

// 电池放电电流过大
#define CURR_BATT_DISCHG_UPPER_I  		((float)1.1)
#define CURR_BATT_DISCHG_LOWER_I  		((float)1.05)
#define CURR_BATT_DISCHG_UPPER_II 		((float)1.2)
#define CURR_BATT_DISCHG_LOWER_II 		((float)1.05)
#define CURR_BATT_DISCHG_UPPER_III		((float)1.87)
#define CURR_BATT_DISCHG_LOWER_III		((float)1.05)


#define TEMP_LOWI_BOUND_ALARM              (5 + 40)      // 5度以下认为是低温 
#define TEMP_LOWII_BOUND_ALARM             (0 + 40)      
#define TEMP_LOWIII_BOUND_ALARM            (-5 + 40)      


// 电池SOC过低
#define SOC_BATT_LOW_UPPER_I        10
#define SOC_BATT_LOW_LOWER_I        15
#define SOC_BATT_LOW_UPPER_II       6
#define SOC_BATT_LOW_LOWER_II       10
#define SOC_BATT_LOW_UPPER_III      3
#define SOC_BATT_LOW_LOWER_III      8



typedef struct
{
    uint8_t  byUpperCnt;
    uint8_t  byLowerCnt;
}tFaultLevelCntParam;
typedef struct
{
    tFaultLevelCntParam tCellTemp[N_CELL_TEMP_ALARM];
    tFaultLevelCntParam tCellVolt[N_CELL_VOLT_ALARM];
    tFaultLevelCntParam tBattVolt[N_BATT_VOLT_ALARM];
    tFaultLevelCntParam tBattCurr[N_BATT_CURR_ALARM];
    //tFaultLevelCntParam tBattCurrLowTemp[N_BATT_CURR_LOW_TEMP_ALARM];
    tFaultLevelCntParam tBattSoc[N_BATT_SOC_ALARM];
    tFaultLevelCntParam tBattInsulate[N_BATT_INSULATE_ALARM];
}tFaultCntParam;



tHcuPowerCmd g_tHcuPowerCmd;
tFaultCntParam g_tFaultCntParam;
tFaultDataParam g_tFaultDataParam;
tDignoseParamIn g_tProDignoseCtrl;
tBatPackInfo g_tBatPackInfo;
tBatPackCtrl g_tBatPackCtrl;
tDiagnosticMsg  g_tDiagnosticMsg;
tFaultBitInfo g_tFaultBitInfo;
tBleName g_tBleName;  

uint8_t g_byCurDriftCalibStatus = 0;
float g_wBatCurDrift = 0.0f;
static bool g_bCurDriftCalibActive = false;
static uint64_t g_u64CurDriftCalibStartTime = 0;
static float g_fCurDriftCalibSum = 0.0f;
static uint32_t g_u32CurDriftCalibCount = 0;
#define CUR_DRIFT_CALIB_DURATION_MS  30000

void App_Init(void)   
{   
    uint8_t l_byResult;
    tEERunParam l_tEERunParam;        
    tEESpecParam l_tEESpecParam;
    tBleName l_tBleName;
    const uint8_t c_CecuSoftVer[] = VERSION_PRODUCT;
    
    memset((uint8_t *)&g_tBatPackInfo, 0x00, sizeof(tBatPackInfo) / sizeof(uint8_t));

    g_tBatPackInfo.tCell.wVolMax = 0x0;
    g_tBatPackInfo.tCell.wVolMin = 0xFFFF;
    g_tBatPackInfo.tCell.byTempMin = 0xFF;
    g_tBatPackInfo.tCell.byTempMax = 0x00;
    memset((uint8_t *)&g_tBatPackInfo.tBmFltVal.wByte, 0, sizeof(g_tBatPackInfo.tBmFltVal.wByte) / sizeof(uint8_t));
    memset((uint8_t *)&g_tBatPackInfo.tBmDiagVal.wByte, 0, sizeof(g_tBatPackInfo.tBmDiagVal.wByte) / sizeof(uint8_t));
    
   
    
    memset((uint8_t *)&g_tBatPackCtrl, 0x00, sizeof(tBatPackCtrl) / sizeof(uint8_t));
    memset((uint8_t *)&g_tFaultBitInfo, 0x00, sizeof(tFaultBitInfo) / sizeof(uint8_t));    


    l_byResult = byEEMemoryRead(EE_RUN_ADDR, (uint8_t *)&l_tEERunParam, EE_RUN_LEN);
    if (Ok == l_byResult)
    {
        g_tBatPackCtrl.tEERun = l_tEERunParam;
    }
    else
    {
        g_tBatPackCtrl.tEERun.tRunTime.year =   YEAR-2000;
        g_tBatPackCtrl.tEERun.tRunTime.month =  MONTH;
        g_tBatPackCtrl.tEERun.tRunTime.day =   DAY;
        g_tBatPackCtrl.tEERun.tRunTime.hour =   HOUR;
        g_tBatPackCtrl.tEERun.tRunTime.minute =  MINUTE;
        g_tBatPackCtrl.tEERun.tRunTime.second =  SECOND;
        g_tBatPackCtrl.tEERun.wSocRef = 5000;
        g_tBatPackCtrl.tEERun.wSocShow = 5000;        
    }
    l_byResult = byEEMemoryRead(EE_SPECIAL_ADDR, (uint8_t *)&l_tEESpecParam, EE_SPECIAL_LEN);            
    if (Ok == l_byResult)
    {
        g_tBatPackCtrl.tEESpec = l_tEESpecParam;
        g_tBatPackCtrl.tEESpec.byLecuNum = N_LECU_NUM;   
        if (g_tBatPackCtrl.tEESpec.wDiagFltSet & (1<<eDiagPowerCmd))
        {
            g_tBatPackCtrl.tEESpec.byPackCell = N_PACK_CELL;
        }
        
    }
    else
    { 
        g_tBatPackCtrl.tEESpec.wHoldingTime = N_PACK_LOOP_TIME;
        g_tBatPackCtrl.tEESpec.byBatParallel = N_BATT_PARALLEL;
        g_tBatPackCtrl.tEESpec.byBatType   = BAT_TYPE;
        g_tBatPackCtrl.tEESpec.byBmsId = 0;
        g_tBatPackCtrl.tEESpec.byLecuNum = N_LECU_NUM;
        g_tBatPackCtrl.tEESpec.byPackCell = N_PACK_CELL;
        g_tBatPackCtrl.tEESpec.byPackTemp = N_PACK_TEMP;
        g_tBatPackCtrl.tEESpec.byProtocolVer = 0;
        g_tBatPackCtrl.tEESpec.byTempType = RT10K_3950;
        g_tBatPackCtrl.tEESpec.wBatCapacity = N_BATT_CAPACITY;
        g_tBatPackCtrl.tEESpec.wBatCurOffset = 0;
        g_tBatPackCtrl.tEESpec.wBatVoltOffset = 4096;
        g_tBatPackCtrl.tEESpec.wCapVoltOffset = 4096;
        g_tBatPackCtrl.tEESpec.cbBlanceType = eLecuEquPassiVal;
        if(FeLi90Ah == g_tBatPackCtrl.tEESpec.byBatType )
        {
            g_tBatPackCtrl.tEESpec.wCellVolMax = FELI_SLEFCHEK_CELL_VOLTAGE_UPPER;
            g_tBatPackCtrl.tEESpec.wCellVolMin = FELI_SLEFCHEK_CELL_VOLTAGE_LOWER;            
            g_tBatPackCtrl.tEESpec.wBalanceVolt = FELI_BMS_BALANCE_VOLT;
            g_tBatPackCtrl.tEESpec.wBalanceDiff = FELI_BMS_BALANCE_DIFF;
            g_tBatPackCtrl.tEESpec.byCellTempMax = FELI_SLEFCHEK_TEMPERATURE_UPPER;
            g_tBatPackCtrl.tEESpec.byCellTempMin = FELI_SLEFCHEK_TEMPERATURE_LOWER;
            g_tBatPackCtrl.tEESpec.wCellVolLowChg = FELI_VOLT_CELL_LOW_CHG;
        }    
        else
        {
            g_tBatPackCtrl.tEESpec.wCellVolMax = NCM_SLEFCHEK_CELL_VOLTAGE_UPPER;
            g_tBatPackCtrl.tEESpec.wCellVolMin = NCM_SLEFCHEK_CELL_VOLTAGE_LOWER;            
            g_tBatPackCtrl.tEESpec.wBalanceVolt = NCM_BMS_BALANCE_VOLT;
            g_tBatPackCtrl.tEESpec.wBalanceDiff = NCM_BMS_BALANCE_DIFF; 
            g_tBatPackCtrl.tEESpec.byCellTempMax = NCM_SLEFCHEK_TEMPERATURE_UPPER;
            g_tBatPackCtrl.tEESpec.byCellTempMin = NCM_SLEFCHEK_TEMPERATURE_LOWER;
            g_tBatPackCtrl.tEESpec.wCellVolLowChg = NCM_VOLT_CELL_LOW_CHG;
        }    
        g_tBatPackCtrl.tEESpec.wPackChgMax = PACK_CHG_MAX_CUR;
        g_tBatPackCtrl.tEESpec.wPackDischgMax = PACK_DSG_MAX_CUR;
        g_tBatPackCtrl.tEESpec.wDiagFltSet = 0xFFFF; 
        
        if(!EN_BLUCOMM)
            g_tBatPackCtrl.tEESpec.wDiagFltSet &= ~(1<< eDiagBluComm); 
        if(!EN_4GCOMM)
            g_tBatPackCtrl.tEESpec.wDiagFltSet &= ~(1<< eDiag4GComm); 
 
        
    }
     l_byResult = byEEMemoryRead(EE_BLENAME_LEN, (uint8_t *)&l_tBleName, EE_BLENAME_LEN);            
    if (Ok == l_byResult)
    {
        g_tBleName =l_tBleName;
    
    }
    else
    {
        //memset(&g_tBleName,0,sizeof(g_tBleName));
    }
    memcpy(g_tBatPackCtrl.cPrdType,c_CecuSoftVer,ARRAY_SZ(g_tBatPackCtrl.cPrdType)<ARRAY_SZ(c_CecuSoftVer)?ARRAY_SZ(g_tBatPackCtrl.cPrdType):ARRAY_SZ(c_CecuSoftVer));
    g_tBatPackCtrl.dwPrdVer = VERSION_PROCESS;    
    
  
    
    //默认地址
    if(g_tBatPackCtrl.tEESpec.byBmsId == 0)
    {
      g_tBatPackCtrl.tEESpec.byBmsId = 1;
    }    
}

uint8_t FaultLevelInit(void)
{
    uint8_t l_byResult;
    
    // 先读内存，内存参数不正确，使用默认值—功能保留
    l_byResult = byEEMemoryRead(EE_FAULT_LEVEL_PARAM_START_ADDR, (uint8_t *)&g_tFaultDataParam, sizeof(g_tFaultDataParam) / sizeof(uint8_t));
    
    if (Ok != l_byResult)
    {
        // 单体温度界限参数初始化
        g_tFaultDataParam.tCellTemp[0].byUpperVal = TEMP_CELL_HIGH_UPPER_I;
        g_tFaultDataParam.tCellTemp[0].byLowerVal = TEMP_CELL_HIGH_LOWER_I;
        g_tFaultDataParam.tCellTemp[1].byUpperVal = TEMP_CELL_HIGH_UPPER_II;
        g_tFaultDataParam.tCellTemp[1].byLowerVal = TEMP_CELL_HIGH_LOWER_II;
        g_tFaultDataParam.tCellTemp[2].byUpperVal = TEMP_CELL_DISCHG_HIGH_UPPER_III;
        g_tFaultDataParam.tCellTemp[2].byLowerVal = TEMP_CELL_DISCHG_HIGH_LOWER_III;
        g_tFaultDataParam.tCellTemp[3].byUpperVal = TEMP_CELL_LOW_UPPER_I;
        g_tFaultDataParam.tCellTemp[3].byLowerVal = TEMP_CELL_LOW_LOWER_I;
        g_tFaultDataParam.tCellTemp[4].byUpperVal = TEMP_CELL_LOW_UPPER_II;
        g_tFaultDataParam.tCellTemp[4].byLowerVal = TEMP_CELL_LOW_LOWER_II;
        g_tFaultDataParam.tCellTemp[5].byUpperVal = TEMP_CELL_DISCHG_LOW_UPPER_III;
        g_tFaultDataParam.tCellTemp[5].byLowerVal = TEMP_CELL_DISCHG_LOW_LOWER_III;
        g_tFaultDataParam.tCellTemp[6].byUpperVal = TEMP_CELL_DIFF_UPPER_I;
        g_tFaultDataParam.tCellTemp[6].byLowerVal = TEMP_CELL_DIFF_LOWER_I;
        g_tFaultDataParam.tCellTemp[7].byUpperVal = TEMP_CELL_DIFF_UPPER_II;
        g_tFaultDataParam.tCellTemp[7].byLowerVal = TEMP_CELL_DIFF_LOWER_II;
        g_tFaultDataParam.tCellTemp[8].byUpperVal = TEMP_CELL_CHG_HIGH_UPPER_III;
        g_tFaultDataParam.tCellTemp[8].byLowerVal = TEMP_CELL_CHG_HIGH_LOWER_III;
        g_tFaultDataParam.tCellTemp[9].byUpperVal = TEMP_CELL_HOTCHG_LOW_UPPER_III;
        g_tFaultDataParam.tCellTemp[9].byLowerVal = TEMP_CELL_HOTCHG_LOW_LOWER_III;
        
        
        if(g_tBatPackCtrl.tEESpec.byBatType == FeLi90Ah)
        {
            g_tFaultDataParam.tCellVolt[0].wUpperVal = FELI_VOLT_CELL_HIGH_UPPER_I;
            g_tFaultDataParam.tCellVolt[0].wLowerVal = FELI_VOLT_CELL_HIGH_LOWER_I;
            g_tFaultDataParam.tCellVolt[1].wUpperVal = FELI_VOLT_CELL_HIGH_UPPER_II;
            g_tFaultDataParam.tCellVolt[1].wLowerVal = FELI_VOLT_CELL_HIGH_LOWER_II;
            g_tFaultDataParam.tCellVolt[2].wUpperVal = FELI_VOLT_CELL_HIGH_UPPER_III;
            g_tFaultDataParam.tCellVolt[2].wLowerVal = FELI_VOLT_CELL_HIGH_LOWER_III;
            g_tFaultDataParam.tCellVolt[3].wUpperVal = FELI_VOLT_CELL_LOW_UPPER_I;
            g_tFaultDataParam.tCellVolt[3].wLowerVal = FELI_VOLT_CELL_LOW_LOWER_I;
            g_tFaultDataParam.tCellVolt[4].wUpperVal = FELI_VOLT_CELL_LOW_UPPER_II;
            g_tFaultDataParam.tCellVolt[4].wLowerVal = FELI_VOLT_CELL_LOW_LOWER_II;
            g_tFaultDataParam.tCellVolt[5].wUpperVal = FELI_VOLT_CELL_LOW_UPPER_III;
            g_tFaultDataParam.tCellVolt[5].wLowerVal = FELI_VOLT_CELL_LOW_LOWER_III;
            g_tFaultDataParam.tCellVolt[6].wUpperVal = FELI_VOLT_CELL_DIFF_UPPER_I;
            g_tFaultDataParam.tCellVolt[6].wLowerVal = FELI_VOLT_CELL_DIFF_LOWER_I;
            g_tFaultDataParam.tCellVolt[7].wUpperVal = FELI_VOLT_CELL_DIFF_UPPER_II;
            g_tFaultDataParam.tCellVolt[7].wLowerVal = FELI_VOLT_CELL_DIFF_LOWER_II;
            g_tFaultDataParam.tCellVolt[8].wUpperVal = FELI_VOLT_CELL_DIFF_UPPER_III;
            g_tFaultDataParam.tCellVolt[8].wLowerVal = FELI_VOLT_CELL_DIFF_LOWER_III;      
            
        }
        else
        {
            g_tFaultDataParam.tCellVolt[0].wUpperVal = NCM_VOLT_CELL_HIGH_UPPER_I;
            g_tFaultDataParam.tCellVolt[0].wLowerVal = NCM_VOLT_CELL_HIGH_LOWER_I;
            g_tFaultDataParam.tCellVolt[1].wUpperVal = NCM_VOLT_CELL_HIGH_UPPER_II;
            g_tFaultDataParam.tCellVolt[1].wLowerVal = NCM_VOLT_CELL_HIGH_LOWER_II;
            g_tFaultDataParam.tCellVolt[2].wUpperVal = NCM_VOLT_CELL_HIGH_UPPER_III;
            g_tFaultDataParam.tCellVolt[2].wLowerVal = NCM_VOLT_CELL_HIGH_LOWER_III;
            g_tFaultDataParam.tCellVolt[3].wUpperVal = NCM_VOLT_CELL_LOW_UPPER_I;
            g_tFaultDataParam.tCellVolt[3].wLowerVal = NCM_VOLT_CELL_LOW_LOWER_I;
            g_tFaultDataParam.tCellVolt[4].wUpperVal = NCM_VOLT_CELL_LOW_UPPER_II;
            g_tFaultDataParam.tCellVolt[4].wLowerVal = NCM_VOLT_CELL_LOW_LOWER_II;
            g_tFaultDataParam.tCellVolt[5].wUpperVal = NCM_VOLT_CELL_LOW_UPPER_III;
            g_tFaultDataParam.tCellVolt[5].wLowerVal = NCM_VOLT_CELL_LOW_LOWER_III;
            g_tFaultDataParam.tCellVolt[6].wUpperVal = NCM_VOLT_CELL_DIFF_UPPER_I;
            g_tFaultDataParam.tCellVolt[6].wLowerVal = NCM_VOLT_CELL_DIFF_LOWER_I;
            g_tFaultDataParam.tCellVolt[7].wUpperVal = NCM_VOLT_CELL_DIFF_UPPER_II;
            g_tFaultDataParam.tCellVolt[7].wLowerVal = NCM_VOLT_CELL_DIFF_LOWER_II;
            g_tFaultDataParam.tCellVolt[8].wUpperVal = NCM_VOLT_CELL_DIFF_UPPER_III;
            g_tFaultDataParam.tCellVolt[8].wLowerVal = NCM_VOLT_CELL_DIFF_LOWER_III;            
        }        
     
        if(g_tBatPackCtrl.tEESpec.byBatType == FeLi90Ah)
        {
            g_tFaultDataParam.tBattVolt[0].wUpperVal = FELI_VOLT_BATT_HIGH_UPPER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[0].wLowerVal = FELI_VOLT_BATT_HIGH_LOWER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[1].wUpperVal = FELI_VOLT_BATT_HIGH_UPPER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[1].wLowerVal = FELI_VOLT_BATT_HIGH_LOWER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[2].wUpperVal = FELI_VOLT_BATT_HIGH_UPPER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[2].wLowerVal = FELI_VOLT_BATT_HIGH_LOWER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[3].wUpperVal = FELI_VOLT_BATT_LOW_UPPER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[3].wLowerVal = FELI_VOLT_BATT_LOW_LOWER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[4].wUpperVal = FELI_VOLT_BATT_LOW_UPPER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[4].wLowerVal = FELI_VOLT_BATT_LOW_LOWER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[5].wUpperVal = FELI_VOLT_BATT_LOW_UPPER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[5].wLowerVal = FELI_VOLT_BATT_LOW_LOWER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
        }
        else
        {
            g_tFaultDataParam.tBattVolt[0].wUpperVal = NCM_VOLT_BATT_HIGH_UPPER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[0].wLowerVal = NCM_VOLT_BATT_HIGH_LOWER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[1].wUpperVal = NCM_VOLT_BATT_HIGH_UPPER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[1].wLowerVal = NCM_VOLT_BATT_HIGH_LOWER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[2].wUpperVal = NCM_VOLT_BATT_HIGH_UPPER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[2].wLowerVal = NCM_VOLT_BATT_HIGH_LOWER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[3].wUpperVal = NCM_VOLT_BATT_LOW_UPPER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[3].wLowerVal = NCM_VOLT_BATT_LOW_LOWER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[4].wUpperVal = NCM_VOLT_BATT_LOW_UPPER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[4].wLowerVal = NCM_VOLT_BATT_LOW_LOWER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[5].wUpperVal = NCM_VOLT_BATT_LOW_UPPER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[5].wLowerVal = NCM_VOLT_BATT_LOW_LOWER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell; 
        }
        g_tFaultDataParam.tBattCurr[0].fUpperVal = CURR_BATT_CHG_UPPER_I;
        g_tFaultDataParam.tBattCurr[0].fLowerVal = CURR_BATT_CHG_LOWER_I;
        g_tFaultDataParam.tBattCurr[1].fUpperVal = CURR_BATT_CHG_UPPER_II;
        g_tFaultDataParam.tBattCurr[1].fLowerVal = CURR_BATT_CHG_LOWER_II;
        g_tFaultDataParam.tBattCurr[2].fUpperVal = CURR_BATT_CHG_UPPER_III;
        g_tFaultDataParam.tBattCurr[2].fLowerVal = CURR_BATT_CHG_LOWER_III;
        g_tFaultDataParam.tBattCurr[3].fUpperVal = CURR_BATT_DISCHG_UPPER_I;
        g_tFaultDataParam.tBattCurr[3].fLowerVal = CURR_BATT_DISCHG_LOWER_I;
        g_tFaultDataParam.tBattCurr[4].fUpperVal = CURR_BATT_DISCHG_UPPER_II;
        g_tFaultDataParam.tBattCurr[4].fLowerVal = CURR_BATT_DISCHG_LOWER_II;
        g_tFaultDataParam.tBattCurr[5].fUpperVal = CURR_BATT_DISCHG_UPPER_III;
        g_tFaultDataParam.tBattCurr[5].fLowerVal = CURR_BATT_DISCHG_LOWER_III;
        g_tFaultDataParam.tBattSoc[0].byUpperVal = SOC_BATT_LOW_UPPER_I;
        g_tFaultDataParam.tBattSoc[0].byLowerVal = SOC_BATT_LOW_LOWER_I;
        g_tFaultDataParam.tBattSoc[1].byUpperVal = SOC_BATT_LOW_UPPER_II;
        g_tFaultDataParam.tBattSoc[1].byLowerVal = SOC_BATT_LOW_LOWER_II;
        g_tFaultDataParam.tBattSoc[2].byUpperVal = SOC_BATT_LOW_UPPER_III;
        g_tFaultDataParam.tBattSoc[2].byLowerVal = SOC_BATT_LOW_LOWER_III;

    }
    

    
    memset((uint8_t *)&g_tFaultCntParam, 0x00, sizeof(g_tFaultCntParam) / sizeof(uint8_t));
   
    return l_byResult;

}

uint8_t FaultLevelOneKey(uint8_t byBatType)
{
       
        if(byBatType == FeLi90Ah)
        {
            g_tFaultDataParam.tCellVolt[0].wUpperVal = FELI_VOLT_CELL_HIGH_UPPER_I;
            g_tFaultDataParam.tCellVolt[0].wLowerVal = FELI_VOLT_CELL_HIGH_LOWER_I;
            g_tFaultDataParam.tCellVolt[1].wUpperVal = FELI_VOLT_CELL_HIGH_UPPER_II;
            g_tFaultDataParam.tCellVolt[1].wLowerVal = FELI_VOLT_CELL_HIGH_LOWER_II;
            g_tFaultDataParam.tCellVolt[2].wUpperVal = FELI_VOLT_CELL_HIGH_UPPER_III;
            g_tFaultDataParam.tCellVolt[2].wLowerVal = FELI_VOLT_CELL_HIGH_LOWER_III;
            g_tFaultDataParam.tCellVolt[3].wUpperVal = FELI_VOLT_CELL_LOW_UPPER_I;
            g_tFaultDataParam.tCellVolt[3].wLowerVal = FELI_VOLT_CELL_LOW_LOWER_I;
            g_tFaultDataParam.tCellVolt[4].wUpperVal = FELI_VOLT_CELL_LOW_UPPER_II;
            g_tFaultDataParam.tCellVolt[4].wLowerVal = FELI_VOLT_CELL_LOW_LOWER_II;
            g_tFaultDataParam.tCellVolt[5].wUpperVal = FELI_VOLT_CELL_LOW_UPPER_III;
            g_tFaultDataParam.tCellVolt[5].wLowerVal = FELI_VOLT_CELL_LOW_LOWER_III;
            g_tFaultDataParam.tCellVolt[6].wUpperVal = FELI_VOLT_CELL_DIFF_UPPER_I;
            g_tFaultDataParam.tCellVolt[6].wLowerVal = FELI_VOLT_CELL_DIFF_LOWER_I;
            g_tFaultDataParam.tCellVolt[7].wUpperVal = FELI_VOLT_CELL_DIFF_UPPER_II;
            g_tFaultDataParam.tCellVolt[7].wLowerVal = FELI_VOLT_CELL_DIFF_LOWER_II;
            g_tFaultDataParam.tCellVolt[8].wUpperVal = FELI_VOLT_CELL_DIFF_UPPER_III;
            g_tFaultDataParam.tCellVolt[8].wLowerVal = FELI_VOLT_CELL_DIFF_LOWER_III;      
            
        }
        else
        {
            g_tFaultDataParam.tCellVolt[0].wUpperVal = NCM_VOLT_CELL_HIGH_UPPER_I;
            g_tFaultDataParam.tCellVolt[0].wLowerVal = NCM_VOLT_CELL_HIGH_LOWER_I;
            g_tFaultDataParam.tCellVolt[1].wUpperVal = NCM_VOLT_CELL_HIGH_UPPER_II;
            g_tFaultDataParam.tCellVolt[1].wLowerVal = NCM_VOLT_CELL_HIGH_LOWER_II;
            g_tFaultDataParam.tCellVolt[2].wUpperVal = NCM_VOLT_CELL_HIGH_UPPER_III;
            g_tFaultDataParam.tCellVolt[2].wLowerVal = NCM_VOLT_CELL_HIGH_LOWER_III;
            g_tFaultDataParam.tCellVolt[3].wUpperVal = NCM_VOLT_CELL_LOW_UPPER_I;
            g_tFaultDataParam.tCellVolt[3].wLowerVal = NCM_VOLT_CELL_LOW_LOWER_I;
            g_tFaultDataParam.tCellVolt[4].wUpperVal = NCM_VOLT_CELL_LOW_UPPER_II;
            g_tFaultDataParam.tCellVolt[4].wLowerVal = NCM_VOLT_CELL_LOW_LOWER_II;
            g_tFaultDataParam.tCellVolt[5].wUpperVal = NCM_VOLT_CELL_LOW_UPPER_III;
            g_tFaultDataParam.tCellVolt[5].wLowerVal = NCM_VOLT_CELL_LOW_LOWER_III;
            g_tFaultDataParam.tCellVolt[6].wUpperVal = NCM_VOLT_CELL_DIFF_UPPER_I;
            g_tFaultDataParam.tCellVolt[6].wLowerVal = NCM_VOLT_CELL_DIFF_LOWER_I;
            g_tFaultDataParam.tCellVolt[7].wUpperVal = NCM_VOLT_CELL_DIFF_UPPER_II;
            g_tFaultDataParam.tCellVolt[7].wLowerVal = NCM_VOLT_CELL_DIFF_LOWER_II;
            g_tFaultDataParam.tCellVolt[8].wUpperVal = NCM_VOLT_CELL_DIFF_UPPER_III;
            g_tFaultDataParam.tCellVolt[8].wLowerVal = NCM_VOLT_CELL_DIFF_LOWER_III;            
        }        
     
        if(g_tBatPackCtrl.tEESpec.byBatType == FeLi90Ah)
        {
            g_tFaultDataParam.tBattVolt[0].wUpperVal = FELI_VOLT_BATT_HIGH_UPPER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[0].wLowerVal = FELI_VOLT_BATT_HIGH_LOWER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[1].wUpperVal = FELI_VOLT_BATT_HIGH_UPPER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[1].wLowerVal = FELI_VOLT_BATT_HIGH_LOWER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[2].wUpperVal = FELI_VOLT_BATT_HIGH_UPPER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[2].wLowerVal = FELI_VOLT_BATT_HIGH_LOWER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[3].wUpperVal = FELI_VOLT_BATT_LOW_UPPER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[3].wLowerVal = FELI_VOLT_BATT_LOW_LOWER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[4].wUpperVal = FELI_VOLT_BATT_LOW_UPPER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[4].wLowerVal = FELI_VOLT_BATT_LOW_LOWER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[5].wUpperVal = FELI_VOLT_BATT_LOW_UPPER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[5].wLowerVal = FELI_VOLT_BATT_LOW_LOWER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
        }
        else
        {
            g_tFaultDataParam.tBattVolt[0].wUpperVal = NCM_VOLT_BATT_HIGH_UPPER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[0].wLowerVal = NCM_VOLT_BATT_HIGH_LOWER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[1].wUpperVal = NCM_VOLT_BATT_HIGH_UPPER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[1].wLowerVal = NCM_VOLT_BATT_HIGH_LOWER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[2].wUpperVal = NCM_VOLT_BATT_HIGH_UPPER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[2].wLowerVal = NCM_VOLT_BATT_HIGH_LOWER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[3].wUpperVal = NCM_VOLT_BATT_LOW_UPPER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[3].wLowerVal = NCM_VOLT_BATT_LOW_LOWER_I*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[4].wUpperVal = NCM_VOLT_BATT_LOW_UPPER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[4].wLowerVal = NCM_VOLT_BATT_LOW_LOWER_II*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[5].wUpperVal = NCM_VOLT_BATT_LOW_UPPER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell;
            g_tFaultDataParam.tBattVolt[5].wLowerVal = NCM_VOLT_BATT_LOW_LOWER_III*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell; 
        }
         if(FeLi90Ah == g_tBatPackCtrl.tEESpec.byBatType )
        {
            g_tBatPackCtrl.tEESpec.wCellVolMax = FELI_SLEFCHEK_CELL_VOLTAGE_UPPER;
            g_tBatPackCtrl.tEESpec.wCellVolMin = FELI_SLEFCHEK_CELL_VOLTAGE_LOWER;            
            g_tBatPackCtrl.tEESpec.wBalanceVolt = FELI_BMS_BALANCE_VOLT;
            g_tBatPackCtrl.tEESpec.wBalanceDiff = FELI_BMS_BALANCE_DIFF;
            g_tBatPackCtrl.tEESpec.byCellTempMax = FELI_SLEFCHEK_TEMPERATURE_UPPER;
            g_tBatPackCtrl.tEESpec.byCellTempMin = FELI_SLEFCHEK_TEMPERATURE_LOWER;
            g_tBatPackCtrl.tEESpec.wCellVolLowChg = FELI_VOLT_CELL_LOW_CHG;
        }    
        else
        {
            g_tBatPackCtrl.tEESpec.wCellVolMax = NCM_SLEFCHEK_CELL_VOLTAGE_UPPER;
            g_tBatPackCtrl.tEESpec.wCellVolMin = NCM_SLEFCHEK_CELL_VOLTAGE_LOWER;            
            g_tBatPackCtrl.tEESpec.wBalanceVolt = NCM_BMS_BALANCE_VOLT;
            g_tBatPackCtrl.tEESpec.wBalanceDiff = NCM_BMS_BALANCE_DIFF; 
            g_tBatPackCtrl.tEESpec.byCellTempMax = NCM_SLEFCHEK_TEMPERATURE_UPPER;
            g_tBatPackCtrl.tEESpec.byCellTempMin = NCM_SLEFCHEK_TEMPERATURE_LOWER;
            g_tBatPackCtrl.tEESpec.wCellVolLowChg = NCM_VOLT_CELL_LOW_CHG;
        }   
    memset((uint8_t *)&g_tFaultCntParam, 0x00, sizeof(g_tFaultCntParam) / sizeof(uint8_t));
   
    return Ok;
}

uint32_t flash_Erase_Sector(uint32_t address, uint32_t size)
{
    uint32_t error = 0;
    uint32_t offset = 0;
    FLASH_Unlock();
    if(FLASH_HSICLOCK_DISABLE == FLASH_ClockInit())
    {
 
        while(1);
    }    
    while(offset < size){
        
        if(FLASH_COMPL != FLASH_EraseOnePage(address + offset))
        {
            error = 1;
            break;
        }
        offset += 512;
    }
    
    
    FLASH_Lock();
    return error;
}

uint32_t flash_Write(uint32_t address, uint8_t* p_data, uint32_t len)
{
    uint32_t error = 0;
    uint32_t offset = 0;
        if(FLASH_HSICLOCK_DISABLE == FLASH_ClockInit())
    {
     
        while(1);
    }  
    FLASH_Unlock();
   
    while(offset < len){
        if(FLASH_COMPL != FLASH_ProgramWord(address + offset,  *(uint32_t *)(p_data + offset)))
        {
            error = 1;
            break;
        }
        offset += 4;
    }
    FLASH_Lock();
    return error;

}

/* BEGIN_FUNCTION_HDR
**********************************************************************************************
  Function:			ExecEnterUpdate(tSciMsgBufRxTx sUpMsg)            
  Version:          V1.0      
  Description:      串口更新函数
  Calls:                
  Called By:           
  Input:                
  Output:             
  Return:            
  Others:              
**********************************************************************************************
END_FUNCTION_HDR */
#if  EN_BOOTLOADER
//static void ExecEnterUpdate(USART_Module *USARTx,sUARTMsgType* sUpMsg)
//{
//    uint32_t dwWriteData=APP_UPGRADE_FLAG;
//    //串口更新
//    if ((sUpMsg->rbuf[0]==0x6D)&&(sUpMsg->rbuf[1]==0xAC)&&(sUpMsg->rbuf[6]==0x26)) //是APP更新帧
//    {        
//        //
//         uint8_t u8TxData[]={0x6D,0xAC,0x00,0xFF,0x0C,0x00,0x26,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x3A,0xB5};//IAP_BOOT_SIZE ==0x4000
//        //0x6D,0xAC,0x00,0xFF,0x0C,0x00,0x26,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0xBF,0xE7
//        //  uint8_t u8TxData[]={0x6D,0xAC,0x00,0xFF,0x0C,0x00,0x26,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x36,0x73};//IAP_BOOT_SIZE ==0x8000
//         RS485_Send_Data( u8TxData,ARRAY_SZ(u8TxData));
//        //复位命令
//        if((sUpMsg->rbuf[16]==0xA6)&&(sUpMsg->rbuf[17]==0xDA))//CRC校验
//        { 

//             if (0xFFFFFFFFUL != *(__IO uint32_t *)APP_UPGRADE_FLAG_ADDR) 
//             {              
//                flash_Erase_Sector(APP_UPGRADE_FLAG_ADDR, 4);
//             }
//             flash_Write(APP_UPGRADE_FLAG_ADDR, (uint8_t*)&dwWriteData, 4);
//   

//    /* Unlocks the FLASH Program Erase Controller */
//   

//            Delay_ms(50);                 //等待
//            
//            NVIC_SystemReset();  //软件复位MCU
//        }
//    }

//}



#define FRAME_CRC16_INIT_VALUE          (0xA28C)

uint16_t Cal_CRC16(uint8_t * p_data, uint16_t offset, uint32_t size)
{
	uint8_t u8Cnt;
	uint16_t u16CrcResult = FRAME_CRC16_INIT_VALUE;
	int u32Offset = offset;

	while (size != 0)
	{
		u16CrcResult ^= p_data[u32Offset++];
		for (u8Cnt = 0; u8Cnt < 8; u8Cnt++)
		{
			if ((u16CrcResult & 0x1) == 0x1)
			{
				u16CrcResult >>= 1;
				u16CrcResult ^= 0x8408;
			}
			else
			{
				u16CrcResult >>= 1;
			}
		}
		size--;
	}
	u16CrcResult = (uint16_t)(~u16CrcResult);

	return u16CrcResult;
}

static uint16_t FLASH_PageNumber(uint32_t u32Size)
{
    uint16_t u32PageNum = u32Size / FLASH_SECTOR_SIZE;

    if ((u32Size % FLASH_SECTOR_SIZE) != 0)
    {
        u32PageNum += 1u;
    }

    return u32PageNum;
}

en_frame_recv_status_t enFrameRecvStatus;

/* Frame and packet size */
#define FRAME_SHELL_SIZE                    8
#define PACKET_INSTRUCT_SEGMENT_SIZE        10
#define PACKET_DATA_SEGMENT_SIZE            512
#define FRAME_MIN_SIZE                      PACKET_INSTRUCT_SEGMENT_SIZE
#define FRAME_MAX_SIZE                      (PACKET_DATA_SEGMENT_SIZE + PACKET_INSTRUCT_SEGMENT_SIZE + FRAME_SHELL_SIZE)

/* Frame structure defines */
#define FRAME_HEAD_H_INDEX                  0x00
#define FRAME_HEAD_L_INDEX                  0x01
#define FRAME_NUM_INDEX                     0x02
#define FRAME_XORNUM_INDEX                  0x03
#define FRAME_LENGTH_INDEX                  0x04
#define FRAME_PACKET_INDEX                  0x06
#define FRAME_NUM_XOR_BYTE                  0xFF
/* Packet structure defines */
#define PACKET_CMD_INDEX                   (FRAME_PACKET_INDEX + 0x00)
#define PACKET_TYPE_INDEX                  (FRAME_PACKET_INDEX + 0x01)
#define PACKET_RESULT_INDEX                (FRAME_PACKET_INDEX + 0x01)
#define PACKET_ADDRESS_INDEX               (FRAME_PACKET_INDEX + 0x02)
#define PACKET_FLASH_CRC_INDEX             (FRAME_PACKET_INDEX + 0x0A)
#define PACKET_DATA_INDEX                  (FRAME_PACKET_INDEX + PACKET_INSTRUCT_SEGMENT_SIZE)

void Modem_SendFrame(USART_Module *USARTx,uint8_t *u8TxBuff, uint16_t u16TxLength)
{
    uint16_t u16Crc16;

    u8TxBuff[FRAME_LENGTH_INDEX] = u16TxLength & 0x00FF;                   //存储数据包长度
    u8TxBuff[FRAME_LENGTH_INDEX + 1] = u16TxLength >> 8;
    u16Crc16 = Cal_CRC16(u8TxBuff,FRAME_PACKET_INDEX, u16TxLength);      //计算数据包的CRC校验值
    u8TxBuff[FRAME_PACKET_INDEX + u16TxLength] = u16Crc16 & 0x00FF;        //存储CRC至数据包
    u8TxBuff[FRAME_PACKET_INDEX + u16TxLength + 1] = u16Crc16 >> 8;
    switch ((uint32_t)USARTx)
    {
      case (uint32_t)USARTy:           
        at_blesend(&u8TxBuff[0], FRAME_PACKET_INDEX + u16TxLength + 2);     //发送应答帧
        break;
      case (uint32_t)USART485:
        memcpy(g_sMainRtxMsg.wbuf, u8TxBuff, FRAME_PACKET_INDEX + u16TxLength + 2);
        g_sMainRtxMsg.w_num = FRAME_PACKET_INDEX + u16TxLength + 2;
        // 启动发送中断
        GPIO_SetBits(UARTTxRxm_PORT, UARTTxRxm_PIN);
        Delay_us(3);
        USART_ConfigInt(USART485, USART_INT_TXDE, ENABLE);        
        break;
      case (uint32_t)UART:
        memcpy(g_sUartRtxMsg.wbuf, u8TxBuff, FRAME_PACKET_INDEX + u16TxLength + 2);
        g_sUartRtxMsg.w_num =FRAME_PACKET_INDEX + u16TxLength + 2;
        USART_ConfigInt(UART, USART_INT_TXDE, ENABLE);
        break;
     default:
        break;
    }
}

bool updata_ble = false;
uint64_t g_u64LastCommMs = 0;

void read_flash(uint32_t addr, uint8_t *buffer, uint32_t byte_count)
{
    volatile uint8_t *flash_ptr = (volatile uint8_t*)addr;
    
    for(uint32_t i = 0; i < byte_count; i++)
    {
        buffer[i] = flash_ptr[i];
    }
}

/**
 *******************************************************************************
 ** \brief 上位机数据帧解析及处理
 **
 ** \param [in] None             
 **
 ** \retval Ok                          APP程序升级完成，并接受到跳转至APP命令
 ** \retval OperationInProgress         数据处理中
 ** \retval Error                       通讯错误
 **
 ******************************************************************************/
en_result_t Modem_Process(USART_Module *USARTx,sUARTMsgType* sUpMsg)
{
    uint8_t  u8Cmd, u8FlashAddrValid, u8Cnt, u8Ret;
    uint16_t u16DataLength,u16PageNum, u16Ret,u16Ret1;
    static uint32_t u32FlashAddr, u32FlashLength, u32Temp;
    static uint32_t u32AppAddr  = APP_ADDRESS;
    enFrameRecvStatus = FRAME_RECV_IDLE_STATUS;
    if ((sUpMsg->rbuf[FRAME_NUM_INDEX] != (sUpMsg->rbuf[FRAME_XORNUM_INDEX] ^ FRAME_NUM_XOR_BYTE)))                    //数据帧序号及校验值不匹配
    {
       // enFrameRecvStatus = FRAME_RECV_IDLE_STATUS;     //帧接收恢复到初始状态
        return Error;                                         //错误返回
    }
    uint32_t u32FrameSize = sUpMsg->rbuf[FRAME_LENGTH_INDEX] + (sUpMsg->rbuf[FRAME_LENGTH_INDEX + 1] << 8) + FRAME_SHELL_SIZE;  //计算此帧的长度
    if ((u32FrameSize < FRAME_MIN_SIZE) || (u32FrameSize > FRAME_MAX_SIZE))  //帧长度不在有效范围内
    {
       // enFrameRecvStatus = FRAME_RECV_IDLE_STATUS;     //帧接收恢复到初始状态
        return Error;                                         //错误返回
    } 
    uint16_t u16Crc16 = sUpMsg->rbuf[u32FrameSize-2] + (sUpMsg->rbuf[u32FrameSize-1]<<8);
    //if (CRC16_CalcBufCrc(&u8FrameData[FRAME_PACKET_INDEX], (u32FrameSize-FRAME_SHELL_SIZE)) == u16Crc16)                    //如果CRC校验通过
    if (Cal_CRC16(sUpMsg->rbuf,FRAME_PACKET_INDEX, (u32FrameSize-FRAME_SHELL_SIZE)) == u16Crc16)                    //如果CRC校验通过
    {
        enFrameRecvStatus = FRAME_RECV_PROC_STATUS;     //帧接收进入下一状态:  帧处理状态                    
    }else                                               //校验失败
    {
        //enFrameRecvStatus = FRAME_RECV_IDLE_STATUS;     //帧接收恢复到初始状态
        return Error;                                         //错误返回
    }
    if (enFrameRecvStatus == FRAME_RECV_PROC_STATUS)                //有数据帧待处理, enFrameRecvStatus值在串口中断中调整
    {
        u8Cmd = sUpMsg->rbuf[PACKET_CMD_INDEX];                      //获取帧指令码
        if (PACKET_CMD_TYPE_DATA == sUpMsg->rbuf[PACKET_TYPE_INDEX]) //如果是数据指令
        {
            u8FlashAddrValid = 0u;
            
            u32FlashAddr = FLASH_BASE+sUpMsg->rbuf[PACKET_ADDRESS_INDEX] +      //读取地址值
                           (sUpMsg->rbuf[PACKET_ADDRESS_INDEX + 1] << 8)  +
                           (sUpMsg->rbuf[PACKET_ADDRESS_INDEX + 2] << 16) +
                           (sUpMsg->rbuf[PACKET_ADDRESS_INDEX + 3] << 24);
            if ((u32FlashAddr >= (FLASH_BASE + BOOT_SIZE)) && (u32FlashAddr < (FLASH_BASE + FLASH_SIZE)))  //如果地址值在有效范围内
            {
                u8FlashAddrValid = 1u;                              //标记地址有效
            }
        }
        
        switch (u8Cmd)                                              //根据指令码跳转执行
        {
            case  PACKET_CMD_HANDSHAKE    :                         //握手帧 指令码
                sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_OK;   //返回状态为：正确
                Modem_SendFrame(USARTx,&sUpMsg->rbuf[0], PACKET_INSTRUCT_SEGMENT_SIZE);   //发送应答帧给上位机
                break;
            case  PACKET_CMD_ERASE_FLASH  :                         //擦除flash 指令码
                if ((u32FlashAddr % FLASH_SECTOR_SIZE) != 0)        //如果擦除地址不是页首地址
                {
                    u8FlashAddrValid = 0u;                          //标记地址无效
                }

                if (1u == u8FlashAddrValid)                         //如果地址有效
                {
                    u32Temp = sUpMsg->rbuf[PACKET_DATA_INDEX] +      //获取待擦除flash尺寸
                              (sUpMsg->rbuf[PACKET_DATA_INDEX + 1] << 8)  +
                              (sUpMsg->rbuf[PACKET_DATA_INDEX + 2] << 16) +
                              (sUpMsg->rbuf[PACKET_DATA_INDEX + 3] << 24);
                    u16PageNum = FLASH_PageNumber(u32Temp);          //计算需擦除多少页
                    u32AppAddr = u32FlashAddr;
                       u8Ret = flash_Erase_Sector((u32FlashAddr + UPDATE_SIZE ), (u16PageNum * FLASH_SECTOR_SIZE));
                        if (Ok != u8Ret)                             //如果擦除失败，反馈上位机错误代码
                        {
                            sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_ERROR;
                            break;
                        }
                    if (Ok == u8Ret)                                 //如果全部擦除成功，反馈上位机成功
                    {
                        sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_OK;
                    }else                                            //如果擦除失败，反馈上位机错误超时标志
                    {
                        sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_TIMEOUT;
                    }
                }
                else                                                 //地址无效，反馈上位机地址错误
                {
                    sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_ADDR_ERROR;
                }
                Modem_SendFrame(USARTx,&sUpMsg->rbuf[0], PACKET_INSTRUCT_SEGMENT_SIZE);             //发送应答帧到上位机
                break;
            case  PACKET_CMD_APP_DOWNLOAD :                          //数据下载 指令码
                if (1u == u8FlashAddrValid)                          //如果地址有效
                {
                    u16DataLength = sUpMsg->rbuf[FRAME_LENGTH_INDEX] + (sUpMsg->rbuf[FRAME_LENGTH_INDEX + 1] << 8)
                                     - PACKET_INSTRUCT_SEGMENT_SIZE; //获取数据包中的数据长度(不包含指令码指令类型等等)
                    if (u16DataLength > PACKET_DATA_SEGMENT_SIZE)    //如果数据长度大于最大长度
                    {
                        u16DataLength = PACKET_DATA_SEGMENT_SIZE;    //设置数据最大值
                    }
                   
                    u8Ret = flash_Write((u32FlashAddr + UPDATE_SIZE), (uint8_t *)&sUpMsg->rbuf[PACKET_DATA_INDEX], u16DataLength); //把所有数据写入flash
                    if (Ok != u8Ret)                                 //如果写数据失败       
                    {
                        sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_ERROR;                //反馈上位机错误 标志
                    }
                    else                                             //如果写数据成功
                    {
                        sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_OK;                   //反馈上位机成功 标志
                    }
                }
                else                                                 //如果地址无效
                {
                    sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_ADDR_ERROR;               //反馈上位机地址错误
                }
                Modem_SendFrame(USARTx,&sUpMsg->rbuf[0], PACKET_INSTRUCT_SEGMENT_SIZE);             //发送应答帧到上位机
                break;
            case  PACKET_CMD_CRC_FLASH    :                          //查询flash校验值 指令码
                if (1u == u8FlashAddrValid)                          //如果地址有效
                {
                    u32FlashLength =  sUpMsg->rbuf[PACKET_DATA_INDEX] +                 
                                    (sUpMsg->rbuf[PACKET_DATA_INDEX + 1] << 8)  +
                                    (sUpMsg->rbuf[PACKET_DATA_INDEX + 2] << 16) +
                                    (sUpMsg->rbuf[PACKET_DATA_INDEX + 3] << 24);             //获取待校验flash大小
                    if ((u32FlashLength + u32FlashAddr) > (FLASH_BASE + FLASH_SIZE))        //如果flash长度超出有效范围
                    {
                        sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_FLASH_SIZE_ERROR;     //反馈上位机flash尺寸错误
                    }else
                    {   
                        u16Ret1 = Cal_CRC16(((unsigned char *)(u32FlashAddr)),0, u32FlashLength);
                        u16Ret = Cal_CRC16(((unsigned char *)(u32FlashAddr + UPDATE_SIZE)),0, u32FlashLength);//读取flash指定区域的值并计算crc值
                        sUpMsg->rbuf[PACKET_FLASH_CRC_INDEX] = (uint8_t)u16Ret;              //把crc值存储到应答帧
                        sUpMsg->rbuf[PACKET_FLASH_CRC_INDEX+1] = (uint8_t)(u16Ret>>8);
                        sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_OK;                   //反馈上位机成功 标志
                    }
                }
                else                                                                        //如果地址无效
                {
                    sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_ADDR_ERROR;               //反馈上位机地址错误
                }
                Modem_SendFrame(USARTx,&sUpMsg->rbuf[0], PACKET_INSTRUCT_SEGMENT_SIZE+2);           //发送应答帧到上位机
                break;
            case  PACKET_CMD_JUMP_TO_APP  :                          //跳转至APP 指令码
                flash_Erase_Sector(APP_RUN_ADDR_BLE, 4);                //擦除BOOT parameter 扇区
                u32Temp = APP_FLAG;
                flash_Write(BOOT_PARA_ADDRESS_BLE, (uint8_t *)&u32Temp, 4);
                flash_Write(APP_RUN_ADDR_BLE, (uint8_t *)&u32AppAddr, 4);
                sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_OK;    //反馈上位机成功
                Modem_SendFrame(USARTx,&sUpMsg->rbuf[0], PACKET_INSTRUCT_SEGMENT_SIZE);             //发送应答帧到上位机
                updata_ble = true;
                return Ok;                                           //APP更新完成，返回OK，接下来执行跳转函数，跳转至APP
            case  PACKET_CMD_APP_UPLOAD   :                          //数据上传
                if (1u == u8FlashAddrValid)                          //如果地址有效
                {
                    u32Temp = sUpMsg->rbuf[PACKET_DATA_INDEX] +
                              (sUpMsg->rbuf[PACKET_DATA_INDEX + 1] << 8)  +
                              (sUpMsg->rbuf[PACKET_DATA_INDEX + 2] << 16) +
                              (sUpMsg->rbuf[PACKET_DATA_INDEX + 3] << 24);                   //读取上传数据长度
                    if (u32Temp > PACKET_DATA_SEGMENT_SIZE)                                 //如果数据长度大于最大值
                    {
                        u32Temp = PACKET_DATA_SEGMENT_SIZE;                                 //设置数据长度为最大值
                    }
                    read_flash(u32FlashAddr, (uint8_t *)&sUpMsg->rbuf[PACKET_DATA_INDEX], u32Temp); //读flash数据
                    sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_OK;                       //反馈上位机成功 标志
                    Modem_SendFrame(USARTx,&sUpMsg->rbuf[0], PACKET_INSTRUCT_SEGMENT_SIZE + u32Temp);//发送应答帧到上位机
                }
                else                                                  //如果地址无效
                {
                    sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_ADDR_ERROR;               //反馈上位机地址错误 标志
                    Modem_SendFrame(USARTx,&sUpMsg->rbuf[0], PACKET_INSTRUCT_SEGMENT_SIZE);         //发送应答帧到上位机
                }
                break;
            case  PACKET_CMD_START_UPDATE :                           //启动APP更新(此指令正常在APP程序中调用)
                sUpMsg->rbuf[PACKET_RESULT_INDEX] = PACKET_ACK_OK;     //反馈上位机成功 标志
                sUpMsg->rbuf[PACKET_DATA_INDEX]      = (uint8_t)(( BOOT_SIZE) & 0xFFU);
                sUpMsg->rbuf[PACKET_DATA_INDEX + 1U] = (uint8_t)((( BOOT_SIZE) >> 8U) & 0xFFU);
                Modem_SendFrame(USARTx,&sUpMsg->rbuf[0], PACKET_INSTRUCT_SEGMENT_SIZE+2);             //发送应答帧到上位机
                break;
        }
        enFrameRecvStatus = FRAME_RECV_IDLE_STATUS;                   //帧数据处理完成，帧接收状态恢复到空闲状态
    }
    
    return OperationInProgress;                                       //返回，APP更新中。。。
}

static void ExecEnterUpdate(USART_Module *USARTx,sUARTMsgType* sUpMsg)
{
    uint32_t dwWriteData=APP_UPGRADE_FLAG;
   

    //BLE更新
    if ((sUpMsg->rbuf[0] == 0x6D) && (sUpMsg->rbuf[1] == 0xAC) && (sUpMsg->rbuf[6] == 0x26)) // 是APP更新帧
    {
        //
        uint8_t u8TxData[] = {0x6D, 0xAC, 0x00, 0xFF, 0x0C, 0x00, 0x26, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x3A, 0xB5}; // IAP_BOOT_SIZE ==0x4000
        // 0x6D,0xAC,0x00,0xFF,0x0C,0x00,0x26,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0xBF,0xE7
        //uint8_t u8TxData[]={0x6D,0xAC,0x00,0xFF,0x0C,0x00,0x26,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x36,0x73};//IAP_BOOT_SIZE ==0x8000
         if(USARTx==USART485)
         {
            memcpy(g_sMainRtxMsg.wbuf, u8TxData, ARRAY_SZ(u8TxData));
            g_sMainRtxMsg.w_num = ARRAY_SZ(u8TxData);
            // 启动发送中断
            GPIO_SetBits(UARTTxRxm_PORT, UARTTxRxm_PIN);
            Delay_us(3);
            USART_ConfigInt(USART485, USART_INT_TXDE, ENABLE);
         }             
         else if(USARTx==UART)
         {
            memcpy(g_sUartRtxMsg.wbuf, u8TxData, ARRAY_SZ(u8TxData));
            g_sUartRtxMsg.w_num =ARRAY_SZ(u8TxData);
            USART_ConfigInt(UART, USART_INT_TXDE, ENABLE);
         }             
         else if(USARTx==USARTy)   
            at_blesend(u8TxData, ARRAY_SZ(u8TxData));
        // 复位命令
	    uint16_t l_wCalcCrc = App_Crc16Calculate(sUpMsg->rbuf, 16);
        uint16_t l_wRecvCrc = ((uint16_t)sUpMsg->rbuf[17] << 8) | sUpMsg->rbuf[16];
        //if ((sUpMsg->rbuf[16] == 0xA6) && (sUpMsg->rbuf[17] == 0xDA)) // CRC校验
        {

            if (0xFFFFFFFFUL != *(__IO uint32_t *)APP_UPGRADE_FLAG_ADDR_BLE)
            {
                flash_Erase_Sector(APP_UPGRADE_FLAG_ADDR_BLE, 4);
            }


            // Delay_ms(50);                 //等待

            // NVIC_SystemReset();  //软件复位MCU
        }
    }
	    else if((sUpMsg->rbuf[0] == 0x6D) && (sUpMsg->rbuf[1] == 0xAC) &&(sUpMsg->rbuf[6]>= 0x20&&sUpMsg->rbuf[6] <= 0x26) )
    {
        Modem_Process(USARTx,sUpMsg);
    }
}


#endif

static bool recv_isOk(uint8_t* p_buf, uint16_t len, uint8_t salve_id)
{
	 if(len < 8)
		 return false;
	 
	 if(p_buf[0] != salve_id)
		 return false;
	 
	 if(len == 8)
	 {
			if((p_buf[1] == 0x03) || (p_buf[1] == 0x06))
				return true;
	 } else if((p_buf[1] == 0x10) || (p_buf[1] == 0x57))
	 {
		  uint16_t reg_num = p_buf[4] << 8 | p_buf[5];
		  if((reg_num * 2 == p_buf[6]) && (len == (7 + reg_num * 2 + 2)))
				return true;
	 }

	 return false;
}

 
void ExecDiagnoseCmd(void)
{ 
    tDignoseParamIn l_tIn;
    l_tIn = g_tProDignoseCtrl;
    if(PASSWORD_DIAG == l_tIn.wPassCode)
    {
        g_tBatPackCtrl.eDiagMode = eBasicDiag;

        if (l_tIn.uRelayCtrl1.tBits.bPos)
        {
            CHG_FETControl(FET_OPEN);
        }
        else
        {
            CHG_FETControl(FET_Close);
        }

        if (l_tIn.uRelayCtrl1.tBits.bNeg)
        {
            DSG_FETControl(FET_OPEN);
        }
        else
        {
            DSG_FETControl(FET_Close);
        }

        if (l_tIn.uRelayCtrl1.tBits.bBm1)
        {
            GPIO_SetBits(HEAT_PORT, HEAT_HTCTRL_PIN);
            g_tDiagnosticMsg.uRelaySt.tBits.bBm1 = 1;
        }
        else
        {
            GPIO_ResetBits(HEAT_PORT, HEAT_HTCTRL_PIN);
            g_tDiagnosticMsg.uRelaySt.tBits.bBm1 = 0;
        }
        
        if (l_tIn.uRelayCtrl1.tBits.bBm2)
        {
            g_tDiagnosticMsg.uRelaySt.tBits.bBm2 = 1;
        }
        else
        {            
            g_tDiagnosticMsg.uRelaySt.tBits.bBm2 = 0;
        }
        if(l_tIn.uRelayCtrl1.tBits.bSoc)
        {
            g_fRefSoc = l_tIn.wCalibrateVal;  
            g_fShowSoc  = g_fRefSoc;             
            g_tBatPackInfo.wSocRef = g_fShowSoc;        
            g_tBatPackInfo.wSocShow = g_fShowSoc;
            g_tBatPackCtrl.tEERun.wSocRef = g_tBatPackInfo.wSocRef;        
            g_tBatPackCtrl.tEERun.wSocShow = g_tBatPackInfo.wSocShow;
            byEEMemoryWrite(EE_RUN_ADDR, (uint8_t *)& g_tBatPackCtrl.tEERun, EE_RUN_LEN);
        }        
       
        if(l_tIn.byEquCmd  == 1)     
        {
            // 0x01:按预定值均衡；
            g_tBatPackCtrl.tEESpec.cbBlanceType = eLecuEquPreSet;
            g_tBatPackCtrl.tEESpec.wBalanceVolt = FELI_BMS_BALANCE_VOLT;
            g_tBatPackCtrl.tEESpec.wBalanceDiff = 50;
        }
        else if(l_tIn.byEquCmd == 2)     
        {
            // 0x02:按设定值被动均衡
            // 均衡电压设定，此时分辨率为0.001
            g_tBatPackCtrl.tEESpec.cbBlanceType = eLecuEquPassiVal;
            g_tBatPackCtrl.tEESpec.wBalanceVolt = l_tIn.wCalibrateVal;  
            g_tBatPackCtrl.tEESpec.wBalanceDiff = 50;            
        }
        else if(l_tIn.byEquCmd == 3)     
        {
            // 0x03:按设定值主动均衡
            // 均衡电压为单体最小电压，此时分辨率为0.001
            g_tBatPackCtrl.tEESpec.cbBlanceType = eLecuEquInitiVal;
            g_tBatPackCtrl.tEESpec.wBalanceVolt = g_tBatPackInfo.tCell.wVolMin; 
            g_tBatPackCtrl.tEESpec.wBalanceDiff = 50;
        }
        else if(l_tIn.byEquCmd == 0)   
        {              
            // 0x00:停止均衡  
//            g_tBatPackCtrl.tEESpec.cbBlanceType  = eLecuEquStop;
//            g_tBatPackCtrl.tEESpec.wBalanceVolt = 0;
//            g_tBatPackCtrl.tEESpec.wBalanceDiff = 0;
        }
        else
        {
            g_tBatPackCtrl.tEESpec.cbBlanceType  = eLecuEquStop;
            g_tBatPackCtrl.tEESpec.wBalanceVolt = 0;
            g_tBatPackCtrl.tEESpec.wBalanceDiff = 0;
        }
        
    }
    else if(PASSWORD_PRO_DIAG == l_tIn.wPassCode)
    {
        g_tBatPackCtrl.eDiagMode = eProDiag;
    }
    else if(PASSWORD_XCP == l_tIn.wPassCode)
    {
        
        g_tBatPackCtrl.eDiagMode = eXcpDiag;
        
//        RPAGE = 0xF3;
//        *(unsigned int *)0x14FE = 0x0000;                    // 设定标志位
    }
    else 
    {
        g_tBatPackCtrl.eDiagMode = eNoDiag;
    }
}

void ExecMainMsgProcess(void)
{

    sUARTMsgType* sRxMsg = &g_sMainRtxMsg;
    if (sRxMsg->r_index == 0) LogMemorySendPoll(USART485);   /* 空闲拍：推进日志分批发送(485) */ 
 
    if(sRxMsg->r_index > 0)
		{
			 g_u64LastCommMs = GetTick_ms();
    
        bool is_end = false;
        if (sRxMsg->Timeout_ms == 0)
        {
           is_end = true;
        }
        if(!is_end)
		is_end = recv_isOk(sRxMsg->rbuf, sRxMsg->r_index, g_tBatPackCtrl.tEESpec.byBmsId);
        if(is_end)
        {
#if EN_BOOTLOADER
            if (sRxMsg->r_index >= 18)
                ExecEnterUpdate(USART485, sRxMsg);
#endif
              
            if (Ok == ExecModbusMasterProcess(USART485, sRxMsg))
            {
                g_tBatPackInfo.tBmDiagVal.tBits.u1485CommFlg = 0;
             
            }
        memset(sRxMsg->rbuf, 0, sRxMsg->r_index);	
		sRxMsg->r_index = 0;
        }

    }
  
}

void ExecBleMsgProcess(void)
{
   sUARTMsgType* sRxMsg = &g_sBleRtxMsg;
   if (sRxMsg->r_index == 0) LogMemorySendPoll(USARTy);   /* 空闲拍：推进日志分批发送(BLE) */
   if(sRxMsg->r_index > 0)
    { g_u64LastCommMs = GetTick_ms();
   
       bool is_end = false;
       if (sRxMsg->Timeout_ms == 0)
       {
          is_end = true;
       }
       if(!is_end)
		is_end = recv_isOk(sRxMsg->rbuf, sRxMsg->r_index, g_tBatPackCtrl.tEESpec.byBmsId);
       if(is_end)
       {
#if EN_BOOTLOADER
           if (sRxMsg->r_index >= 18)
               ExecEnterUpdate(USARTy,sRxMsg);
#endif
             
           if (Ok == ExecModbusMasterProcess(USARTy,sRxMsg))
           {        
               g_tBatPackInfo.tBmDiagVal.tBits.u1BluCommFlg = 0;
            
           }
             	memset(sRxMsg->rbuf, 0, sRxMsg->r_index);	
				sRxMsg->r_index = 0;
       }
   }
   
}



void ExecUartMsgProcess(void)
{

    sUARTMsgType* sRxMsg = &g_sUartRtxMsg;
    if (sRxMsg->r_index == 0) LogMemorySendPoll(UART);   /* 空闲拍：推进日志分批发送(UART) */ 
 
    if(sRxMsg->r_index > 0)
   {  g_u64LastCommMs = GetTick_ms();
    
        bool is_end = false;
        if (sRxMsg->Timeout_ms == 0)
        {
           is_end = true;
        }
        if(!is_end)
		is_end = recv_isOk(sRxMsg->rbuf, sRxMsg->r_index, g_tBatPackCtrl.tEESpec.byBmsId);
        if(is_end)
        {
 #if EN_BOOTLOADER
            if (sRxMsg->r_index >= 18)
                ExecEnterUpdate(UART, sRxMsg);
#endif
            if (Ok == ExecModbusMasterProcess(UART, sRxMsg))
            {
                g_tBatPackInfo.tBmDiagVal.tBits.u1485CommFlg = 0;
             
            }
        memset(sRxMsg->rbuf, 0, sRxMsg->r_index);	
		sRxMsg->r_index = 0;
        }

    }
  
}

/* BMS history log: record every 30 seconds */
void ExecLogBmStRecord(void)
{
    tLogBmStItem sItem;

    memset((uint8_t *)&sItem, 0xFF, sizeof(sItem));
    sItem.dwUtcTime      = DATE_DateToUtc(g_tBatPackCtrl.tEERun.tRunTime);
    sItem.wBatVolt       = g_tBatPackInfo.wBatVolt;
    sItem.wCapVolt       = g_tBatPackInfo.wCapVolt;
    sItem.wBatCur        = g_tBatPackInfo.wCur;
    sItem.wSoc           = g_tBatPackInfo.wSocShow;
    sItem.wRelayState    = g_tDiagnosticMsg.uRelaySt.wWord;
    sItem.wCellVoltMax   = g_tBatPackInfo.tCell.wVolMax;
    sItem.wCellVoltMin   = g_tBatPackInfo.tCell.wVolMin;
    sItem.byBmState      = (uint8_t)g_tBatPackInfo.eCtrlMode;
    sItem.byBmFltLevl    = (uint8_t)g_tBatPackInfo.eFltLevel;
    sItem.byCellTempMax  = g_tBatPackInfo.tCell.byTempMax;
    sItem.byCellTempMin  = g_tBatPackInfo.tCell.byTempMin;
    sItem.byRsv01[0]     = g_tDiagnosticMsg.tLecu.uUpload.byByte;

    LOG_BmsStItemWrite(&sItem);
}

/* Alarm log: record when fault bits change */
void ExecLogErrStRecord(void)
{
    static uint16_t s_wLastFaultBits = 0;
    static uint8_t  s_byLastFaultLevel = 0;
    tLogErrStItem sItem;

    uint16_t wFaultBits = g_tBatPackInfo.tBmFltVal.wByte;
    uint8_t  byFltLevel = (uint8_t)g_tBatPackInfo.eFltLevel;

    if (wFaultBits == s_wLastFaultBits && byFltLevel == s_byLastFaultLevel)
        return;
    if (wFaultBits == 0 && byFltLevel == eBmFltNone)
        return;

    s_wLastFaultBits   = wFaultBits;
    s_byLastFaultLevel = byFltLevel;

    memset((uint8_t *)&sItem, 0xFF, sizeof(sItem));
    sItem.dwUtcTime       = DATE_DateToUtc(g_tBatPackCtrl.tEERun.tRunTime);
    sItem.wFaultBits      = wFaultBits;
    sItem.wFaultVal       = g_tBatPackInfo.tBmFltVal.wByte;
    sItem.wDiagVal        = g_tBatPackInfo.tBmDiagVal.wByte;
    sItem.byFaultLevel    = byFltLevel;
    sItem.wCellVoltMax    = g_tBatPackInfo.tCell.wVolMax;
    sItem.byCellVoltMaxId = g_tBatPackInfo.tCell.byVolMaxCellId;
    sItem.wCellVoltMin    = g_tBatPackInfo.tCell.wVolMin;
    sItem.byCellVoltMinId = g_tBatPackInfo.tCell.byVolMinCellId;
    sItem.byTempMax       = g_tBatPackInfo.tCell.byTempMax;
    sItem.byTempMin       = g_tBatPackInfo.tCell.byTempMin;
    sItem.wBatVolt        = g_tBatPackInfo.wBatVolt;
    sItem.wBatCur         = g_tBatPackInfo.wCur;

    LOG_ErrStItemWrite(&sItem);
}

void CurDriftCalib_Init(void)
{
    if (g_fRawCur > -0.4f && g_fRawCur < 0.4f)
    {
        g_bCurDriftCalibActive = true;
        g_u64CurDriftCalibStartTime = GetTick_ms();
        g_fCurDriftCalibSum = 0.0f;
        g_u32CurDriftCalibCount = 0;
        g_byCurDriftCalibStatus = 1;
    }
    else
    {
        g_byCurDriftCalibStatus = 3;
    }
}

void CurDriftCalib_Process(void)
{
    if (!g_bCurDriftCalibActive)
        return;
    if (bDSGING || bCHGING)
    {
        g_byCurDriftCalibStatus = 3;
        g_bCurDriftCalibActive = false;
        return;
    }
    if (g_fRawCur > 0.4f || g_fRawCur < -0.4f)
    {
        g_byCurDriftCalibStatus = 3;
        g_bCurDriftCalibActive = false;
        return;
    }

    uint64_t now = GetTick_ms();
    if (now - g_u64CurDriftCalibStartTime >= CUR_DRIFT_CALIB_DURATION_MS)
    {
        if (g_u32CurDriftCalibCount > 0)
        {
            float avgRawCur = g_fCurDriftCalibSum / g_u32CurDriftCalibCount;
            g_wBatCurDrift = avgRawCur * 10.0f - (float)g_tBatPackCtrl.tEESpec.wBatCurOffset;
            g_byCurDriftCalibStatus = 2;
        }
        else
        {
            g_byCurDriftCalibStatus = 3;
        }
        g_bCurDriftCalibActive = false;
    }
    else
    {
        g_fCurDriftCalibSum += g_fRawCur;
        g_u32CurDriftCalibCount++;
    }
}
