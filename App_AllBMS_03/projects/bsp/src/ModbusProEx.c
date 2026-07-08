/**********************************************************************************************
 *                             Include File
**********************************************************************************************/
#include "ModbusProEx.h"
#include "CrcCfg.h"  
#include "MemoryModule.h" 
#include <string.h>
#include"UserDef.h"
#include"App.h"
#include"port.h"


/**********************************************************************************************
*							   Macro Define
**********************************************************************************************/

#define ADDR_MODBUS_START        1
#define ADDR_MODBUS_END          12
#define ADDR_MODBUS_BAMS         0x1

#define T_R                      0             
#define T_W                      1             

#define ADDR_REGISTER_OFFSET           0x1000u
#define ADDR_WRITE_REGISTER_OFFSET     0x2000u

#define CMD_READ_COIL_STATUS     0x01
#define CMD_READ_INPUT_STATUS    0x02
#define CMD_READ_HOLDING_REGS    0x03
#define CMD_READ_INPUT_REGS      0x04
#define CMD_WRITE_SINGLE_COIL    0x05
#define CMD_WRITE_SINGLE_REG     0x06
#define CMD_WRITE_COILS          0x0F
#define CMD_WRITE_REGS           0x10

#define E_MODBUS_FUNC_CODE       0x01
#define E_MODBUS_ILLEGAL_ADDR    0x02
#define E_MODBUS_ILLEGAL_DATA    0x03
#define E_MODBUS_EXEC_CMD        0x04
#define E_MODBUS_SLAVE_ADDR      0x05

#define N_READ_DATA_LEN         (127 * 2)


/*extern uint8_t c_szBamsPermissionRW[31];

const uint8_t c_szBamsPermissionRW[31] = {
     T_W, T_R, T_R, T_R, T_R, T_R, T_R, T_R, T_R, T_R,
     T_R, T_R, T_R, T_R, T_R, T_R, T_R, T_R, T_R, T_R,     
     T_R, T_R, T_R, T_R, T_R, T_R, T_R, T_R, T_R, T_R,
     T_R,
};
*/
#define  BMS_ADDRESS			 1
#define E_MODBUS_MAX_READ      (60)
#define E_MODBUS_MAX_WRITE     (60)
/**********************************************************************************************
*							   		Type Define
**********************************************************************************************/
#define ADDR_MODBUS_START        1
#define ADDR_MODBUS_END          12

#define ADDR_REGISTER_OFFSET           0x1000u
#define ADDR_WRITE_REGISTER_OFFSET     0x2000u

#define CMD_READ_COIL_STATUS     0x01
#define CMD_READ_INPUT_STATUS    0x02
#define CMD_READ_HOLDING_REGS    0x03
#define CMD_READ_INPUT_REGS      0x04
#define CMD_WRITE_SINGLE_COIL    0x05
#define CMD_WRITE_SINGLE_REG     0x06
#define CMD_WRITE_COILS          0x0F
#define CMD_WRITE_REGS           0x10

#define E_MODBUS_FUNC_CODE       0x01
#define E_MODBUS_ILLEGAL_ADDR    0x02
#define E_MODBUS_ILLEGAL_DATA    0x03
#define E_MODBUS_EXEC_CMD        0x04
#define E_MODBUS_SLAVE_ADDR      0x05

#define N_READ_DATA_LEN         (127 * 2)

#define N_REG_HOLDING_SIZE       ((sizeof(tBatPackInfo)+sizeof(tBatPackCtrl)+sizeof(tFaultBitInfo)+sizeof(tDiagnosticMsg)+1)/2)
#define REG_HOLDING_START        0u
#define REG_HOLDING_END          (REG_HOLDING_START + N_REG_HOLDING_SIZE)    

#define N_WRITE_HOLDING_SIZE     ((sizeof(tHcuPowerCmd)+sizeof(tDignoseParamIn)+sizeof(tFaultDataParam)+sizeof(tEERunParam)+sizeof(tEESpecParam)+1)/2)
#define WRITE_HOLDING_START      0u
#define WRITE_HOLDING_END        (WRITE_HOLDING_START + N_WRITE_HOLDING_SIZE)
///**********************************************************************************************
//*							   Variable Declaration
//**********************************************************************************************/

///**********************************************************************************************
//*                              Function Declaration
//**********************************************************************************************/
static en_result_t  SCI_GeneralSendParamOut(sUARTMsgType* sRxMsg,uint8_t *ptrData, uint16_t wDataLen)
{


    uint8_t l_bySendBuf[2];
    uint16_t l_wCrc;
    
    if(wDataLen > N_READ_DATA_LEN -2 )
         return Error;
    l_wCrc = App_Crc16Calculate(ptrData, wDataLen);
    l_bySendBuf[0] = (uint8_t)(l_wCrc);    
    l_bySendBuf[1] = (uint8_t)(l_wCrc >> 8);
    memcpy(sRxMsg->wbuf,ptrData,wDataLen);
    memcpy(&sRxMsg->wbuf[wDataLen],l_bySendBuf,2);
    sRxMsg->w_num =  wDataLen+2;   
    return Ok;
    
}

static en_result_t SUNSPEC_ErrFuncSend(sUARTMsgType* sRxMsg,uint8_t bySlaveAddr, uint8_t byFuncCode, uint8_t byErrCode)
{
   uint8_t l_bySendBuf[4];
    uint16_t  wLen;

    wLen = 0;
    l_bySendBuf[wLen++] = bySlaveAddr;
    l_bySendBuf[wLen++] = byFuncCode;
    l_bySendBuf[wLen++] = byErrCode;

    return SCI_GeneralSendParamOut(sRxMsg,l_bySendBuf,wLen) ;        
}

static en_result_t SUNSPEC_ReadHoldingRegister(sUARTMsgType* sRxMsg,uint8_t bySlaveAddr, uint16_t wAddr, uint16_t wReadLen)
{
 
    uint16_t wSendLen,wSendAddr;    
    uint16_t i;
    uint8_t l_bySendBuf[N_READ_DATA_LEN + 3];
    
   if(wAddr < ADDR_WRITE_REGISTER_OFFSET)
    {
        if (wAddr < ADDR_REGISTER_OFFSET)
        {
            return  ErrorAddressAlignment;
        }
        wAddr -= ADDR_REGISTER_OFFSET; 

        if ((wAddr + wReadLen) > REG_HOLDING_END)
        {
            return  ErrorAddressAlignment;
        }
        if (wReadLen > E_MODBUS_MAX_READ)
        {
            return ErrorInvalidParameter;
        }
         wSendLen = 0;
        l_bySendBuf[wSendLen++] = bySlaveAddr;   
        l_bySendBuf[wSendLen++] = CMD_READ_HOLDING_REGS;    
        l_bySendBuf[wSendLen++] = wReadLen << 1;  
       
        wAddr -= REG_HOLDING_START;
        for(i=0;i< (wReadLen<<1);i++)
        {
            wSendAddr = wAddr * 2 + i;
 
            if(wSendAddr < sizeof(tBatPackInfo))
                l_bySendBuf[wSendLen++] = *((uint8_t *)&g_tBatPackInfo + wSendAddr);
            else
            {
                wSendAddr -= sizeof(tBatPackInfo);
                if(wSendAddr < sizeof(tBatPackCtrl))
                    l_bySendBuf[wSendLen++] = *((uint8_t *)&g_tBatPackCtrl + wSendAddr);
                else
                {
                   wSendAddr -= sizeof(tBatPackCtrl); 
                   if(wSendAddr < sizeof(tFaultBitInfo))
                        l_bySendBuf[wSendLen++] = *((uint8_t *)&g_tFaultBitInfo + wSendAddr);
                   else
                   {
                        wSendAddr -= sizeof(tFaultBitInfo); 
                        if(wSendAddr < sizeof(tDiagnosticMsg))
                            l_bySendBuf[wSendLen++] = *((uint8_t *)&g_tDiagnosticMsg + wSendAddr);
                   }
                }
            }
            
        }
    }
    else
    {
        wAddr -= ADDR_WRITE_REGISTER_OFFSET; 

        if ((wAddr + wReadLen) > WRITE_HOLDING_END)
        {
            return  ErrorAddressAlignment;
        }
        if (wReadLen > E_MODBUS_MAX_READ)
        {
            return ErrorInvalidParameter;
        }
          wSendLen = 0;
        l_bySendBuf[wSendLen++] = bySlaveAddr;   
        l_bySendBuf[wSendLen++] = CMD_READ_HOLDING_REGS;    
        l_bySendBuf[wSendLen++] = wReadLen << 1;  
       
        wAddr -= WRITE_HOLDING_START;
        for(i=0;i< (wReadLen<<1);i++)
        {
            wSendAddr = wAddr * 2 + i;          
            if(wSendAddr < sizeof(tHcuPowerCmd))
                l_bySendBuf[wSendLen++] = *((uint8_t *)&g_tHcuPowerCmd + wSendAddr);
             else
            {
                 wSendAddr -= sizeof(tHcuPowerCmd); 
                if(wSendAddr < sizeof(tDignoseParamIn))
                    l_bySendBuf[wSendLen++] = *((uint8_t *)&g_tProDignoseCtrl + wSendAddr);                                    
                else
                {    
                    wSendAddr -= sizeof(tDignoseParamIn);
                    if(wSendAddr < sizeof(tFaultDataParam))
                        l_bySendBuf[wSendLen++] = *((uint8_t *)&g_tFaultDataParam + wSendAddr);  
                    else
                    {
                        wSendAddr -= sizeof(tFaultDataParam);
                        if(wSendAddr < sizeof(tEERunParam))
                            l_bySendBuf[wSendLen++] = *((uint8_t *)&g_tBatPackCtrl.tEERun + wSendAddr);
                        else
                        {
                           wSendAddr -= sizeof(tEERunParam); 
                           if(wSendAddr < sizeof(tEESpecParam))
                                l_bySendBuf[wSendLen++] = *((uint8_t *)&g_tBatPackCtrl.tEESpec + wSendAddr);
                        //    else
                        //    {
                        //         wSendAddr -= sizeof(tEESpecParam); 
                        //         if(wSendAddr < sizeof(tPT100RTable))
                        //             l_bySendBuf[wSendLen++] = *((uint8_t *)&g_tPT100RTable + wSendAddr);
                        //         else
                        //        {
                        //             wSendAddr -= sizeof(tPT100RTable); 
                        //             if(wSendAddr < sizeof(tOcvSocTable))
                        //                 l_bySendBuf[wSendLen++] = *((uint8_t *)&g_tOcvSocTable + wSendAddr);                               
                        //        }
                        //    }
                        }
                    } 
                }   
            }                
        } 
    }
    return SCI_GeneralSendParamOut(sRxMsg,l_bySendBuf,wSendLen);     
}
static en_result_t SUNSPEC_WriteHoldingRegisterSingle(sUARTMsgType* sRxMsg,uint8_t bySlaveAddr, uint16_t wAddr, uint16_t wData)
{
    uint8_t l_bySendBuf[6],l_byResult = Error;
    uint16_t wSendLen,wWriteAddr;

    if (wAddr < ADDR_WRITE_REGISTER_OFFSET)
    { 
        return  ErrorAddressAlignment;
    }
    wAddr -= ADDR_WRITE_REGISTER_OFFSET; 
    
    if ((wAddr + 0) > REG_HOLDING_END)
    {
        return ErrorAddressAlignment;
    }

    wSendLen = 0;
    l_bySendBuf[wSendLen++] = bySlaveAddr;   
    l_bySendBuf[wSendLen++] = CMD_WRITE_SINGLE_REG;    
    l_bySendBuf[wSendLen++] = (uint8_t)((wAddr  + ADDR_REGISTER_OFFSET) >> 8);
    l_bySendBuf[wSendLen++] = (uint8_t)(wAddr  + ADDR_REGISTER_OFFSET);


    wWriteAddr = wAddr*2;
    if(wWriteAddr<sizeof(tHcuPowerCmd)) 
    {
       memcpy(((uint8_t *)&g_tHcuPowerCmd) + wWriteAddr, &wData, 2);
       wData = *((uint16_t *)&g_tHcuPowerCmd + (wWriteAddr>>1));
       if(wWriteAddr + 2 == sizeof(g_tHcuPowerCmd))
           ;
    }
    else
    {
        wWriteAddr -= sizeof(tHcuPowerCmd);
        if(wWriteAddr < sizeof(tDignoseParamIn)) 
        {
            memcpy(((uint8_t *)&g_tProDignoseCtrl) + wWriteAddr, &wData, 2);
            wData = *((uint16_t *)&g_tProDignoseCtrl + (wWriteAddr>>1));
            if(wWriteAddr + 2 == sizeof(tDignoseParamIn))
                ExecDiagnoseCmd();
        }                            
        else
        {
            wWriteAddr -= sizeof(tDignoseParamIn);
            if(wWriteAddr<sizeof(tFaultDataParam))
            {
                memcpy(((uint8_t *)&g_tFaultDataParam) + wWriteAddr, &wData, 2);
                
                wData = *((uint16_t *)&g_tFaultDataParam + (wWriteAddr>>1));
                if(wWriteAddr + 2 == sizeof(tFaultDataParam))
                    l_byResult = byEEMemoryWrite(EE_FAULT_LEVEL_PARAM_START_ADDR, (uint8_t *)&g_tFaultDataParam, sizeof(tFaultDataParam) / sizeof(uint8_t));
            }
            else
            {
                wWriteAddr -= sizeof(tFaultDataParam);
                if(wWriteAddr<sizeof(tEERunParam)) 
                {
                   memcpy(((uint8_t *)&g_tBatPackCtrl.tEERun) + wWriteAddr, &wData, 2);
                   wData = *((uint16_t *)&g_tBatPackCtrl.tEERun + (wWriteAddr>>1));
                   if(wWriteAddr + 2 == sizeof(tEERunParam))
                        l_byResult = byEEMemoryWrite(EE_RUN_ADDR, (uint8_t *)&g_tBatPackCtrl.tEERun, sizeof(tEERunParam) / sizeof(uint8_t));
                }
                else
                {
                     wWriteAddr -= sizeof(tEERunParam);
                    if(wWriteAddr < sizeof(tEESpecParam)) 
                    {
                         memcpy(((uint8_t *)&g_tBatPackCtrl.tEESpec) + wWriteAddr, &wData, 2);
                         wData = *((uint16_t *)&g_tBatPackCtrl.tEESpec + (wWriteAddr>>1)) = wData;
                        if(wWriteAddr + 2 == sizeof(tEESpecParam))
                           l_byResult = byEEMemoryWrite(EE_SPECIAL_ADDR, (uint8_t *)&g_tBatPackCtrl.tEESpec, sizeof(tEESpecParam) / sizeof(uint8_t));
                    }
                  
                }
            }
        }
    }
    l_bySendBuf[wSendLen++] = (uint8_t)(wData >> 8);
    l_bySendBuf[wSendLen++] = (uint8_t)(wData);

    
   if(Ok== SCI_GeneralSendParamOut(sRxMsg,l_bySendBuf,wSendLen)&&Ok ==l_byResult)
         NVIC_SystemReset();  //软件复位MCU
   return Ok;
}

static en_result_t  SUNSPEC_WriteHoldingRegisterMulti(sUARTMsgType* sRxMsg,uint8_t bySlaveAddr, uint16_t wAddr, uint16_t wWriteLen, uint8_t *ptrData)
{
    uint8_t l_bySendBuf[N_READ_DATA_LEN],l_byResult = Error;
    uint16_t wSendLen,wWriteAddr;
    

    if (wAddr < ADDR_WRITE_REGISTER_OFFSET)
    {
        return  ErrorAddressAlignment;
    }
    wAddr -= ADDR_WRITE_REGISTER_OFFSET; 
    if ((wAddr + wWriteLen) > REG_HOLDING_END)
    {
        return ErrorAddressAlignment;
    }
    if (wWriteLen > E_MODBUS_MAX_WRITE)
    {
        return ErrorInvalidParameter;
    }     
    
    wWriteAddr = wAddr*2;  
    if(wWriteAddr<sizeof(tHcuPowerCmd)) 
    {
       memcpy(((uint8_t *)&g_tHcuPowerCmd) + wWriteAddr, ptrData, wWriteLen << 1);       
       g_tHcuPowerCmd.uStatusCtrl.tBits.bUpdate = 1;
    }
    else
    {
        wWriteAddr -= sizeof(tHcuPowerCmd);
        if(wWriteAddr < sizeof(tDignoseParamIn)) 
        {
            memcpy(((uint8_t *)&g_tProDignoseCtrl) + wWriteAddr, ptrData, wWriteLen << 1);
            if(wWriteAddr + (wWriteLen << 1) == sizeof(tDignoseParamIn))
                ExecDiagnoseCmd();
        } 
        else
        {
            wWriteAddr -= sizeof(tDignoseParamIn);
            if(wWriteAddr<sizeof(tFaultDataParam))
            {
                memcpy(((uint8_t *)&g_tFaultDataParam) + wWriteAddr,ptrData, wWriteLen << 1);      
                if(wWriteAddr + (wWriteLen << 1) == sizeof(tFaultDataParam))
                    l_byResult = byEEMemoryWrite(EE_FAULT_LEVEL_PARAM_START_ADDR, (uint8_t *)&g_tFaultDataParam, sizeof(tFaultDataParam) / sizeof(uint8_t));
            }
            else
            {
                wWriteAddr -= sizeof(tFaultDataParam);
                if(wWriteAddr<sizeof(tEERunParam)) 
                {
                   memcpy(((uint8_t *)&g_tBatPackCtrl.tEERun) + wWriteAddr,ptrData, wWriteLen << 1);
                   if(wWriteAddr + (wWriteLen << 1) == sizeof(tEERunParam))
                        l_byResult = byEEMemoryWrite(EE_RUN_ADDR, (uint8_t *)&g_tBatPackCtrl.tEERun, sizeof(tEERunParam) / sizeof(uint8_t));
                }
                else
                {
                     wWriteAddr -= sizeof(tEERunParam);
                    if(wWriteAddr < sizeof(tEESpecParam)) 
                    {
                         memcpy(((uint8_t *)&g_tBatPackCtrl.tEESpec) + wWriteAddr,ptrData, wWriteLen << 1);
                        if(wWriteAddr + (wWriteLen << 1) == sizeof(tEESpecParam))
                            l_byResult = byEEMemoryWrite(EE_SPECIAL_ADDR, (uint8_t *)&g_tBatPackCtrl.tEESpec, sizeof(tEESpecParam) / sizeof(uint8_t));
                    }
                    
                }
            }
        }
    }
    //memcpy(((uint8_t *)&g_tFaultDataParam) + wAddr * 2, ptrData, wWriteLen << 1);
        
    wSendLen = 0;
    l_bySendBuf[wSendLen++] = bySlaveAddr;   
    l_bySendBuf[wSendLen++] = CMD_WRITE_REGS;     
    l_bySendBuf[wSendLen++] = (uint8_t)((wAddr  + ADDR_WRITE_REGISTER_OFFSET) >> 8);
    l_bySendBuf[wSendLen++] = (uint8_t)((wAddr  + ADDR_WRITE_REGISTER_OFFSET));
    l_bySendBuf[wSendLen++] = (uint8_t)(wWriteLen >> 8);
    l_bySendBuf[wSendLen++] = (uint8_t)(wWriteLen);       
    
    
    if(Ok ==SCI_GeneralSendParamOut(sRxMsg,l_bySendBuf,wSendLen)&&l_byResult == Ok)
         NVIC_SystemReset();  //软件复位MCU
    return Ok;
}

en_result_t ExecModbusMasterProcessEx(sUARTMsgType* sRxMsg)
{
    uint16_t  l_wRecivCrc,l_wCrc;
    uint16_t l_wAddress, l_wDataLen;  
    en_result_t l_byResult;
  
    if (sRxMsg->r_index < 4)
    {
        return ErrorInvalidParameter;
    }
    
    if (sRxMsg->rbuf[0] < ADDR_MODBUS_START || sRxMsg->rbuf[0] > ADDR_MODBUS_END)
    {
        return ErrorAddressAlignment;
    } 
    
    if (sRxMsg->rbuf[0] != g_tBatPackCtrl.tEESpec.byBmsId&& g_tBatPackCtrl.tEESpec.byBmsId > 0)
    {
        return ErrorAddressAlignment;
    } 
    
    l_wRecivCrc = ((uint16_t)sRxMsg->rbuf[sRxMsg->r_index - 1] << 8) | sRxMsg->rbuf[sRxMsg->r_index - 2];   
    l_wCrc= App_Crc16Calculate((uint8_t*)sRxMsg->rbuf,sRxMsg->r_index - 2);
    if(l_wRecivCrc!=l_wCrc)
        return Error;
    l_wAddress = ((uint16_t)sRxMsg->rbuf[2] << 8) | (uint16_t)sRxMsg->rbuf[3];
    l_wDataLen = ((uint16_t)sRxMsg->rbuf[4] << 8) | (uint16_t)sRxMsg->rbuf[5];
     switch(sRxMsg->rbuf[1])
    {
        case CMD_READ_COIL_STATUS:
            
            break;

        case CMD_READ_INPUT_STATUS:
            l_byResult = ErrorInvalidMode;
            break;

        case CMD_READ_HOLDING_REGS:
            l_byResult = SUNSPEC_ReadHoldingRegister(sRxMsg,sRxMsg->rbuf[0], l_wAddress, l_wDataLen);            
            deep_sleeping = false;
//            shallow_sleeping = false;
            break;

        case CMD_READ_INPUT_REGS:
            l_byResult = ErrorInvalidMode;
            break;

        case CMD_WRITE_SINGLE_COIL:
            break;

        case CMD_WRITE_SINGLE_REG:                
            l_byResult = SUNSPEC_WriteHoldingRegisterSingle(sRxMsg,sRxMsg->rbuf[0], l_wAddress, *(uint16_t *)&sRxMsg->rbuf[4]); 
            
            break;

        case CMD_WRITE_COILS:

            break;

        case CMD_WRITE_REGS:                
            l_byResult = SUNSPEC_WriteHoldingRegisterMulti(sRxMsg,sRxMsg->rbuf[0], l_wAddress, l_wDataLen, (uint8_t *)&sRxMsg->rbuf[7]); 
            break;

        default:
            SUNSPEC_ErrFuncSend(sRxMsg,sRxMsg->rbuf[0], sRxMsg->rbuf[1] + 0x80, E_MODBUS_FUNC_CODE);
            break;
    } 

    if (Ok != l_byResult && ErrorInvalidMode != l_byResult && ErrorAddressAlignment!=l_byResult) 
    {
        SUNSPEC_ErrFuncSend(sRxMsg,sRxMsg->rbuf[0],  sRxMsg->rbuf[1]  + 0x80, l_byResult);
        return Error;
    }
    return l_byResult;    
}
/**********************************************************************************************
 * EOF
**********************************************************************************************/

