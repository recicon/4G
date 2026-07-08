#ifndef __APP_H__
#define __APP_H__
#include"Usart_Com.h"
#include "ModbusPro.h"
//#include "ModbusProEx.h"  
#include"UserDef.h"


#define N_CELL_TEMP_ALARM                       10  
#define N_CELL_VOLT_ALARM                       9  
#define N_BATT_VOLT_ALARM                       6  
#define N_BATT_CURR_ALARM                       6
#define N_BATT_CURR_LOW_TEMP_ALARM              9  
#define N_BATT_SOC_ALARM                        3  
#define N_BATT_INSULATE_ALARM                   3  

#define RT10K_3950                (0)         
#define RT10K_3435                (1)
#define TEMP_TYPE                  (RT10K_3950)




#if  EN_BOOTLOADER


#define FLASH_PAGE_SIZE             64u
#define FLASH_SECTOR_SIZE           0x800

/* BootLoader flash相关宏定义 */
#define BOOT_SIZE                   (0x4000ul)                        //BootLoader flash尺寸
#define UPDATE_SIZE                 (0xC800ul) 
#define UPDATE_PRAM                  (0X1F800)
#define FLASH_SIZE                   (FLASH_PAGE_SIZE * FLASH_SECTOR_SIZE)      

#define BOOT_PARA_ADDRESS           (FLASH_BASE + UPDATE_PRAM + 8U)   //BootLoader para存储地址
#define APP_UPGRADE_FLAG_ADDR       (FLASH_BASE + UPDATE_PRAM + 4U)   //更新标志
#define APP_RUN_ADDR                (FLASH_BASE + UPDATE_PRAM )  //启动地址存储

#define BLE_ADDRESS                  16U
#define BOOT_PARA_ADDRESS_BLE           (FLASH_BASE + UPDATE_PRAM + 8U + BLE_ADDRESS)   //BootLoader para存储地址
#define APP_UPGRADE_FLAG_ADDR_BLE       (FLASH_BASE + UPDATE_PRAM + 4U + BLE_ADDRESS)   //更新标志
#define APP_RUN_ADDR_BLE                (FLASH_BASE + UPDATE_PRAM + BLE_ADDRESS)  //启动地址存储

/* APP flash相关宏定义 */
#define APP_FLAG                    ((uint32_t)0x67890123UL)         //从BootLoader para区读到此值，表示APP需要更新
#define APP_UPGRADE_FLAG            ((uint32_t)0xA5B6C7FFUL)         //更新标志
#define APP_ADDRESS                 (FLASH_BASE + BOOT_SIZE)          //APP程序存储基地址

#endif

typedef struct
{
    uint8_t byUpperVal;
    uint8_t byLowerVal;
}tFaultLevelDataParam;

typedef struct
{
    uint16_t wUpperVal;
    uint16_t wLowerVal;
}tFaultLevel16BitParam;

typedef struct
{
    float fUpperVal;
    float fLowerVal;
}tFaultLevelFPParam;

typedef struct
{
    tFaultLevelDataParam tCellTemp[N_CELL_TEMP_ALARM];
    tFaultLevel16BitParam tCellVolt[N_CELL_VOLT_ALARM];
    tFaultLevel16BitParam tBattVolt[N_BATT_VOLT_ALARM];
    tFaultLevelFPParam    tBattCurr[N_BATT_CURR_ALARM];
    //tFaultLevel16BitParam tBattCurrLowTemp[N_BATT_CURR_LOW_TEMP_ALARM];
    tFaultLevelDataParam tBattSoc[N_BATT_SOC_ALARM];
    //tFaultLevel16BitParam tBattInsulate[N_BATT_INSULATE_ALARM];
    uint16_t wCrc;
}tFaultDataParam;


typedef struct
{
    uint8_t  byContracRequst;
    union
    {
        uint8_t byByte;
        struct
        {
            uint8_t bChgFull:     1;
            uint8_t bSoc:         1;
            uint8_t bRsv01:       5;
            uint8_t bUpdate:      1;
        }tBits;
    }uStatusCtrl;
    uint16_t wCalibrateVal;
    uint8_t  aRsv01[3];
    uint8_t  byCnt;
}tHcuPowerCmd;

typedef struct
{
    uint8_t byHalDhab;  
    union
    {
        uint8_t byByte;
        struct
        {
            uint8_t bTempSensorVoltHi: 1;
            uint8_t bTempSensorVoltLo: 1;
            uint8_t bPcbTempSensorVoltHi:   1;
            uint8_t bPcbTempSensorVoltLo: 1;
            uint8_t bOpen:    1;
            uint8_t bSpi:     1;        
            uint8_t bPcbHi:   2;
        }tBits;
    }uUpload;
    uint32_t dwCan;   // CAN通讯故障,最多支持32块
}tDiagLecuData;

typedef struct
{
      
    union
    {
        uint16_t wWord;
        struct
        {
            uint16_t bNeg:    2;      
            uint16_t bPre:    2;
            uint16_t bPos:    2;
            uint16_t bBm1:    2;
            uint16_t bBm2:    2;
            uint16_t bSec:    2;
            uint16_t bFan1:   2;
            uint16_t bFan2:   2;
        }tBits;
    }uRelaySt;
    tDiagLecuData tLecu;
}tDiagnosticMsg;

typedef struct
{
    uint16_t wPassCode;  
    union 
    {
        uint8_t byBytes;
        struct
        {
            uint8_t bNeg    :1;
            uint8_t bPos    :1;
            uint8_t bPre    :1;
            uint8_t bBm1    :1;
            uint8_t bBm2    :1;            
            uint8_t bBSec   :1;
            uint8_t bSoc    :1;
            uint8_t bSoh    :1;
        }tBits;
    }uRelayCtrl1;
    
    union 
    {
        uint8_t byBytes;
        struct
        {
            uint8_t bFan1    :1;            
            uint8_t bFan2    :1;
            uint8_t bRsv01   :4;
            uint8_t bOffset  :2;
        }tBits;
    }uRelayCtrl2;
    uint16_t wCalibrateVal;                     
    uint8_t  byCalibrateSoH;
    uint8_t  byEquCmd;
}tDignoseParamIn;

typedef enum
{
    FRAME_RECV_IDLE_STATUS       = 0x00,
    FRAME_RECV_HEADER_STATUS     = 0x01,
    FRAME_RECV_DATA_STATUS       = 0x02,
    FRAME_RECV_PROC_STATUS       = 0x03,
} en_frame_recv_status_t;

typedef enum
{
    PACKET_CMD_TYPE_CONTROL     = 0x11,
    PACKET_CMD_TYPE_DATA        = 0x12,
} en_packet_cmd_type_t;

/**
 *******************************************************************************
 ** \brief Packet command enumeration
 ******************************************************************************/
typedef enum
{
    PACKET_CMD_HANDSHAKE       = 0x20,
    PACKET_CMD_JUMP_TO_APP     = 0x21,
    PACKET_CMD_APP_DOWNLOAD    = 0x22,
    PACKET_CMD_APP_UPLOAD      = 0x23,
    PACKET_CMD_ERASE_FLASH     = 0x24,
    PACKET_CMD_CRC_FLASH       = 0x25,
    PACKET_CMD_START_UPDATE    = 0x26,
} en_packet_cmd_t;

/**
 *******************************************************************************
 ** \brief Packet status enumeration
 ******************************************************************************/
typedef enum
{
    PACKET_ACK_OK                = 0x00,
    PACKET_ACK_ERROR             = 0x01,
    PACKET_ACK_ABORT             = 0x02,
    PACKET_ACK_TIMEOUT           = 0x03,
    PACKET_ACK_ADDR_ERROR        = 0x04,
    PACKET_ACK_FLASH_SIZE_ERROR  = 0x05,
} en_packet_status_t;


extern tHcuPowerCmd g_tHcuPowerCmd;
extern tFaultDataParam g_tFaultDataParam;
extern tDignoseParamIn g_tProDignoseCtrl;
extern tBatPackInfo g_tBatPackInfo;
extern tBatPackCtrl g_tBatPackCtrl;
extern tDiagnosticMsg  g_tDiagnosticMsg;
extern tFaultBitInfo g_tFaultBitInfo;
extern tBleName g_tBleName;
extern bool updata_ble;
extern uint64_t g_u64LastCommMs;
extern uint8_t g_byCurDriftCalibStatus;
extern float g_wBatCurDrift;
void CurDriftCalib_Process(void);
void CurDriftCalib_Init(void);

void App_Init(void);  

uint8_t FaultLevelInit(void);
uint8_t FaultLevelOneKey(uint8_t byBatType);
void ExecMainMsgProcess(void);
void ExecBleMsgProcess(void);
void ExecUartMsgProcess(void);
void ExecDiagnoseCmd(void);
void ExecLogBmStRecord(void);
void ExecLogErrStRecord(void);
uint16_t Cal_CRC16(uint8_t * p_data, uint16_t offset, uint32_t size);
#endif
