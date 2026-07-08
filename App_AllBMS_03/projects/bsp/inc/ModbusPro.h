#ifndef __MODBUSPRO_H__
#define __MODBUSPRO_H__

#include"Usart_Com.h"
#include "MemoryModule.h"

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

#define N_WRITE_HOLDING_SIZE     ((sizeof(tHcuPowerCmd)+sizeof(tDignoseParamIn)+sizeof(tFaultDataParam)+sizeof(tEERunParam)+sizeof(tEESpecParam)+sizeof(tBleName)+1)/2)
#define WRITE_HOLDING_START      0u
#define WRITE_HOLDING_END        (WRITE_HOLDING_START + N_WRITE_HOLDING_SIZE)



en_result_t ExecModbusMasterProcess(USART_Module* USART, sUARTMsgType* sRxMsg);

#define LOG_CMD_BMST    0x1000
#define LOG_CMD_ERRST   0x1001
void SCLogMemorySendData(USART_Module* USART, uint8_t bySlaveAddr, uint16_t wCmd, uint16_t wCount);
void LogMemorySendPoll(USART_Module* USART);

#endif
