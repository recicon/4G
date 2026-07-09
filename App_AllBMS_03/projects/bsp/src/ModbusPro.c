/**********************************************************************************************
 *                             Include File
**********************************************************************************************/
#include "ModbusPro.h"
#include "CrcCfg.h"  
#include "MemoryModule.h" 
#include <string.h>
#include"UserDef.h"
#include"App.h"
#include"port.h"
#include "n32wb43x_it.h"
#include "app_proto_cmn.h"
#include "app_proto_cmd.h"
#include "app_uart.h"
#include "Comm4G.h"     /* 4G(USART1) 发送分流: Comm4G_SocketSend */
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

///**********************************************************************************************
//*							   Variable Declaration
//**********************************************************************************************/

///**********************************************************************************************
//*                              Function Declaration
//**********************************************************************************************/
static en_result_t  SCI_GeneralSendParamOut(USART_Module* USART, uint8_t *ptrData, uint16_t wDataLen)
{

    uint8_t l_bySendBuf[2];
    uint16_t l_wCrc;

    if (wDataLen > N_READ_DATA_LEN - 2)
        return Error;
    l_wCrc = App_Crc16Calculate(ptrData, wDataLen);
    l_bySendBuf[0] = (uint8_t)(l_wCrc);
    l_bySendBuf[1] = (uint8_t)(l_wCrc >> 8);
    switch ((uint32_t)USART)
    {
    case (uint32_t)USART485:
        memcpy(g_sMainRtxMsg.wbuf, ptrData, wDataLen);
        memcpy(&g_sMainRtxMsg.wbuf[wDataLen], l_bySendBuf, 2);
        g_sMainRtxMsg.w_num = wDataLen + 2;
        // 启动发送中断
        GPIO_SetBits(UARTTxRxm_PORT, UARTTxRxm_PIN);
        Delay_us(3);
        USART_ConfigInt(USART485, USART_INT_TXDE, ENABLE);
        break;
    case (uint32_t)UART:
        memcpy(g_sUartRtxMsg.wbuf, ptrData, wDataLen);
        memcpy(&g_sUartRtxMsg.wbuf[wDataLen], l_bySendBuf, 2);
        g_sUartRtxMsg.w_num = wDataLen + 2;
        USART_ConfigInt(UART, USART_INT_TXDE, ENABLE);
        break;
     case (uint32_t)USARTy:
        if(app_cmn_slv_env.tx_stat == TX_STAT_IDLE)
        {
            memcpy(g_sBleRtxMsg.wbuf,ptrData,wDataLen);
            memcpy(&g_sBleRtxMsg.wbuf[wDataLen],l_bySendBuf,2);
            g_sBleRtxMsg.w_num =  wDataLen+2;   
            at_blesend(g_sBleRtxMsg.wbuf, g_sBleRtxMsg.w_num);
        }
        break;
#if EN_4GCOMM
    case (uint32_t)USART1:
        memcpy(g_s4GRtxMsg.wbuf, ptrData, wDataLen);
        memcpy(&g_s4GRtxMsg.wbuf[wDataLen], l_bySendBuf, 2);
        g_s4GRtxMsg.w_num = wDataLen + 2;
        Comm4G_SocketSend(g_s4GRtxMsg.wbuf, g_s4GRtxMsg.w_num);  /* 置待发,由Comm4G_Loop异步CIPSEND */
        break;
#endif
    default:
        break;
    }
    return Ok;
}

static en_result_t SUNSPEC_ErrFuncSend(USART_Module* USART, uint8_t bySlaveAddr, uint8_t byFuncCode, uint8_t byErrCode)
{
   uint8_t l_bySendBuf[4];
    uint16_t  wLen;

    wLen = 0;
    l_bySendBuf[wLen++] = bySlaveAddr;
    l_bySendBuf[wLen++] = byFuncCode;
    l_bySendBuf[wLen++] = byErrCode;

    return SCI_GeneralSendParamOut(USART, l_bySendBuf,wLen) ;        
}

static en_result_t SUNSPEC_ReadHoldingRegister(USART_Module* USART, uint8_t bySlaveAddr, uint16_t wAddr, uint16_t wReadLen)
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
                           else
                           {
                                wSendAddr -= sizeof(tEESpecParam); 
                                 if(wSendAddr < sizeof(tBleName))
                                     l_bySendBuf[wSendLen++] = *((uint8_t *)&g_tBleName + wSendAddr);
                           }
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
    return SCI_GeneralSendParamOut(USART, l_bySendBuf,wSendLen);     
}
static en_result_t SUNSPEC_WriteHoldingRegisterSingle(USART_Module* USART, uint8_t bySlaveAddr, uint16_t wAddr, uint16_t wData)
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
                         uint8_t byBatType=g_tBatPackCtrl.tEESpec.byBatType;  
                         memcpy(((uint8_t *)&g_tBatPackCtrl.tEESpec) + wWriteAddr, &wData, 2);
                         wData = *((uint16_t *)&g_tBatPackCtrl.tEESpec + (wWriteAddr>>1)) = wData;                                             
                        if(byBatType!=g_tBatPackCtrl.tEESpec.byBatType)//电池类型变化
                        {
                           FaultLevelOneKey(g_tBatPackCtrl.tEESpec.byBatType); 
                        }
                        if(wWriteAddr + 2 == sizeof(tEESpecParam))
                           l_byResult = byEEMemoryWrite(EE_SPECIAL_ADDR, (uint8_t *)&g_tBatPackCtrl.tEESpec, sizeof(tEESpecParam) / sizeof(uint8_t));
                    }
                     else
                    {
                         wWriteAddr -= sizeof(tEESpecParam);
                        if(wWriteAddr < sizeof(tBleName)) 
                        {
                             memcpy(((uint8_t *)&g_tBleName) + wWriteAddr, &wData, 2);
                             wData = *((uint16_t *)&g_tBleName + (wWriteAddr>>1)) = wData;
                            if(wWriteAddr + 2 == sizeof(tBleName))
                               l_byResult = byEEMemoryWrite(EE_BLENAME_END_ADDR, (uint8_t *)&g_tBleName, sizeof(tBleName) / sizeof(uint8_t));
                        }
                      
                    }
                  
                }
            }
        }
    }
    l_bySendBuf[wSendLen++] = (uint8_t)(wData >> 8);
    l_bySendBuf[wSendLen++] = (uint8_t)(wData);

    
   if(Ok== SCI_GeneralSendParamOut(USART, l_bySendBuf,wSendLen)&&Ok ==l_byResult)
    {
         Delay_ms(20); 
         NVIC_SystemReset();  //软件复位MCU
    }
   return Ok;
}

static en_result_t  SUNSPEC_WriteHoldingRegisterMulti(USART_Module* USART, uint8_t bySlaveAddr, uint16_t wAddr, uint16_t wWriteLen, uint8_t *ptrData)
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
                        byEEMemoryWrite(EE_RUN_ADDR, (uint8_t *)&g_tBatPackCtrl.tEERun, sizeof(tEERunParam) / sizeof(uint8_t));
                }
                else
                {
                     wWriteAddr -= sizeof(tEERunParam);
                    if(wWriteAddr < sizeof(tEESpecParam)) 
                    {
                        uint8_t byBatType=g_tBatPackCtrl.tEESpec.byBatType;                        
                        memcpy(((uint8_t *)&g_tBatPackCtrl.tEESpec) + wWriteAddr,ptrData, wWriteLen << 1);
                        if(byBatType!=g_tBatPackCtrl.tEESpec.byBatType)//电池类型变化
                        {
                           FaultLevelOneKey(g_tBatPackCtrl.tEESpec.byBatType); 
                        }
                        if(wWriteAddr + (wWriteLen << 1) == sizeof(tEESpecParam))
                            l_byResult = byEEMemoryWrite(EE_SPECIAL_ADDR, (uint8_t *)&g_tBatPackCtrl.tEESpec, sizeof(tEESpecParam) / sizeof(uint8_t));
                    }
                    else
                    {
                          wWriteAddr -= sizeof(tEESpecParam);
                          if(wWriteAddr<sizeof(tBleName))  
                          {
                            memcpy(((uint8_t *)&g_tBleName) + wWriteAddr,ptrData, wWriteLen << 1);
                            if(wWriteAddr + (wWriteLen << 1) == sizeof(tBleName))
                                l_byResult = byEEMemoryWrite(EE_BLENAME_ADDR, (uint8_t *)&g_tBleName, sizeof(tBleName) / sizeof(uint8_t));
                         }
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
    
    
    if(Ok ==SCI_GeneralSendParamOut(USART, l_bySendBuf,wSendLen)&&l_byResult == Ok)
    {
         Delay_ms(20); 
         NVIC_SystemReset();  //软件复位MCU
    }
    return Ok;
}

en_result_t ExecModbusMasterProcess(USART_Module* USART, sUARTMsgType* sRxMsg)
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
    
    if (sRxMsg->rbuf[0] != g_tBatPackCtrl.tEESpec.byBmsId)
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
            SCLogMemorySendData(USART, sRxMsg->rbuf[0], LOG_CMD_ERRST, l_wDataLen);
            l_byResult = Ok;
            break;

        case CMD_READ_HOLDING_REGS:
            l_byResult = SUNSPEC_ReadHoldingRegister(USART, sRxMsg->rbuf[0], l_wAddress, l_wDataLen);            
           deep_sleeping = false;
//            shallow_sleeping = false;
            break;

        case CMD_READ_INPUT_REGS:
            SCLogMemorySendData(USART, sRxMsg->rbuf[0], LOG_CMD_BMST, l_wDataLen);
            l_byResult = Ok;
            break;

        case CMD_WRITE_SINGLE_COIL:
            break;

        case CMD_WRITE_SINGLE_REG:                
            l_byResult = SUNSPEC_WriteHoldingRegisterSingle(USART, sRxMsg->rbuf[0], l_wAddress, *(uint16_t *)&sRxMsg->rbuf[4]); 
            
            break;

        case CMD_WRITE_COILS:

            break;

        case CMD_WRITE_REGS:                
            l_byResult = SUNSPEC_WriteHoldingRegisterMulti(USART, sRxMsg->rbuf[0], l_wAddress, l_wDataLen, (uint8_t *)&sRxMsg->rbuf[7]); 
            break;

        default:
            SUNSPEC_ErrFuncSend(USART, sRxMsg->rbuf[0], sRxMsg->rbuf[1] + 0x80, E_MODBUS_FUNC_CODE);
            break;
    } 

    if (Ok != l_byResult && ErrorInvalidMode != l_byResult && ErrorAddressAlignment!=l_byResult) 
    {
        SUNSPEC_ErrFuncSend(USART, sRxMsg->rbuf[0],  sRxMsg->rbuf[1]  + 0x80, l_byResult);
        return Error;
    }
    return l_byResult;    
}
/**********************************************************************************************
 * EOF
**********************************************************************************************/


/* 日志帧改用 SCI_GeneralSendParamOut 发送(自带 CRC16)，原 LogSendResponse(不含 CRC)已弃用删除 */

/*===========================================================================
 *  日志记录(历史/告警) 300ms 周期分批发送
 *  收到读命令后启动对应通道的状态机，由 LogMemorySendPoll 每 300ms 倒序发一条，
 *  发完全部记录补一帧 0xFF 0xFF 结束。485/BLE/UART 各一份状态机，互不干扰，
 *  命令从哪个通道来就从哪个通道发回。
 *===========================================================================*/
#define LOG_SEND_INTERVAL_MS   300

typedef struct {
    uint8_t  byActive;     /* 0=空闲 1=逐条发送中 2=发结束帧 */
    uint16_t wCmd;         /* LOG_CMD_BMST / LOG_CMD_ERRST (16位，不能截断成 uint8) */
    uint8_t  bySlaveAddr;  /* 回帧从机地址 */
    uint32_t dwSendIdx;    /* 已发游标 0..dwTotal */
    uint32_t dwTotal;      /* 本轮要发的条数 = min(wCount, dwAvail) */
    uint32_t dwAvail;      /* 启动时实际记录总数(定位最新记录) */
    uint64_t u64LastTick;  /* 上次发送时刻(300ms 节流) */
} tLogSendCtrl;

#if EN_4GCOMM
static tLogSendCtrl s_aLogSend[4];   /* 0=485 1=BLE 2=UART 3=4G */
#else
static tLogSendCtrl s_aLogSend[3];   /* 0=485 1=BLE 2=UART */
#endif

static uint8_t LogChIndex(USART_Module* USART)
{   /* 与 SCI_GeneralSendParamOut 的通道分流口径保持一致 */
    if ((uint32_t)USART == (uint32_t)USARTy) return 1;   /* BLE */
    if ((uint32_t)USART == (uint32_t)UART)   return 2;   /* UART */
#if EN_4GCOMM
    if ((uint32_t)USART == (uint32_t)USART1) return 3;   /* 4G */
#endif
    return 0;                                            /* 485 (default) */
}

/* 收到 Modbus 读日志命令时调用：启动该通道的分批发送状态机（不再一次性全发）。
 * 发送条数由请求的 wCount 决定（取最新的 wCount 条，不超过实际记录数）。 */
void SCLogMemorySendData(USART_Module* USART, uint8_t bySlaveAddr, uint16_t wCmd, uint16_t wCount)
{
    tLogSendCtrl* p = &s_aLogSend[LogChIndex(USART)];
    uint32_t dwAvail = (wCmd == LOG_CMD_ERRST) ? LOG_ErrStItemCount() : LOG_BmsStItemCount();
    p->wCmd        = wCmd;
    p->bySlaveAddr = bySlaveAddr;
    p->dwAvail     = dwAvail;
    p->dwTotal     = (wCount < dwAvail) ? wCount : dwAvail;  /* 发送条数由 wCount 决定，不超过实际记录数 */
    p->dwSendIdx   = 0;
    p->u64LastTick = GetTick_ms() - LOG_SEND_INTERVAL_MS;    /* 让首条立即发 */
    p->byActive    = (p->dwTotal > 0) ? 1 : 2;               /* 无记录/请求0条则直接发结束帧 */
}

/* 挂在各消息泵的空闲分支：每 300ms 推进一步——倒序发一条记录，发完补结束帧。 */
void LogMemorySendPoll(USART_Module* USART)
{
    tLogSendCtrl* p = &s_aLogSend[LogChIndex(USART)];
    uint8_t aBuf[3 + LOG_ITEM_SIZE];
    uint8_t byFunc;

    if (p->byActive == 0) return;
    if (GetTick_ms() - p->u64LastTick < LOG_SEND_INTERVAL_MS) return;
    p->u64LastTick = GetTick_ms();

    byFunc = (p->wCmd == LOG_CMD_ERRST) ? CMD_READ_INPUT_STATUS : CMD_READ_INPUT_REGS;

    if (p->byActive == 1)   /* 逐条发送，倒序：最新在前 */
    {
        uint32_t dwIdx = p->dwAvail - 1 - p->dwSendIdx;   /* 从最新记录往旧发 wCount 条 */
        uint8_t  byOk;
        aBuf[0] = p->bySlaveAddr;
        aBuf[1] = byFunc;
        aBuf[2] = LOG_ITEM_SIZE;
        byOk = (p->wCmd == LOG_CMD_ERRST)
             ? LOG_ErrStItemReadByIndex((uint16_t)dwIdx, (tLogErrStItem *)&aBuf[3])
             : LOG_BmsStItemReadByIndex((uint16_t)dwIdx, (tLogBmStItem *)&aBuf[3]);
        if (byOk)
            SCI_GeneralSendParamOut(USART, aBuf, 3 + LOG_ITEM_SIZE);   /* 含 CRC16；读失败则本拍跳过 */

        if (++p->dwSendIdx >= p->dwTotal)
            p->byActive = 2;
    }
    else                    /* byActive==2：结束帧 */
    {
        aBuf[0] = p->bySlaveAddr;
        aBuf[1] = byFunc;
        aBuf[2] = 0x02;
        aBuf[3] = 0xFF;
        aBuf[4] = 0xFF;
        SCI_GeneralSendParamOut(USART, aBuf, 5);   /* 含 CRC16 */
        p->byActive = 0;
    }
}
