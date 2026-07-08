#include <stdio.h>
#include "n32wb43x.h"
#include "User_Can_Config.h"
#include "App.h"
#include "string.h"
#include"Protection.h"

uint8_t u8RxFlag  = false;
uint32_t protect_sata = 0;
CanRxMessage stcRxFrame;


/**
 * @brief  Configures CAN GPIOs
 */
void CAN_GPIO_Config(void)
{
    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    /* Configures CAN IOs */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO | RCC_APB2_PERIPH_GPIOA, ENABLE);
    /* Configure CAN RX PA11 */
    GPIO_InitStructure.Pin       = GPIO_PIN_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Input;
    GPIO_InitStructure.GPIO_Pull = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF1_CAN;
    GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);
    /* Configure CAN TX PA12 */
    GPIO_InitStructure.Pin        = GPIO_PIN_12;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);
}

/**
 * @brief  Configures CAN Filer.
 * @param CAN_BaudRate 10Kbit/s ~ 1Mbit/s
 */
void CAN_Filter_Init(void)
{
    CAN_FilterInitType CAN_FilterInitStructure;
    /* CAN filter init */
    CAN_FilterInitStructure.Filter_Num            = CAN_FILTERNUM0;
    CAN_FilterInitStructure.Filter_Mode           = CAN_Filter_IdMaskMode;
    CAN_FilterInitStructure.Filter_Scale          = CAN_Filter_32bitScale;
    CAN_FilterInitStructure.Filter_HighId         = CAN_FILTER_EXTID_H(0x00005678);
    CAN_FilterInitStructure.Filter_LowId          = CAN_FILTER_EXTID_L(0x00005678);
    CAN_FilterInitStructure.FilterMask_HighId     = CAN_EXT_ID_H_MASK_DONT_CARE;
    CAN_FilterInitStructure.FilterMask_LowId      = CAN_EXT_ID_L_MASK_DONT_CARE;
    CAN_FilterInitStructure.Filter_FIFOAssignment = CAN_FIFO0;
    CAN_FilterInitStructure.Filter_Act            = ENABLE;
    CAN_InitFilter(&CAN_FilterInitStructure);
    CAN_INTConfig(CAN, CAN_INT_FMP0, ENABLE);
}


/**
 * @brief  CAN Interrupt Configures .
 */
void CAN_NVIC_Config(void)
{
    NVIC_InitType NVIC_InitStructure;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    NVIC_InitStructure.NVIC_IRQChannel                   = CAN_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0x0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


/**
 * @brief  Configures CAN.
 * @param CAN_BaudRate 10Kbit/s ~ 1Mbit/s
 */
void CAN_Config(void)
{
    CAN_InitType CAN_InitStructure;
    /* Configure CAN */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_CAN, ENABLE);
    /* CAN register init */
    CAN_DeInit(CAN);
    /* Struct init*/
    CAN_InitStruct(&CAN_InitStructure);
    /* CAN cell init */
    CAN_InitStructure.TTCM              = DISABLE;
    CAN_InitStructure.ABOM              = DISABLE;
    CAN_InitStructure.AWKUM             = DISABLE;
    CAN_InitStructure.NART              = DISABLE;
    CAN_InitStructure.RFLM              = DISABLE;
    CAN_InitStructure.TXFP              = ENABLE;
    CAN_InitStructure.OperatingMode     = OPERATINGMODE;
    CAN_InitStructure.RSJW              = CAN_BIT_RSJW;
    CAN_InitStructure.TBS1              = CAN_BIT_BS1;
    CAN_InitStructure.TBS2              = CAN_BIT_BS2;
    CAN_InitStructure.BaudRatePrescaler = CAN_BAUDRATEPRESCALER;
    /*Initializes the CAN */
    CAN_Init(CAN, &CAN_InitStructure);
    CAN_Filter_Init();
    CAN_NVIC_Config();
}

/**
 * @brief  CAN Transmit Message.
 * @param  CAN
 * @param  TxMessage CAN_TxMessage
 * @return The number of the mailbox that is used for transmission or CAN_TxSTS_NoMailBox if there is no empty mailbox.
 */
uint8_t CANTxMessage(CAN_Module* CANx,CanTxMessage* TxMessage)
{
    return CAN_TransmitMessage(CANx, TxMessage);
}



void CAN_User_Init(void)
{
    CAN_GPIO_Config();
    CAN_Config();
}


uint16_t swap_bytes(uint16_t value) {
    return (value >> 8) | (value << 8);
}
void handleProtectData(uint16_t *protect_data, CanTxMessage *stcTxFrame)
{

    if (stcRxFrame.Data[0] & 0x01) // 读
    {
        if (stcTxFrame->DLC > 2)
        {
            *protect_data = swap_bytes(*protect_data);
        }
        memcpy((stcTxFrame->Data) + 1, protect_data, stcTxFrame->DLC - 1);
    }
    else // 写
    {

        memcpy(protect_data, &stcRxFrame.Data[1], stcTxFrame->DLC - 1);

        memcpy(stcTxFrame->Data + 1, &stcRxFrame.Data[1], stcTxFrame->DLC - 1);
        if (stcTxFrame->DLC > 2)
        {
            *protect_data = swap_bytes(*protect_data);
        }
    }
}

bool AFE_SetFlg = 0;

void CanTxRx(void)
{

    uint8_t i = 0;
    uint8_t TransmitMailbox = 0;
    uint16_t protect_wdata = 0;
    CanTxMessage stcTxFrame;
    uint8_t l_byResult = Error;

    uint16_t Tempmax = g_tBatPackInfo.tCell.byTempMax; // 温度偏移40
    uint16_t Tempmin = g_tBatPackInfo.tCell.byTempMin;
    uint64_t SN = 0x20251210;
    uint16_t SW = 0x101;
    uint8_t HW = 0x01;
    uint8_t soh = 100;
    int32_t data_300 = g_tBatPackInfo.wCur * 10;
    uint64_t data_301 = ((uint64_t)(g_tBatPackInfo.wBatVolt * 100) << 32U) | (uint64_t)(g_tBatPackInfo.wCapVolt * 100);
    uint16_t data_302 = ((uint16_t)soh << 8U) | (uint16_t) (g_tBatPackInfo.wSocShow/100);
    uint32_t data_303 = protect_sata;
    uint64_t data_304 = ((uint64_t)Tempmin << 40) | ((uint64_t)Tempmax << 32) | ((uint64_t)g_tBatPackInfo.tCell.wVolMin << 16U) | (uint64_t)g_tBatPackInfo.tCell.wVolMax;
    uint64_t data_306 = SN;
    uint32_t data_308 = ((uint32_t)HW << 16U) | (uint32_t)SW;
    uint32_t data_30a = 0x0; 
    uint64_t data_30b = ((uint64_t)g_tBatPackInfo.tCell.wVolt[3] << 48U) | ((uint64_t)g_tBatPackInfo.tCell.wVolt[2] << 32U) | ((uint64_t)g_tBatPackInfo.tCell.wVolt[1] << 16U) | (uint64_t)g_tBatPackInfo.tCell.wVolt[0];
    uint64_t data_30c = ((uint64_t)g_tBatPackInfo.tCell.wVolt[7] << 48U) | ((uint64_t)g_tBatPackInfo.tCell.wVolt[6] << 32U) | ((uint64_t)g_tBatPackInfo.tCell.wVolt[5] << 16U) | (uint64_t)g_tBatPackInfo.tCell.wVolt[4];
    uint64_t data_30d = ((uint64_t)g_tBatPackInfo.tCell.wVolt[11] << 48U) | ((uint64_t)g_tBatPackInfo.tCell.wVolt[10] << 32U) | ((uint64_t)g_tBatPackInfo.tCell.wVolt[9] << 16U) | (uint64_t)g_tBatPackInfo.tCell.wVolt[8];
    uint64_t data_30e = ((uint64_t)g_tBatPackInfo.tCell.wVolt[15] << 48U) | ((uint64_t)g_tBatPackInfo.tCell.wVolt[14] << 32U) | ((uint64_t)g_tBatPackInfo.tCell.wVolt[13] << 16U) | (uint64_t)g_tBatPackInfo.tCell.wVolt[12];
    uint64_t data_30f = ((uint64_t)g_tBatPackInfo.tCell.wVolt[19] << 48U) | ((uint64_t)g_tBatPackInfo.tCell.wVolt[18] << 32U) | ((uint64_t)g_tBatPackInfo.tCell.wVolt[17] << 16U) | (uint64_t)g_tBatPackInfo.tCell.wVolt[16];;
    uint64_t data_310 = ((uint64_t)g_tBatPackInfo.tCell.wVolt[23] << 48U) | ((uint64_t)g_tBatPackInfo.tCell.wVolt[22] << 32U) | ((uint64_t)g_tBatPackInfo.tCell.wVolt[21] << 16U) | (uint64_t)g_tBatPackInfo.tCell.wVolt[20];;
    uint32_t data_311 = g_tBatPackCtrl.tEESpec.wHoldingTime;  // 循环次数
    uint32_t data_313 = g_tBatPackCtrl.tEESpec.wBatCapacity*100; // 电池容量
    uint64_t data_314 = ((uint64_t)g_tBatPackInfo.tCell.byTemp[1] << 24U) | ((uint64_t)g_tBatPackInfo.tCell.byTemp[1] << 16U) | ((uint64_t)g_tBatPackInfo.tCell.byTemp[1] << 8U) | (uint64_t)g_tBatPackInfo.tCell.byTemp[0];
    uint64_t data_315 = 0;

    const mod_obj_t regTable_real[] = {
        {0x300, 4, &data_300},
        {0x301, 8, &data_301},
        {0x302, 2, &data_302},
        {0x303, 4, &data_303},
        {0x304, 6, &data_304},
        {0x306, 8, &data_306},
        {0x308, 4, &data_308},
        {0x30a, 4, &data_30a},
        {0x30b, 8, &data_30b},
        {0x30c, 8, &data_30c},
        {0x30d, 8, &data_30d},
        {0x30e, 8, &data_30e},
        {0x30f, 8, &data_30f},
        {0x310, 8, &data_310},
        {0x311, 4, &data_311},
        {0x313, 4, &data_313},
        {0x314, 8, &data_314},
        {0x315, 7, &data_315},
    };

if (true == u8RxFlag)

        {
           u8RxFlag = false;
           g_u64LastCommMs = GetTick_ms();
           if (stcRxFrame.StdId < 0x200 && stcRxFrame.DLC < 4)
           {
             stcTxFrame.StdId = stcRxFrame.StdId;
             stcTxFrame.IDE = CAN_ID_STD;  
             stcTxFrame.RTR = CAN_RTRQ_DATA; 
             stcTxFrame.DLC = stcRxFrame.DLC;
             switch (stcTxFrame.StdId)
             {
                case 0x100: //过压
                    if (stcRxFrame.Data[0] == 1) // 读保护
                    {
                        protect_wdata =  g_tFaultDataParam.tCellVolt[2].wUpperVal;
                    }
                    if (stcRxFrame.Data[0] == 3) // 读恢复
                    {
                        protect_wdata = g_tFaultDataParam.tCellVolt[2].wLowerVal;
                    }

                    handleProtectData(&protect_wdata, &stcTxFrame);

                    if ((stcRxFrame.Data[0] == 0) && (g_tFaultDataParam.tCellVolt[2].wUpperVal != protect_wdata)) // 写保护
                    {
                        g_tFaultDataParam.tCellVolt[2].wUpperVal = protect_wdata;
                        AFE_SetFlg = 1;
                    }
                    if ((stcRxFrame.Data[0] == 2) && (g_tFaultDataParam.tCellVolt[2].wLowerVal != protect_wdata)) // 写恢复
                    {
                        g_tFaultDataParam.tCellVolt[2].wLowerVal = protect_wdata;
                        AFE_SetFlg = 1;
                    }

                    break;
                case 0x101: //欠压
                    if (stcRxFrame.Data[0] == 1) // 读保护
                    {
                        protect_wdata = g_tFaultDataParam.tCellVolt[5].wUpperVal;
                    }
                    if (stcRxFrame.Data[0] == 3) // 读恢复
                    {
                        protect_wdata = g_tFaultDataParam.tCellVolt[5].wLowerVal;
                    }

                    handleProtectData(&protect_wdata, &stcTxFrame);

                    if ((stcRxFrame.Data[0] == 0) && (g_tFaultDataParam.tCellVolt[5].wUpperVal != protect_wdata))// 写保护
                    {
                      
                        g_tFaultDataParam.tCellVolt[5].wUpperVal = protect_wdata;
                        AFE_SetFlg = 1;
                    }
                    if ((stcRxFrame.Data[0] == 2) && (g_tFaultDataParam.tCellVolt[5].wLowerVal != protect_wdata)) // 写恢复
                    {
                       g_tFaultDataParam.tCellVolt[5].wLowerVal = protect_wdata;
                        AFE_SetFlg = 1;
                    }
                    break;

                case 0x102: //充电过流

                    if (stcRxFrame.Data[0] == 1) // 读保护
                    {
                        protect_wdata = g_tBatPackCtrl.tEESpec.wPackChgMax/10;
                    }

                    handleProtectData(&protect_wdata, &stcTxFrame);

                    if ((stcRxFrame.Data[0] == 0) && ((g_tBatPackCtrl.tEESpec.wPackChgMax/10) != protect_wdata)) // 写保护
                    {
                        g_tBatPackCtrl.tEESpec.wPackChgMax = protect_wdata * 10;
                        AFE_SetFlg = 1;
                    }
                    break;
                case 0x103:

                    if (stcRxFrame.Data[0] == 1) // 读保护
                    {
                        protect_wdata = g_tBatPackCtrl.tEESpec.wPackDischgMax / 10;
                    }

                    handleProtectData(&protect_wdata, &stcTxFrame);

                    if ((stcRxFrame.Data[0] == 0) && ((g_tBatPackCtrl.tEESpec.wPackDischgMax / 10) != protect_wdata)) // 写保护
                    {
                        g_tBatPackCtrl.tEESpec.wPackDischgMax = protect_wdata * 10;
                        AFE_SetFlg = 1;
                    }
                    break;
                case 0x104://充电高温
                    if (stcRxFrame.Data[0] == 1) // 读保护
                    {
                        protect_wdata =  g_tFaultDataParam.tCellTemp[8].byUpperVal;
                    }
                    if (stcRxFrame.Data[0] == 3) // 读恢复
                    {
                        protect_wdata = g_tFaultDataParam.tCellTemp[8].byLowerVal;
                    }

                    handleProtectData(&protect_wdata, &stcTxFrame);

                    if ((stcRxFrame.Data[0] == 0) && (g_tFaultDataParam.tCellTemp[8].byUpperVal!= protect_wdata)) // 写保护
                    {
                        g_tFaultDataParam.tCellTemp[8].byUpperVal = protect_wdata - 40;
                        AFE_SetFlg = 1;
                    }
                    if ((stcRxFrame.Data[0] == 2) && (g_tFaultDataParam.tCellTemp[8].byLowerVal != protect_wdata)) // 写恢复
                    {
                       g_tFaultDataParam.tCellTemp[8].byLowerVal = protect_wdata;
                        AFE_SetFlg = 1;
                    }
                    break;
                case 0x105://放电高温
                    if (stcRxFrame.Data[0] == 1) // 读保护
                    {
                        protect_wdata = g_tFaultDataParam.tCellTemp[2].byUpperVal;
                    }
                    if (stcRxFrame.Data[0] == 3) // 读恢复
                    {
                        protect_wdata = g_tFaultDataParam.tCellTemp[2].byLowerVal;
                    }

                    handleProtectData(&protect_wdata, &stcTxFrame);

                    if ((stcRxFrame.Data[0] == 0) && (g_tFaultDataParam.tCellTemp[2].byUpperVal != protect_wdata)) // 写保护
                    {
                        g_tFaultDataParam.tCellTemp[2].byUpperVal = protect_wdata;
                        AFE_SetFlg = 1;
                    }
                    if ((stcRxFrame.Data[0] == 2) && (g_tFaultDataParam.tCellTemp[2].byLowerVal != protect_wdata)) // 写恢复
                    {
                        g_tFaultDataParam.tCellTemp[2].byLowerVal = protect_wdata;
                        AFE_SetFlg = 1;
                    }
                    break;
                case 0x106://充电低温
                    if (stcRxFrame.Data[0] == 1) // 读保护
                    {
                        protect_wdata = g_tFaultDataParam.tCellTemp[9].byUpperVal;
                    }
                    if (stcRxFrame.Data[0] == 3) // 读恢复
                    {
                        protect_wdata = g_tFaultDataParam.tCellTemp[9].byLowerVal;
                    }

                    handleProtectData(&protect_wdata, &stcTxFrame);

                    if ((stcRxFrame.Data[0] == 0) && (g_tFaultDataParam.tCellTemp[9].byUpperVal != protect_wdata)) // 写保护
                    {
                        g_tFaultDataParam.tCellTemp[9].byUpperVal = protect_wdata;
                        AFE_SetFlg = 1;
                    }
                    if ((stcRxFrame.Data[0] == 2) && (g_tFaultDataParam.tCellTemp[9].byLowerVal != protect_wdata)) // 写恢复
                    {
                        g_tFaultDataParam.tCellTemp[9].byLowerVal = protect_wdata;
                        AFE_SetFlg = 1;
                    }
                    break;
                case 0x107://放电低温
                    if (stcRxFrame.Data[0] == 1) // 读保护
                    {
                        protect_wdata = g_tFaultDataParam.tCellTemp[5].byUpperVal;
                    }
                    if (stcRxFrame.Data[0] == 3) // 读恢复
                    {
                        protect_wdata = g_tFaultDataParam.tCellTemp[5].byLowerVal;
                    }

                    handleProtectData(&protect_wdata, &stcTxFrame);

                    if ((stcRxFrame.Data[0] == 0) && (g_tFaultDataParam.tCellTemp[5].byUpperVal != protect_wdata)) // 写保护
                    {
                        g_tFaultDataParam.tCellTemp[5].byUpperVal = protect_wdata;
                        AFE_SetFlg = 1;
                    }
                    if ((stcRxFrame.Data[0] == 2) && (g_tFaultDataParam.tCellTemp[5].byLowerVal != protect_wdata)) // 写恢复
                    {
                        g_tFaultDataParam.tCellTemp[5].byLowerVal = protect_wdata - 40;
                        AFE_SetFlg = 1;
                    }
                    break;
                case 0x108://总压过压

                    if (stcRxFrame.Data[0] == 1) // 读保护
                    {
                        protect_wdata = g_tFaultDataParam.tBattVolt[2].wUpperVal;
                    }
                    if (stcRxFrame.Data[0] == 3) // 读恢复
                    {
                        protect_wdata =  g_tFaultDataParam.tBattVolt[2].wLowerVal;
                    }

                    handleProtectData(&protect_wdata, &stcTxFrame);

                    if ((stcRxFrame.Data[0] == 0) && (g_tFaultDataParam.tBattVolt[2].wUpperVal != protect_wdata)) // 写保护
                    {

                        g_tFaultDataParam.tBattVolt[2].wUpperVal = protect_wdata;
                        AFE_SetFlg = 1;
                    }
                    if ((stcRxFrame.Data[0] == 2) && (g_tFaultDataParam.tBattVolt[2].wLowerVal != protect_wdata)) // 写恢复
                    {
                        g_tFaultDataParam.tBattVolt[2].wLowerVal = protect_wdata;
                        AFE_SetFlg = 1;
                    }

                    break;
                case 0x109://总压欠压
                    if (stcRxFrame.Data[0] == 1) // 读保护
                    {
                        protect_wdata = g_tFaultDataParam.tBattVolt[5].wUpperVal;
                    }
                    if (stcRxFrame.Data[0] == 3) // 读恢复
                    {
                        protect_wdata = g_tFaultDataParam.tBattVolt[5].wLowerVal;
                    }

                    handleProtectData(&protect_wdata, &stcTxFrame);

                    if ((stcRxFrame.Data[0] == 0) && (g_tFaultDataParam.tBattVolt[5].wUpperVal != protect_wdata)) // 写保护
                    {

                        g_tFaultDataParam.tBattVolt[5].wUpperVal = protect_wdata;
                        AFE_SetFlg = 1;
                    }
                    if ((stcRxFrame.Data[0] == 2) && (g_tFaultDataParam.tBattVolt[5].wLowerVal != protect_wdata)) // 写恢复
                    {
                        g_tFaultDataParam.tBattVolt[5].wLowerVal = protect_wdata;
                        AFE_SetFlg = 1;
                    }
                    break;
                default:
                    break;
             }
            TransmitMailbox = CANTxMessage(CAN, &stcTxFrame);                                                  
            while ((CAN_TransmitSTS(CAN, TransmitMailbox) != CANTXSTSOK));
           }
           else
           {
                switch (stcRxFrame.StdId)
                {
                case 0x200:
                    i = 0;
                    break;
                case 0x201:
                    i = 1;
                    break;
                case 0x202:
                    i = 2;
                    break;
                case 0x203:
                    i = 3;
                    break;
                case 0x204:
                    i = 4;
                    break;
                case 0x206:
                    i = 5;
                    break;
                case 0x208:
                    i = 6;
                    break;
                case 0x20a:
                    i = 7;
                    break;
                case 0x20b:
                    i = 8;
                    break;
                case 0x20c:
                    i = 9;
                    break;
                case 0x20d:
                    i = 10;
                    break;
                case 0x20e:
                    i = 11;
                    break;
                case 0x20f:
                    i = 12;
                    break;
                case 0x210:
                    i = 13;
                    break;
                case 0x211:
                    i = 14;
                    break;
                case 0x213:
                    i = 15;
                    break;
                case 0x214:
                    i = 16;
                    break;
                case 0x215:
                    i = 17;
                    break;
                default:
                    i = 0xff;
                    break;
                }
                if (i != 0xff)
                {
                    stcTxFrame.StdId = regTable_real[i].id;
                    stcTxFrame.IDE     = CAN_ID_STD;  
                    stcTxFrame.RTR     = CAN_RTRQ_DATA; 
                    stcTxFrame.DLC = regTable_real[i].len;
                    memcpy(stcTxFrame.Data, (uint8_t *)regTable_real[i].pDat, stcTxFrame.DLC);
                   TransmitMailbox = CANTxMessage(CAN, &stcTxFrame);                                                  
                   while ((CAN_TransmitSTS(CAN, TransmitMailbox) != CANTXSTSOK));
                }
                else
                {
                    stcTxFrame.StdId = 0;
                    stcTxFrame.IDE     = CAN_ID_STD;  
                    stcTxFrame.RTR     = CAN_RTRQ_DATA; 
                    stcTxFrame.DLC = 8;
                    memset(stcTxFrame.Data, 0, stcTxFrame.DLC);
                    TransmitMailbox = CANTxMessage(CAN, &stcTxFrame);                                                  
                    while ((CAN_TransmitSTS(CAN, TransmitMailbox) != CANTXSTSOK));
                }
           }
                
        }

        if (AFE_SetFlg)
        {
            AFE_SetFlg = 0;
           l_byResult = byEEMemoryWrite(EE_FAULT_LEVEL_PARAM_START_ADDR, (uint8_t *)&g_tFaultDataParam, sizeof(tFaultDataParam) / sizeof(uint8_t));
           l_byResult =  byEEMemoryWrite(EE_RUN_ADDR, (uint8_t *)&g_tBatPackCtrl.tEERun, sizeof(tEERunParam) / sizeof(uint8_t));
            if (l_byResult == Ok)
            {
                AFEParam_Config();
            }
           
        }
        
}
    
 
