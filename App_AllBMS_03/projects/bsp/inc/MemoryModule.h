
#ifndef __MEMORYMODULE_H__
#define __MEMORYMODULE_H__
#include "software_i2c.h"
#include "Soc.h"

/**********************************************************************************************
 *                             Include File
**********************************************************************************************/

#define AX24C02_ADDR	(0xA0)					//器件代码为 1010000x,右移1bit 为 0101 0000(0x50)

#define AX24C02_MAX_ADDR ((uint8_t)0xFF)		//EEPROM 最大地址
#define PAGE_BYTES  	((uint8_t)8)			//每页字节数 (每页为 8Bytes)
#define PAGE_SIZE   	((uint8_t)32)			//页总数
#define PAGE_ONE    	((uint8_t)0x00)			//第 1 页首地址
#define PAGE_TWO    	((uint8_t)0x08)			//第 2 页首地址
#define PAGE_THREE  	((uint8_t)0x10)			//第 3 页首地址
#define PAGE_FOUR   	((uint8_t)0x18)			//第 4 页首地址
#define PAGE_FIVE   	((uint8_t)0x20)			//第 5 页首地址
#define PAGE_SIX    	((uint8_t)0x28)			//第 6 页首地址
#define PAGE_SEVEN  	((uint8_t)0x30)			//第 7 页首地址

//====== 操作方式有关定义 ======//
#define AX24_ONCE_WR_MAX_PAGES_NUM  	32u		//单次操作最大页数(所跨页数)


/**********************************************************************************************
*							   Type Define
**********************************************************************************************/
//EEPROM 页数据结构类型
typedef enum
{
	EP_IDLE = 0,								//空闲/待机
	EP_SET = 1,									//设置/配置状态
	EP_WR = 2,									//写操作进行中
} eEEPROMStType; 

//EEPROM 页数据结构类型
typedef struct
{
    uint8_t  u8DatBuf[PAGE_BYTES];				//当前页写数据缓冲区
	uint8_t  u8PageSAdd;						//当前页写入起始地址	
	uint8_t  u8WrNum;							//当前页写入数据长度/数目	
}tag_PageDataBuffType; 

//EEPROM 多页写控制结构类型
typedef struct
{
    tag_PageDataBuffType sPageData[AX24_ONCE_WR_MAX_PAGES_NUM];
	uint8_t u8PagePt;							//页指针
	uint8_t u8WrPageNum;						//需写入数据所占页数
	
	eEEPROMStType eWrSt;                 		//写入操作状态	
}tag_MutiPageWtireCtrlType; 

/**********************************************************************************************
*							   Macro Define
**********************************************************************************************/
typedef struct
{
    int8_t  byVal;
    int16_t wCrc;
}tEE8BitParam;

typedef struct
{
    int16_t wVal;
    int16_t wCrc;
}tEE16BitParam;

typedef struct
{
    int32_t dwVal;
    int16_t wCrc;
}tEE32BitParam;

typedef struct
{
    float   fVal;
    int16_t wCrc;
}tEEFP32Param;



#define EEPROM_AREA_START               0x020UL
#define EEPROM_AREA_END                 0x0FFUL

// 故障阈值等级参数存储区域
#define EE_FAULT_LEVEL_PARAM_LEN                (sizeof(tFaultDataParam) / sizeof (uint8_t))      // 故障等级阈值参数长度
#define EE_FAULT_LEVEL_PARAM_START_ADDR         (EEPROM_AREA_START)
#define EE_FAULT_LEVEL_PARAM_END_ADDR           (EE_FAULT_LEVEL_PARAM_START_ADDR + EE_FAULT_LEVEL_PARAM_LEN)

//// 当前时间存储区域
//#define EE_TIME_LEN                 (sizeof(tEETimeParam) / sizeof (uint8_t))
//#define EE_CUR_TIME_ADDR            (EE_FAULT_LEVEL_PARAM_END_ADDR + 0) 
//#define EE_CUR_TIME_END_ADDR        (EE_FAULT_LEVEL_PARAM_END_ADDR + EE_TIME_LEN) 

//// SOC存储区域
//#define EE_SOC_LEN                  (sizeof(tEESocParam) / sizeof (uint8_t))
//#define EE_SOC_ADDR                 (EE_CUR_TIME_END_ADDR + 0)       
//#define EE_SOC_END_ADDR             (EE_CUR_TIME_END_ADDR + EE_SOC_LEN)       

// 电池一些的运行参数存储区域
#define EE_RUN_LEN                  (sizeof(tEERunParam) / sizeof (uint8_t))
#define EE_RUN_ADDR                 (EE_FAULT_LEVEL_PARAM_END_ADDR + 0)       
#define EE_RUN_END_ADDR             (EE_FAULT_LEVEL_PARAM_END_ADDR + EE_RUN_LEN)   

// CECU 特殊参数存储区域
#define EE_SPECIAL_LEN              (sizeof(tEESpecParam) / sizeof (uint8_t))
#define EE_SPECIAL_ADDR             (EE_RUN_END_ADDR + 0)       
#define EE_SPECIAL_END_ADDR         (EE_RUN_END_ADDR + EE_SPECIAL_LEN)    

 //电池SOP
 #define EE_SOV_LEN                  (sizeof(tOcvSocTable) / sizeof (uint8_t))
 #define EE_SOV_ADDR                 (EE_SPECIAL_END_ADDR + 0)       
 #define EE_SOV_END_ADDR             (EE_SPECIAL_END_ADDR + EE_SOV_LEN)     
 //温度表
 #define EE_PT100_LEN                (sizeof(tPT100RTable) / sizeof (uint8_t))
 #define EE_PT100_ADDR               (EE_SOV_END_ADDR + 0)       
 #define EE_PT100_END_ADDR           (EE_SOV_END_ADDR + EE_SOV_LEN)     

 //蓝牙名称
 #define EE_BLENAME_LEN                (sizeof(tBleName) / sizeof (uint8_t))
 #define EE_BLENAME_ADDR               (EE_PT100_END_ADDR + 0)       
 #define EE_BLENAME_END_ADDR           (EE_BLENAME_ADDR + EE_BLENAME_LEN)  
/**********************************************************************************************
*							   		Type Define
**********************************************************************************************/

///**********************************************************************************************
//*							   Variable Declaration
//**********************************************************************************************/

///**********************************************************************************************
//*                              Function Declaration
//**********************************************************************************************/

void AX24_SI2CInit(void);

extern uint8_t byEEMemoryWrite(uint32_t dwAddress, uint8_t *ptrData, uint16_t wDataLen);
extern uint8_t byEEMemoryRead(uint32_t dwAddress, uint8_t *ptrData, uint16_t wDataLen);

/*===========================================================================
 *  SPI Flash Log System (ZD25Q16, 2MB)
 *===========================================================================*/

/* Flash storage layout */
#define FLASH_ERR_LOG_START      0x000000UL   /* Alarm log: 64KB, 2048 x 32B */
#define FLASH_ERR_LOG_END        0x00FFFFUL
#define FLASH_ERR_LOG_COUNT      2048

#define FLASH_BMS_LOG_START      0x010000UL   /* BMS history: 1984KB, 63488 x 32B */
#define FLASH_BMS_LOG_END        0x1FFFFFUL
#define FLASH_BMS_LOG_COUNT      63488

#define LOG_ITEM_SIZE            32
#define LOG_MARKER_VALID         0xAA
#define LOG_MARKER_EMPTY         0xFF

/* BMS history record (32 bytes) —— 字段与 App_BleBMS_01 的 tLogBmStItem 对齐 */
typedef struct __attribute__((packed))
{
    uint32_t dwUtcTime;           /* UTC 时间戳            (0-3)   */
    uint16_t wBatVolt;            /* 电池电压 0.1V         (4-5)   */
    uint16_t wCapVolt;            /* 端口电压 0.1V         (6-7)   */
    int16_t  wBatCur;             /* 电池电流 0.1A (有符号) (8-9)   */
    uint16_t wSoc;                /* SOC 0.01%             (10-11) */
    uint16_t wRelayState;         /* 继电器状态字          (12-13) */
    uint16_t wCellVoltMax;        /* 最高单体电压 mV       (14-15) */
    uint16_t wCellVoltMin;        /* 最低单体电压 mV       (16-17) */
    uint8_t  byBmState;           /* 控制状态 eCtrlMode    (18)    */
    uint8_t  byBmFltLevl;         /* 故障等级 eFltLevel    (19)    */
    uint8_t  byCellTempMax;       /* 最高温度 (offset 40)  (20)    */
    uint8_t  byCellTempMin;       /* 最低温度 (offset 40)  (21)    */
    uint8_t  byRsv01[7];          /* 保留                  (22-28) */
    uint8_t  byMark;              /* 0xAA = valid          (29)    */
    uint16_t wCrc;                /* CRC16 over bytes 0-29 (30-31) */
} tLogBmStItem;

/* Alarm/error record (32 bytes) */
typedef struct __attribute__((packed))
{
    uint32_t dwUtcTime;           /* UTC 时间戳 (与 tLogBmStItem / App_BleBMS_01 一致) */
    uint16_t wFaultBits;          /* Fault bit flags (tBmFltVal.wByte) */
    uint16_t wFaultVal;           /* Fault value word */
    uint16_t wDiagVal;            /* Diagnostic value word */
    uint8_t  byFaultLevel;        /* Fault level (0-3) */
    uint8_t  byCellVoltMaxId;     /* Max cell ID at fault */
    uint16_t wCellVoltMax;        /* Max cell voltage at fault (mV) */
    uint8_t  byCellVoltMinId;     /* Min cell ID at fault */
    uint16_t wCellVoltMin;        /* Min cell voltage at fault (mV) */
    uint8_t  byTempMax;           /* Max temp at fault */
    uint8_t  byTempMin;           /* Min temp at fault */
    uint16_t wBatVolt;            /* Battery voltage at fault */
    int16_t  wBatCur;             /* Battery current at fault */
    uint8_t  byReserved[6];       /* Padding to 30B before CRC */
    uint8_t  byMarker;            /* 0xAA = valid */
    uint16_t wCrc16;              /* CRC16 over bytes 0-29 */
} tLogErrStItem;

/* Log ring buffer control */
typedef struct
{
    uint32_t dwBaseAddr;          /* Flash start address */
    uint32_t dwEndAddr;           /* Flash end address */
    uint32_t dwCurWrAddr;         /* Current write address */
    uint32_t dwCurRdAddr;         /* Current read address */
    uint16_t wMaxCount;           /* Max entries in this ring */
    uint32_t dwWrCount;           /* Total written count */
} tLogRingCtrl;

/* Log API */
void     Flash_MemoryInit(void);
uint8_t  LOG_BmsStItemWrite(tLogBmStItem *pItem);
uint32_t LOG_BmsStItemCount(void);
uint8_t  LOG_BmsStItemReadByIndex(uint16_t wIdx, tLogBmStItem *pItem);
uint8_t  LOG_ErrStItemWrite(tLogErrStItem *pItem);
uint32_t LOG_ErrStItemCount(void);
uint8_t  LOG_ErrStItemReadByIndex(uint16_t wIdx, tLogErrStItem *pItem);

extern tLogRingCtrl g_tBmsLogRing;
extern tLogRingCtrl g_tErrLogRing;
extern uint8_t g_aFlashId[3];

/**********************************************************************************************
 * EOF
**********************************************************************************************/
#endif
