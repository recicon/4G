/**
 * @file UserDef.h
 * @brief BMS用户定义头文件
 */

#ifndef __USERDEF_H__
#define __USERDEF_H__

#include "bms_config.h"
#include <stdint.h>
/*============================================================================
 *                          数据类型定义
 *============================================================================*/

/* 故障等级 */
typedef enum
{
    eBmFltNone = 0,
    eBmFltLevel1,
    eBmFltLevel2,
    eBmFltLevel3,
    eBatFaultLevel_SIZE = 0x7FFFFFFF
} eBatFaultLevel;

/* 诊断状态 */
typedef enum
{
    eNoDiag = 0,
    eBasicDiag,
    eXcpDiag,
    eProDiag,
    eDiagCtrlMode_SIZE = 0x7FFFFFFF
} eDiagCtrlMode;

/* 控制状态 */
typedef enum
{
    eBmPowerOn = 0,
    eBmStandBy,
    eBmPrechg,
    eBmQdsg,
    eBmQchg,
    eBmErr,
    eBmWaring,
    eBmDiag,
    eBmWaiting,
    eBmPowerOff,
    eBatCtrlMode_SIZE = 0x7FFFFFFF
} eBatCtrlMode;

/* 系统诊断 */
typedef enum
{
    eDiag485Comm = 0,
    eDiagCanComm,
    eDiagHotComm,
    eDiagBluComm,
    eDiagRamSpi,
    eDiagEepromI2c,
    eDiagPreChg,
    eDiagRelayAdhesion,
    eDiagPowerCmd,
    eDiagBalance,
    eDiag4GComm,
    eDiagFltBit_SIZE = 0x7FFFFFFF
} eDiagFltBit;

/* 放电电流限制等级 */
typedef enum
{
    eCurrLimitLevel0 = 0,
    eCurrLimitLevel1,
    eCurrLimitLevel2,
    eCurrLimitLevel3,
    eCurrLimitLevel4,
    eCurrLimitLevel5,
    EUNM_SIZE = 0x7FFFFFFF
} eDischgCurrLimit;

/* 均衡方式 */
typedef enum
{
    eLecuEquStop = 0,
    eLecuEquPreSet,
    eLecuEquPassiVal,
    eLecuEquInitiVal,
    eLecuEquCmd_SIZE = 0x7FFFFFFF
} eLecuEquCmd;

/* 预充状态 */
typedef enum
{
    ePreRelayInit = 0,
    ePreRelayPreChg,
    ePreRelayPreDsg,
    ePreRelayNormal,
    ePreRelayPos,
    ePreRelayOff,
    ePreChgRelaySt_SIZE = 0x7FFFFFFF
} ePreChgRelaySt;

/* 充放电控制 */
typedef union
{
    uint8_t byByte;
    struct
    {
        uint8_t bDischgEn : 1;
        uint8_t bChgEn : 1;
        uint8_t bChgFull : 1;
        uint8_t bDischgEmpty : 1;
        uint8_t bRsv01 : 4;
    } tBits;
} uPcsPowerCtrl;

/* 电池信息结构 */
typedef struct
{
    uint16_t wVolSum;
    uint16_t wVolMax;
    uint16_t wVolMin;
    uint16_t wVolMean;
    uint16_t wVolt[N_PACK_CELL_MAX];
    uint8_t byTemp[N_PACK_TEMP_MAX];
    uint8_t byTempMean;
    uint8_t byTempMax;
    uint8_t byTempMin;
    uint8_t byTempMaxCellId;
    uint8_t byTempMinCellId;
    uint8_t byVolMaxCellId;
    uint8_t byVolMinCellId;
    uint8_t byTempAFE;
    uint8_t lstBlance[4];
} tCellInfo;

/* 功率限制 */
typedef struct
{
    uint16_t wMaxDisChgCur;
    uint16_t wMaxChgCur;
    uint16_t wMaxFeedbackChgCur;
} tBatPowerLimit;

/* 故障等级 */
typedef struct
{
    uint8_t byECellTempHigh;
    uint8_t byECellTempLow;
    uint8_t byECellVoltHigh;
    uint8_t byECellVoltLow;
    uint8_t byEChgCurrMuch;
    uint8_t byEChgCurrLowTempMuch;
    uint8_t byEDisChgCurrMuch;
    uint8_t byEBatVoltHigh;
    uint8_t byEBatVoltLow;
    uint8_t byECellDiffTempMuch;
    uint8_t byECellDiffVoltMuch;
    uint8_t byESocLow;
    uint8_t byEInsulLow : 1;
    uint8_t byAfeTempHigh : 1;
    uint8_t byMosTempHigh : 1;
    uint8_t byRsv : 5;
    uint8_t byRsv01[3];
} tBatFaultLevel;

/* 故障值 */
typedef union
{
    uint16_t wByte;
    struct
    {
        uint16_t u1ShortFlg : 1;
        uint16_t u1TempFlg : 1;
        uint16_t u1DisChgFlg : 1;
        uint16_t u1ChgFlg : 1;
        uint16_t u1CellFlg : 1;
        uint16_t uSOCFlg : 1;
        uint16_t u1BatFlg : 1;
        uint16_t u1Rsv : 9;
    } tBits;
} uBatBmFltVal;

/* 诊断值 */
typedef union
{
    uint16_t wByte;
    struct
    {
        uint16_t u1AfeCommFlg : 1;
        uint16_t u1485CommFlg : 1;
        uint16_t u1CanCommFlg : 1;
        uint16_t u1LcdCommFlg : 1;
        uint16_t u14GCommFlg : 1;
        uint16_t u1BluCommFlg : 1;
        uint16_t u1RamSpiFlg : 1;
        uint16_t u1EepromI2cFlg : 1;
        uint16_t u1PreChgFlg : 1;
        uint16_t u1RelayAdhesionFlg : 1;
        uint16_t u2BalanceFlg : 1;
        uint16_t u1Rsv : 5;
    } tBits;
} uBatBmDiagVal;

/* 电池包信息 */
typedef struct
{
    int16_t wCur;
    uint16_t wAbsCur;
    uint16_t wBatVolt;
    uint16_t wCapVolt;
    uint16_t wSocRef;
    uint16_t wSocShow;
    eBatCtrlMode eCtrlMode;
    ePreChgRelaySt ePreRelaySt;
    uint16_t wARegAi;
    uint16_t wResPos;
    uint16_t wResNeg;
    tCellInfo tCell;
    uint8_t byTemp1;
    uint8_t byTemp2;
    uBatBmFltVal tBmFltVal;
    uBatBmDiagVal tBmDiagVal;
    eBatFaultLevel eFltLevel;
    eBatFaultLevel eFltLevelNoSoc;
    tBatPowerLimit tPowerLimit;
    tBatFaultLevel tFaultLevel;
    uint8_t byRsv01[14];
} tBatPackInfo;

/* 时间参数 */
typedef struct
{
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} tTimeParam;

/* EEPROM运行参数 */
typedef struct
{
    tTimeParam tRunTime;
    int16_t wSocShow;
    int16_t wSocRef;
    uint64_t dwChgAhrs;
    uint64_t dwDisChgAhrs;
    uint32_t dvBatCycleCnt;
    uint16_t wBalanceVolt;
    uint16_t wCrc;
} tEERunParam;

/* EEPROM规格参数 */
typedef struct
{
    int16_t wBatVoltOffset;
    int16_t wCapVoltOffset;
    int16_t wBatCurOffset;
    uint16_t wHoldingTime;
    uint8_t byBmsId;
    uint8_t byBatType;
    uint8_t byTempType;
    uint8_t byBatParallel;
    uint8_t byLecuNum;
    uint8_t byPackCell;
    uint8_t byPackTemp;
    uint8_t byProtocolVer;
    uint16_t wBatCapacity;
    uint16_t wCellVolMax;
    uint16_t wCellVolMin;
    uint8_t byCellTempMax;
    uint8_t byCellTempMin;
    uint16_t wDiagFltSet;
    uint16_t wBalanceVolt;
    uint8_t cbBlanceType;
    uint8_t wBalanceDiff;
    uint16_t wPackChgMax;
    uint16_t wPackDischgMax;
    uint16_t wCellVolLowChg;
    uint16_t wCrc;
} tEESpecParam;

/* 电池包控制 */
typedef struct
{
    uint8_t cPrdType[12];
    uint32_t dwPrdVer;
    uint16_t wLimitCurCellLevel[4];
    uint16_t wSingleChgAhrs;
    uint16_t wSingleDisChgAhrs;
    tEERunParam tEERun;
    tEESpecParam tEESpec;
    eDiagCtrlMode eDiagMode;
    uPcsPowerCtrl uPowerCtrl;
    uint8_t bySocInit;
    uint8_t bySocStep;
    uint8_t byRsv01[11];
} tBatPackCtrl;

/* 故障位信息 */
typedef struct
{
    union
    {
        uint8_t byByte;
        struct
        {
            uint8_t bCellTempHigh : 2;
            uint8_t bCellTempLow : 2;
            uint8_t bCellVoltHigh : 2;
            uint8_t bCellVoltLow : 2;
        } tBits;
    } uCell;

    union
    {
        uint8_t byByte;
        struct
        {
            uint8_t bChgCurrMuch : 2;
            uint8_t bChgCurrLowTempMuch : 2;
            uint8_t bDisChgCurrMuch : 2;
            uint8_t bRsv01 : 2;
        } tBits;
    } uCur;

    union
    {
        uint8_t byByte;
        struct
        {
            uint8_t bVoltHigh : 2;
            uint8_t bVoltLow : 2;
            uint8_t bSocLow : 2;
            uint8_t bInsulaLow : 2;
        } tBits;
    } uBat;

    union
    {
        uint8_t byByte;
        struct
        {
            uint8_t bDiffTemp : 2;
            uint8_t bDiffVolt : 2;
            uint8_t bRsv01 : 4;
        } tBits;
    } uDiff;

    union
    {
        uint16_t byByte;
        struct
        {
            uint8_t bAfeComm : 1;
            uint8_t b485Comm : 1;
            uint8_t bCanComm : 1;
            uint8_t bPreChg : 1;
            uint8_t bLcdComm : 1;
            uint8_t bBluComm : 1;
            uint8_t bRamSpi : 1;
            uint8_t bEepromI2c : 1;
        } tBits;
    } uDiag;
} tFaultBitInfo;

/* BLE名称 */
#define MAX_BLE_NAME 20
#define FIX_BLENAME_LEN 8

typedef struct
{
    char g_byBleName[MAX_BLE_NAME];
    uint16_t wCrc;
} tBleName;

#endif /* __USERDEF_H__ */
