

/**********************************************************************************************
 *                             Include File
**********************************************************************************************/
#include "MemoryModule.h"
#include "CrcCfg.h"
#include "software_i2c.h"
#include "n32wb43x_it.h"
#include"App.h"
#include <string.h>
/**********************************************************************************************
*							   Macro Define
**********************************************************************************************/
#define N_MEMORY_READ_BUFF          256


#define EI2C_SDA_GPIOx 	GPIOA
#define EI2C_SCL_GPIOx 	GPIOA
#define EI2C_SDA_Pin 	GPIO_PIN_10
#define EI2C_SCL_Pin    GPIO_PIN_9

/**********************************************************************************************
*							   Macro Define
**********************************************************************************************/


/**********************************************************************************************
*							   Variable Define
**********************************************************************************************/
static tag_MutiPageWtireCtrlType sE2PWrCtrl;			//EEPROM 写操作控制
static SI2C_HANDLE SI2C_handle;
/**********************************************************************************************
*                              Function Declaration
**********************************************************************************************/


void AX24_SI2CInit(void)
{
     if(SI2C_Init(&SI2C_handle,EI2C_SDA_GPIOx, EI2C_SCL_GPIOx, EI2C_SDA_Pin, EI2C_SCL_Pin, 5) == false)
	{
		;
	}
}
/**********************************************************************************************
*                              Function Definnition
**********************************************************************************************/
/* BEGIN_FUNCTION_HDR
**********************************************************************************************
  Function:		en_result_t AX24_Master_Send(uint8_t u8SAddr, const uint8_t *pTxBuff,
											 uint8_t u8TxSize)
  Version:      V1.0      
  Description:  I2C 发送数据
  Calls:                
  Called By:           
  Input:        u8SAddr 数据所在存储器起始地址, pTxBuff 发送数据首地址, u32TxSize 发送数据长度
  Output:             
  Return:            
  Others:       1.写数据步骤: 写入设备地址 --> 写寄存器起始地址 --> 写数据
                2.本函数仅支持页内写操作,不支持跨页,故每次写入数据量不大于 8Bytes(每页长度),
				  且未跨页.
**********************************************************************************************
END_FUNCTION_HDR */
en_result_t AX24_Master_Send(uint8_t u8SAddr, const uint8_t *pTxBuff, uint8_t u8TxSize)
{
	en_result_t eRet = Ok;
    eRet = SI2C_WriteReg(&SI2C_handle, AX24C02_ADDR, u8SAddr,true, (uint8_t *)pTxBuff, u8TxSize);
	return eRet;
}

/* BEGIN_FUNCTION_HDR
**********************************************************************************************
  Function:		en_result_t AX24_Master_Receive(uint8_t u8SAddr, uint8_t *pRxBuff,
												uint8_t u8RxSize)
  Version:		V1.0      
  Description:	I2C 接收数据
  Calls:                
  Called By:           
  Input:		u8SAddr 数据所在存储器起始地址, pRxBuff 接收数据首地址, u8RxSize 接收数据长度
  Output:             
  Return:            
  Others:		1.读取数据步骤: 写入设备地址和读取寄存器起始地址 --> 发送重复起始位 
							    --> 发送读取指令 --> 读取数据
				2.本函数读操作支持跨页,按指定地址和长度顺序读取,到达器件末地址自动回到'0'地址
**********************************************************************************************
END_FUNCTION_HDR */
en_result_t AX24_Master_Receive(uint8_t u8SAddr, uint8_t *pRxBuff, uint8_t u8RxSize)
{		
	en_result_t eRet = Ok;
    
	eRet = SI2C_ReadReg(&SI2C_handle, AX24C02_ADDR, u8SAddr,true, pRxBuff, u8RxSize);	
            
	return eRet;
}

/* BEGIN_FUNCTION_HDR
**********************************************************************************************
  Function:			int8_t AX24_Write_MultiPageCtrl(const uint16_t u16SAddr, const uint8_t *pWrData,
													const uint8_t u8WrNum, const bool bIsNewMem)
  Version:          V1.0      
  Description:      AX24 多页数据写控制函数
  Calls:                
  Called By:           
  Input:            u16SAddr	起始地址(某一页的随机地址或页起始地址)
  					*pWRData	待读取数据缓冲区首地址
					u8WrNum     待写的字节数
					bIsNewMem	本次进入是否为新存储任务(=true: 新存储; =false: 之前未完成的存储)
  Output:    		         
  Return:           TRUE    操作成功(但未必完成)
  					FALSE   失败或完成
  Others:           1、若填入的参数不符合要求,则返回 ERROR;
     				2、根据 EEPROM 的特性,每次写操作出现地址达到页尾地址时,将会回到该页的起始地址,
					   故需要进行跨页的处理. 出现跨页时,无论前一页写入长度多少,都需待其写入完成后
					   再进行后一页的操作,这需要几毫秒的时间. 
**********************************************************************************************
END_FUNCTION_HDR */
en_result_t AX24_Master_MultiSend(const uint16_t u16SAddr, const uint8_t *pWrData, const uint8_t u8WrNum ,bool bLoop)
{
    uint8_t j = 0;  
    uint8_t k = 0;   	
	//bool bWrRes = true;									//本次写操作结果(=true: 操作成功, 默认成功)
	uint8_t u8OnceWrNum = 0;							//分页写每次写入长度
	uint8_t u8RemWrNum = u8WrNum;						//分页写每次写入后剩余需写入长度
	uint8_t u8FirstPageRemNum = 0;						//首页剩余空间长度
//	uint16_t u16WrSAdd = u16SAddr;						//分页写各次写入实际起始地址
	uint8_t u8PCnt;
	if(sE2PWrCtrl.eWrSt == EP_IDLE)
    {
        //当前为空闲状态
        if(					//无新存储需求
           (u16SAddr > AX24C02_MAX_ADDR)||(u8WrNum == 0)||	//判断新的需求参数合法性
           ((u16SAddr + u8WrNum) > AX24C02_MAX_ADDR))//写将会越界(超越器件边界)
        {
            return  Error;							//检查入口参数
        } 

        //写存储位置与长度解析			
        u8FirstPageRemNum = PAGE_BYTES - (u16SAddr % PAGE_BYTES);
        
        if(u8WrNum <= u8FirstPageRemNum)			//无需分页写入
        {
            sE2PWrCtrl.u8WrPageNum = 1;
        }
        else										//需分页写入
        {
            if(((u8WrNum - u8FirstPageRemNum) % PAGE_BYTES) == 0)	//出去首页存储,剩余数据所占空间是否为整页
            {
                sE2PWrCtrl.u8WrPageNum = (u8WrNum - u8FirstPageRemNum)/PAGE_BYTES + 1;
            }
            else
            {
                sE2PWrCtrl.u8WrPageNum = (u8WrNum - u8FirstPageRemNum)/PAGE_BYTES + 2;
            }
        }
        
        if(sE2PWrCtrl.u8WrPageNum > AX24_ONCE_WR_MAX_PAGES_NUM)	//超出单次写缓冲区长度
        {
            return  Error;	
        }
        
        //写缓冲区设置
        for(u8PCnt = 0; u8PCnt < sE2PWrCtrl.u8WrPageNum; u8PCnt++)
        {
            sE2PWrCtrl.sPageData[u8PCnt].u8PageSAdd = u16SAddr + (u8WrNum - u8RemWrNum);
            
            if((1 == sE2PWrCtrl.u8WrPageNum)||					//无需跨页写入
               ((u8RemWrNum < PAGE_BYTES) && (0 != u8PCnt)))	//所占存储空间末页写入数判断
            {
                u8OnceWrNum = u8RemWrNum;
            }
            else
            {
                u8OnceWrNum = PAGE_BYTES - (sE2PWrCtrl.sPageData[u8PCnt].u8PageSAdd % PAGE_BYTES);						
            }
            sE2PWrCtrl.sPageData[u8PCnt].u8WrNum = u8OnceWrNum;

            u8RemWrNum -= u8OnceWrNum;				
            
            //填充写缓冲区(仅数据)
            k = sE2PWrCtrl.sPageData[u8PCnt].u8PageSAdd - u16SAddr;		//已处理数据缓冲区位置
            for(j = 0; j < u8OnceWrNum; j++)
            {
                sE2PWrCtrl.sPageData[u8PCnt].u8DatBuf[j] = pWrData[j + k];
            }		
        }
        
        sE2PWrCtrl.u8PagePt = 0;
        sE2PWrCtrl.eWrSt = EP_WR;								//下次开始写入
    }
    if(bLoop)
    {
        if(sE2PWrCtrl.u8PagePt < sE2PWrCtrl.u8WrPageNum)
        {
            AX24_Master_Send(sE2PWrCtrl.sPageData[sE2PWrCtrl.u8PagePt].u8PageSAdd,
                             sE2PWrCtrl.sPageData[sE2PWrCtrl.u8PagePt].u8DatBuf,
                             sE2PWrCtrl.sPageData[sE2PWrCtrl.u8PagePt].u8WrNum);
            
            sE2PWrCtrl.u8PagePt++;					//指向下一待写入页缓冲区
            return OperationInProgress;
        }       
    }
    else
    {
        while(sE2PWrCtrl.u8PagePt < sE2PWrCtrl.u8WrPageNum)		//写入未完成
        {
            AX24_Master_Send(sE2PWrCtrl.sPageData[sE2PWrCtrl.u8PagePt].u8PageSAdd,
                             sE2PWrCtrl.sPageData[sE2PWrCtrl.u8PagePt].u8DatBuf,
                             sE2PWrCtrl.sPageData[sE2PWrCtrl.u8PagePt].u8WrNum);
            
            sE2PWrCtrl.u8PagePt++;					//指向下一待写入页缓冲区
            
           Delay_ms(5);
        }
    }  
   sE2PWrCtrl.eWrSt = EP_IDLE;
   sE2PWrCtrl.u8WrPageNum = 0;
   sE2PWrCtrl.u8PagePt = 0; 
   return Ok;
}

/* BEGIN_FUNCTION_HDR
**********************************************************************************************
  Function:			int8_t AX24_Write_MultiPageCtrl(const uint16_t u16SAddr, const uint8_t *pWrData,
													const uint8_t u8WrNum, const bool bIsNewMem)
  Version:          V1.0      
  Description:      AX24 多页数据写控制函数
  Calls:                
  Called By:           
  Input:            u16SAddr	起始地址(某一页的随机地址或页起始地址)
  					*pWRData	待读取数据缓冲区首地址
					u8WrNum     待写的字节数
					bIsNewMem	本次进入是否为新存储任务(=true: 新存储; =false: 之前未完成的存储)
  Output:    		         
  Return:           TRUE    操作成功(但未必完成)
  					FALSE   失败或完成
  Others:           1、若填入的参数不符合要求,则返回 ERROR;
     				2、根据 EEPROM 的特性,每次写操作出现地址达到页尾地址时,将会回到该页的起始地址,
					   故需要进行跨页的处理. 出现跨页时,无论前一页写入长度多少,都需待其写入完成后
					   再进行后一页的操作,这需要几毫秒的时间. 
**********************************************************************************************
END_FUNCTION_HDR */
bool AX24_Write_MultiPageCtrl(const uint16_t u16SAddr, const uint8_t *pWrData, const uint8_t u8WrNum, const bool bIsNewMem,eEEPROMStType *pstType)
{
    uint8_t j = 0;  
    uint8_t k = 0;   	
	//bool bWrRes = true;									//本次写操作结果(=true: 操作成功, 默认成功)
	uint8_t u8OnceWrNum = 0;							//分页写每次写入长度
	uint8_t u8RemWrNum = u8WrNum;						//分页写每次写入后剩余需写入长度
	uint8_t u8FirstPageRemNum = 0;						//首页剩余空间长度
//	uint16_t u16WrSAdd = u16SAddr;						//分页写各次写入实际起始地址
		uint8_t u8PCnt;
	switch(sE2PWrCtrl.eWrSt)
	{
		case EP_IDLE:									//当前为空闲状态
			if((!bIsNewMem)||							//无新存储需求
			   (u16SAddr > AX24C02_MAX_ADDR)||(u8WrNum == 0)||	//判断新的需求参数合法性
			   ((u16SAddr + u8WrNum) > AX24C02_MAX_ADDR))//写将会越界(超越器件边界)
			{
                *pstType=EP_IDLE;
				return  false;							//检查入口参数
			} 

			//写存储位置与长度解析			
			u8FirstPageRemNum = PAGE_BYTES - (u16SAddr % PAGE_BYTES);
			
			if(u8WrNum <= u8FirstPageRemNum)			//无需分页写入
			{
				sE2PWrCtrl.u8WrPageNum = 1;
			}
			else										//需分页写入
			{
				if(((u8WrNum - u8FirstPageRemNum) % PAGE_BYTES) == 0)	//出去首页存储,剩余数据所占空间是否为整页
				{
					sE2PWrCtrl.u8WrPageNum = (u8WrNum - u8FirstPageRemNum)/PAGE_BYTES + 1;
				}
				else
				{
					sE2PWrCtrl.u8WrPageNum = (u8WrNum - u8FirstPageRemNum)/PAGE_BYTES + 2;
				}
			}
			
			if(sE2PWrCtrl.u8WrPageNum > AX24_ONCE_WR_MAX_PAGES_NUM)	//超出单次写缓冲区长度
			{
                 *pstType=EP_IDLE;
				return  false;	
			}
			
			//写缓冲区设置
			for(u8PCnt = 0; u8PCnt < sE2PWrCtrl.u8WrPageNum; u8PCnt++)
			{
				sE2PWrCtrl.sPageData[u8PCnt].u8PageSAdd = u16SAddr + (u8WrNum - u8RemWrNum);
				
				if((1 == sE2PWrCtrl.u8WrPageNum)||					//无需跨页写入
				   ((u8RemWrNum < PAGE_BYTES) && (0 != u8PCnt)))	//所占存储空间末页写入数判断
				{
					u8OnceWrNum = u8RemWrNum;
				}
				else
				{
					u8OnceWrNum = PAGE_BYTES - (sE2PWrCtrl.sPageData[u8PCnt].u8PageSAdd % PAGE_BYTES);						
				}
				sE2PWrCtrl.sPageData[u8PCnt].u8WrNum = u8OnceWrNum;

				u8RemWrNum -= u8OnceWrNum;				
				
				//填充写缓冲区(仅数据)
				k = sE2PWrCtrl.sPageData[u8PCnt].u8PageSAdd - u16SAddr;		//已处理数据缓冲区位置
				for(j = 0; j < u8OnceWrNum; j++)
				{
					sE2PWrCtrl.sPageData[u8PCnt].u8DatBuf[j] = pWrData[j + k];
				}		
			}
			
			sE2PWrCtrl.u8PagePt = 0;
			sE2PWrCtrl.eWrSt = EP_WR;								//下次开始写入
			break;
		
		case EP_WR:										//写入操作
			if(sE2PWrCtrl.u8PagePt < sE2PWrCtrl.u8WrPageNum)		//写入未完成
			{
				AX24_Master_Send(sE2PWrCtrl.sPageData[sE2PWrCtrl.u8PagePt].u8PageSAdd,
								 sE2PWrCtrl.sPageData[sE2PWrCtrl.u8PagePt].u8DatBuf,
								 sE2PWrCtrl.sPageData[sE2PWrCtrl.u8PagePt].u8WrNum);
				
				sE2PWrCtrl.u8PagePt++;					//指向下一待写入页缓冲区
			}
			else										//写入完成
			{
				sE2PWrCtrl.eWrSt = EP_IDLE;
				sE2PWrCtrl.u8WrPageNum = 0;
				sE2PWrCtrl.u8PagePt = 0;
			}
			break;
		
		default:
			sE2PWrCtrl.eWrSt = EP_IDLE;
			sE2PWrCtrl.u8WrPageNum = 0;
			sE2PWrCtrl.u8PagePt = 0;
			break;		
	}
	*pstType=sE2PWrCtrl.eWrSt;
    return (true);   
}


static uint8_t byEepromMemoryRead(uint32_t dwAddress, uint8_t *ptrData, uint16_t wDataLen)
{
    en_result_t result;
    if(dwAddress+wDataLen > EEPROM_AREA_END)
        return ErrorAddressAlignment;
    else
        result=AX24_Master_Receive(dwAddress, ptrData, wDataLen);
    if( Ok ==result )
       g_tBatPackInfo.tBmDiagVal.tBits.u1EepromI2cFlg =0;// &= ~(1 <<   eDiagEepromI2c);
    else
       g_tBatPackInfo.tBmDiagVal.tBits.u1EepromI2cFlg =1;// |= (1 <<   eDiagEepromI2c);
    return  result;
}

uint8_t byEEMemoryWrite(uint32_t dwAddress, uint8_t *ptrData, uint16_t wDataLen)
{
    uint8_t l_byResult;
    uint8_t l_byReadBuf[N_MEMORY_READ_BUFF];
    uint16_t l_wWriteCrc, l_wReadCrc;

    if (wDataLen < 3)
    {
        return Error;
    }
    l_wWriteCrc = App_Crc16Calculate(ptrData, wDataLen - 2);
    ptrData[wDataLen - 1] = (uint8_t)(l_wWriteCrc >> 8);
    ptrData[wDataLen - 2] = (uint8_t)l_wWriteCrc;

    l_byResult = AX24_Master_MultiSend(dwAddress, ptrData, wDataLen,false);
    //l_byResult = byEepromMemoryWrite(dwAddress   , ptrData, wDataLen);
    if (Ok != l_byResult)
    {
        return Error;
    }

    l_byResult = byEepromMemoryRead(dwAddress   , l_byReadBuf, wDataLen);
    if (Ok != l_byResult)
    {
        return Error;
    }
    l_wReadCrc = App_Crc16Calculate(l_byReadBuf, wDataLen - 2);
    if (l_wReadCrc != (((uint16_t)l_byReadBuf[wDataLen - 1] << 8) | l_byReadBuf[wDataLen - 2]))
    {
        return Error;
    }
    
    return Ok;
}
uint8_t byEEMemoryRead(uint32_t dwAddress, uint8_t *ptrData, uint16_t wDataLen)
{
    uint8_t l_byResult;
    uint8_t l_byReadBuf[N_MEMORY_READ_BUFF];
    uint16_t l_wReadCrc;

    if ( wDataLen < 3)
    {
        return Error;
    }

    l_byResult = byEepromMemoryRead(dwAddress   , l_byReadBuf, wDataLen);
    if (Ok != l_byResult)
    {
        return Error;
    }
    l_wReadCrc = App_Crc16Calculate(l_byReadBuf, wDataLen - 2);
    if (l_wReadCrc != (((uint16_t)l_byReadBuf[wDataLen - 1] << 8) | l_byReadBuf[wDataLen - 2]))
    {
        return Error;
    }
    memcpy(ptrData, l_byReadBuf, wDataLen);
    
    return Ok;
}

/*===========================================================================
 *  SPI Flash Log System
 *===========================================================================*/
#include "spi_flash.h"
#include "TimeModule.h"
#include "CrcCfg.h"

tLogRingCtrl g_tBmsLogRing;
tLogRingCtrl g_tErrLogRing;
uint8_t g_aFlashId[3];   /* 上电自检：SPI Flash JEDEC ID，调试器可观察；全 0/全 FF 表示通信异常 */

static uint8_t LogFlashRead(uint32_t dwAddr, uint8_t *pBuf, uint32_t dwLen)
{
    return SpiFlash_Read(dwAddr, pBuf, dwLen);
}

static uint8_t LogFlashWrite(uint32_t dwAddr, const uint8_t *pBuf, uint32_t dwLen)
{
    return SpiFlash_Write(dwAddr, pBuf, dwLen);
}

/* Scan a log area and recover the write pointer */
static void LogAreaScan(tLogRingCtrl *pRing)
{
    uint32_t dwAddr = pRing->dwBaseAddr;
    uint32_t dwLastValid = pRing->dwBaseAddr;
    uint8_t  byMarker;
    uint8_t  aBuf[LOG_ITEM_SIZE];
    uint16_t wCalcCrc, wStoredCrc;
    uint8_t  bFound = 0;

    while (dwAddr + LOG_ITEM_SIZE <= pRing->dwEndAddr + 1)
    {
        LogFlashRead(dwAddr + 29, &byMarker, 1);
        if (LOG_MARKER_VALID != byMarker)
            break;

        LogFlashRead(dwAddr, aBuf, LOG_ITEM_SIZE);
        wCalcCrc = App_Crc16Calculate(aBuf, LOG_ITEM_SIZE - 2);
        wStoredCrc = ((uint16_t)aBuf[LOG_ITEM_SIZE - 1] << 8) | aBuf[LOG_ITEM_SIZE - 2];
        if (wCalcCrc != wStoredCrc)
            break;

        dwLastValid = dwAddr;
        bFound = 1;
        dwAddr += LOG_ITEM_SIZE;
    }

    if (bFound)
    {
        pRing->dwCurWrAddr = dwLastValid + LOG_ITEM_SIZE;
        pRing->dwWrCount = (uint16_t)((pRing->dwCurWrAddr - pRing->dwBaseAddr) / LOG_ITEM_SIZE);
    }
    else
    {
        pRing->dwCurWrAddr = pRing->dwBaseAddr;
        pRing->dwWrCount = 0;
    }
    pRing->dwCurRdAddr = pRing->dwCurWrAddr;
}

/* Erase the first sector of a log area at wrap-around */
static void LogEraseFirstSector(tLogRingCtrl *pRing)
{
    uint32_t dwSectorAddr = pRing->dwBaseAddr;
    SpiFlash_EraseSector(dwSectorAddr);
}

void Flash_MemoryInit(void)
{
    SpiFlash_Init();
    SpiFlash_ReadID(g_aFlashId);   /* 自检 SPI 通信：ZD25Q16 应返回有效 ID，而非全 0/全 FF */

    g_tErrLogRing.dwBaseAddr = FLASH_ERR_LOG_START;
    g_tErrLogRing.dwEndAddr  = FLASH_ERR_LOG_END;
    g_tErrLogRing.wMaxCount  = FLASH_ERR_LOG_COUNT;

    g_tBmsLogRing.dwBaseAddr = FLASH_BMS_LOG_START;
    g_tBmsLogRing.dwEndAddr  = FLASH_BMS_LOG_END;
    g_tBmsLogRing.wMaxCount  = FLASH_BMS_LOG_COUNT;

    LogAreaScan(&g_tErrLogRing);
    LogAreaScan(&g_tBmsLogRing);
}

static uint8_t LogItemWrite(tLogRingCtrl *pRing, const uint8_t *pData, uint16_t wSize)
{
    uint8_t aBuf[LOG_ITEM_SIZE + 2];
    uint16_t wCrc;

    if (wSize != LOG_ITEM_SIZE - 2)
        return 0;

    /* Build item: copy data, set marker, compute CRC over data+marker, append CRC */
    memcpy(aBuf, pData, LOG_ITEM_SIZE - 2);
    aBuf[(LOG_ITEM_SIZE - 2) - 1] = LOG_MARKER_VALID;  /* marker just before CRC */
    wCrc = App_Crc16Calculate(aBuf, LOG_ITEM_SIZE - 2);
    aBuf[LOG_ITEM_SIZE - 2] = (uint8_t)wCrc;
    aBuf[LOG_ITEM_SIZE - 1] = (uint8_t)(wCrc >> 8);

    /* Check wrap: if at end of area, erase first sector */
    if (pRing->dwCurWrAddr + LOG_ITEM_SIZE > pRing->dwEndAddr + 1)
    {
        LogEraseFirstSector(pRing);
        pRing->dwCurWrAddr = pRing->dwBaseAddr;
    }

    /* Erase sector before writing if at sector boundary */
    if ((pRing->dwCurWrAddr & (SECTOR_SIZE - 1)) == 0)
        SpiFlash_EraseSector(pRing->dwCurWrAddr);

    LogFlashWrite(pRing->dwCurWrAddr, aBuf, LOG_ITEM_SIZE);

    pRing->dwCurWrAddr += LOG_ITEM_SIZE;
    pRing->dwWrCount++;
    return 1;
}

uint8_t LOG_BmsStItemWrite(tLogBmStItem *pItem)
{
    pItem->wCrc = 0;
    return LogItemWrite(&g_tBmsLogRing, (const uint8_t *)pItem, sizeof(tLogBmStItem) - 2);
}

uint32_t LOG_BmsStItemCount(void)
{
    return g_tBmsLogRing.dwWrCount;
}

static uint8_t LogReadByIndex(tLogRingCtrl *pRing, uint16_t wMaxCount, uint16_t wIdx, uint8_t *pItem)
{
    uint32_t dwAddr;
    uint32_t wTotal = pRing->dwWrCount;   /* 全精度，避免 uint16 截断累计写入数 */

    if (wIdx >= wTotal) return 0;

    if (wTotal <= wMaxCount)
        dwAddr = pRing->dwBaseAddr + wIdx * LOG_ITEM_SIZE;
    else
        dwAddr = pRing->dwBaseAddr + ((pRing->dwWrCount + wIdx) % wMaxCount) * LOG_ITEM_SIZE;

    LogFlashRead(dwAddr, pItem, LOG_ITEM_SIZE);
    return (pItem[29] == LOG_MARKER_VALID) ? 1 : 0;
}

uint8_t LOG_BmsStItemReadByIndex(uint16_t wIdx, tLogBmStItem *pItem)
    { return LogReadByIndex(&g_tBmsLogRing, FLASH_BMS_LOG_COUNT, wIdx, (uint8_t *)pItem); }

uint8_t LOG_ErrStItemWrite(tLogErrStItem *pItem)
{
    pItem->wCrc16 = 0;
    return LogItemWrite(&g_tErrLogRing, (const uint8_t *)pItem, sizeof(tLogErrStItem) - 2);
}

uint32_t LOG_ErrStItemCount(void)
    { return g_tErrLogRing.dwWrCount; }

uint8_t LOG_ErrStItemReadByIndex(uint16_t wIdx, tLogErrStItem *pItem)
    { return LogReadByIndex(&g_tErrLogRing, FLASH_ERR_LOG_COUNT, wIdx, (uint8_t *)pItem); }

/**********************************************************************************************
 * EOF
**********************************************************************************************/

