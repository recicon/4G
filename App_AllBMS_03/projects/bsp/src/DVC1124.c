#include"DVC1124.h"
#include "bms_config.h"

#define TEMP_CELL_HOTCHG_LOW_UPPER_III     (-5   + 40)
#define TEMP_CELL_HOTCHG_LOW_LOWER_III		(10   + 40)
TAFERegs g_AfeRegs;
float g_fSampleCur;                        // 电流值A 充电为负，放电为正
float g_fRawCur;                           // 原始电流值（校准前）单位A

u8 I2CTransferBuffer[300];
bool bCHGING;
bool bDSGING;
bool g_bCellCountDone = false;  // 串数识别完成标志

static SI2C_HANDLE SI2C_handle;

#define AFEI2C_SDA_GPIOx 	GPIOB
#define AFEI2C_SCL_GPIOx 	GPIOB
#define AFEI2C_SDA_Pin 	  GPIO_PIN_9
#define AFEI2C_SCL_Pin    GPIO_PIN_8

const u8 CRC8_LOOKUP_TABLE[256] ={
    0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
    0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
    0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
    0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
    0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
    0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
    0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
    0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
    0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
    0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
    0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
    0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
    0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63,
    0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
    0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
    0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3};
/**
 * @说明	crc8计算
 * @参数	数据地址、长度
 * @返回值	u8 crc值
 * @注
 */
u8 calc_crc8(void *dataPtr, u16 dataLen)
{ // 查表法
  u8 carry = 0;
  while (dataLen--)
  {
    carry = CRC8_LOOKUP_TABLE[carry ^ (*(u8 *)dataPtr)];
    dataPtr = (u8 *)dataPtr + 1;
  }
  return carry;
}

bool IIC_WriteDataWithCRC(u8 regAddr, void *dataPtr, u16 dataLen)
{
  u8 *des = I2CTransferBuffer, *src = (u8 *)dataPtr;
  *des++ = ADDRESS | 0x00;
  *des++ = regAddr;
  *des++ = *src++;
  *des++ = calc_crc8(I2CTransferBuffer, 3);
  while (--dataLen > 0)
  {
    *des++ = *src;
    *des++ = calc_crc8(src++, 1);
  }
  return SI2C_WriteReg(&SI2C_handle, ADDRESS, regAddr, true, I2CTransferBuffer+2, (uint16_t)(des - I2CTransferBuffer)-2);
}

bool IIC_ReadDataWithCRC(uint8_t regAddr, void *dataPtr, uint16_t dataLen)
{
  uint8_t dummyWr[4], *src;
  dummyWr[0] = ADDRESS;
  dummyWr[1] = regAddr;
  if (dataLen <= (sizeof(I2CTransferBuffer) << 1) && Ok == SI2C_ReadReg(&SI2C_handle, ADDRESS, regAddr,true, I2CTransferBuffer, dataLen << 1))
  {
    dummyWr[2] = ADDRESS | 0x1;
    dummyWr[3] = I2CTransferBuffer[0];
    if (I2CTransferBuffer[1] == calc_crc8(dummyWr, 4))
    {
      int remains;
      if (dataLen > 1)
      {
        for (remains = dataLen - 1, src = I2CTransferBuffer + 2; remains > 0; remains--, src += 2)
        {
          if (src[1] != calc_crc8(src, 1))
          {
            return ERROR;
          }
        }
        for (remains = dataLen, src = I2CTransferBuffer; remains > 0; remains--, src += 2)
        {
          *(uint8_t *)dataPtr = *src;
          dataPtr = (uint8_t *)dataPtr + 1;
        }
        return SUCCESS;
      }
      else if (dataLen == 1)
      {
        *(uint8_t *)dataPtr = I2CTransferBuffer[0];
        return SUCCESS;
      }
    }
  }
  return ERROR;
}

bool DVC1124_ReadRegs(uint8_t regAddr, uint8_t regLen)
{
  int retry;
  for (retry = 5; retry > 0; retry--)
  {
    if (IIC_ReadDataWithCRC(regAddr, (uint8_t *)&g_AfeRegs + regAddr, regLen))
    {
      g_tBatPackInfo.tBmDiagVal.tBits.u1AfeCommFlg = 0;
      return SUCCESS;
    }
    else
      Delay_us(5);
  }
  g_tBatPackInfo.tBmDiagVal.tBits.u1AfeCommFlg = 1;
  return ERROR;
}

bool DVC1124_WriteRegs(uint8_t regAddr, uint8_t regLen)
{
  uint8_t retry;
  for (retry = 5; retry > 0; retry--)
  {
    if (IIC_WriteDataWithCRC(regAddr, (uint8_t *)&g_AfeRegs + regAddr, regLen) == 0)
    {
      return SUCCESS;
    }
    else
      Delay_us(5);
  }
  return ERROR;
}

bool DVC1124_InitRegs(void)
{
  memcpy((char *)&g_AfeRegs + 81, DVC1124_PresetConfigRegData_R81To121, sizeof(DVC1124_PresetConfigRegData_R81To121));
  return DVC1124_WriteRegs(AFE_ADDR_R(81), sizeof(DVC1124_PresetConfigRegData_R81To121));
}

bool AFE_RST(void)
{
  g_AfeRegs.R1.CST = 0xd;
  return DVC1124_WriteRegs(AFE_ADDR_R(1), 1);
}

void CleanError(void)
{
  g_AfeRegs.R0.cleanflag = 0;
  if (!DVC1124_WriteRegs(AFE_ADDR_R(0), 1))
  {
    g_tBatPackInfo.tBmDiagVal.tBits.u1AfeCommFlg = 1;
  }
}

void DVC1124_Init(void)
{
  if (!DVC1124_InitRegs())
  {
    g_tBatPackInfo.tBmDiagVal.tBits.u1AfeCommFlg = 1;
  }
  GPM_Setup();
  CleanError();
}

void AFE_Iicinit(void)
{
  GPIO_InitType GPIO_InitStructure;
  GPIO_InitStructure.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStructure.GPIO_Current = GPIO_DC_4mA;
  GPIO_InitStructure.GPIO_Pull = GPIO_No_Pull;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitPeripheral(GPIOB, &GPIO_InitStructure);
  GPIO_SetBits(GPIOB,GPIO_PIN_8);
  GPIO_ResetBits(GPIOB,GPIO_PIN_9);
  Delay_ms(3);
  SI2C_Init(&SI2C_handle,AFEI2C_SDA_GPIOx, AFEI2C_SCL_GPIOx, AFEI2C_SDA_Pin, AFEI2C_SCL_Pin, 5);
  AFE_RST();
  Delay_ms(300);
  DVC1124_Init();
}

void DVC1124_Sleep(void){
  g_AfeRegs.R1.CST=0x0E;
  DVC1124_WriteRegs(AFE_ADDR_R(1),1);
}

void DVC1124_Offmos(void){
  g_AfeRegs.R1.CST=0x0F;
  DVC1124_WriteRegs(AFE_ADDR_R(1),1);
}

/**
 * @说明	电压二次校准
 * @参数	电压采样值、电池串号
 * @返回值	float 电压值
 * @注	单位mV
 */
static float s_SecondCALI_p1 = 0.35f / 1000000.0f, s_SecondCALI_p2 = -0.12f / 1000000.0f, s_SecondCALI_preVcm = 0, s_SecondCALI_preVcell_2nd = 0;
static float CellVolSecondaryCalibrate(float value, int cellno)
{

  if (cellno == 0)
  {
    s_SecondCALI_preVcm = 0;
    s_SecondCALI_preVcell_2nd = value;
  }
  else
  {
    s_SecondCALI_preVcm += s_SecondCALI_preVcell_2nd;
    s_SecondCALI_preVcell_2nd = value / (1 + s_SecondCALI_p1 * s_SecondCALI_preVcm + s_SecondCALI_p2 * s_SecondCALI_preVcm * s_SecondCALI_preVcm);
  }
  return s_SecondCALI_preVcell_2nd;
}

float DVC1124_Calc_VCell(uint8_t cellIndex)
{
  float fValue;
  uint16_t uwValue;
  uint8_t cellMask=cellIndex;

#if (BMS_MODEL == MODEL_17S100AQ)
  if (cellIndex > AFE_MAX_CELL_CNT - 1)
    return ERROR;
#else
  if (cellIndex > g_tBatPackCtrl.tEESpec.byPackCell - 1)
    return ERROR;
#endif

  uwValue=(g_AfeRegs.R29_76.VCELLS[cellMask].VCELL_H<<8)|g_AfeRegs.R29_76.VCELLS[cellMask].VCELL_L;

  if (g_AfeRegs.R109.CVS)
  { // 电池电压以有符号数显示，LSB=200μV
    fValue = _lsbVCELL_signed * (int16_t)uwValue;
  }
  else
  { // 电池电压以无符号数显示，LSB=100μV;
    fValue = _lsbVCELL * (uint16_t)uwValue;
  }

  fValue=CellVolSecondaryCalibrate(fValue,cellIndex);
  return fValue*1000+0.5f;
}

uint16_t DVC1124_Calc_VBAT(void)
{
  uint16_t uwValue = (g_AfeRegs.R7_8.VBAT_H << 8) | g_AfeRegs.R7_8.VBAT_L;
  float fValue = _lsbVCELL * 100 * uwValue * 12.8f;
  return (uint16_t)(fValue + 0.5f);
}

uint32_t DVC1124_Calc_VPACK(void)
{
  uint16_t uwValue = (g_AfeRegs.R9_10.VPK_H << 8) | g_AfeRegs.R9_10.VPK_L;
  float fValue = _lsbVCELL * 100 * uwValue * 12.8f;
  return (uint32_t)(fValue + 0.5f);
}

float DVC1124_Calc_CurrentWithCC1(float senseResistance_mR)
{
  int16_t swValue = (g_AfeRegs.R2_3.CC1_H << 8) | g_AfeRegs.R2_3.CC1_L;
  return swValue * _lsbCC1 * 1000 / senseResistance_mR;
}

float DVC1124_Calc_CurrentWithCC2(float senseResistance_mR)
{
  s32 iValue = ((g_AfeRegs.R4_6.CC2_H << 12) | (g_AfeRegs.R4_6.CC2_M << 4) | g_AfeRegs.R4_6.CC2_L);
  if (iValue & 0x80000)
    iValue |= 0xFFF00000;
  return iValue * _lsbCC2 * 1000 / senseResistance_mR;
}

void Voltage_Acquisition(void)
{
  uint8_t i, j, k;
  int8_t cellIndex;
  int cellVoltage;
  uint32_t SumVol = 0;

#if (BMS_MODEL == MODEL_17S100AQ)
  /* 17S: Special cell mapping for cells 10-17 */
  uint8_t mapping[7] = {11, 13, 15, 17, 19, 21, 23};
  uint16_t tempVolt[AFE_MAX_CELL_CNT] = {0};

  if (DVC1124_ReadRegs(AFE_ADDR_R(29), AFE_MAX_CELL_CNT * 2))
  {
    for (i = 0; i < AFE_MAX_CELL_CNT; i++)
    {
      tempVolt[i] = DVC1124_Calc_VCell(i);
    }

    for (j = 0; j < g_tBatPackCtrl.tEESpec.byPackCell; j++)
    {
      if (j < 10)
      {
        g_tBatPackInfo.tCell.wVolt[j] = tempVolt[j];
      }
      else
      {
        g_tBatPackInfo.tCell.wVolt[j] = tempVolt[mapping[j - 10]];
      }
      SumVol += g_tBatPackInfo.tCell.wVolt[j];
    }

    for (k = g_tBatPackCtrl.tEESpec.byPackCell; k < N_PACK_CELL; k++)
    {
      g_tBatPackInfo.tCell.wVolt[k] = 0;
    }
#else
  /* 24S: Standard direct reading */
  if (DVC1124_ReadRegs(AFE_ADDR_R(29), g_tBatPackCtrl.tEESpec.byPackCell * 2))
  {
    for (i = 0; i < g_tBatPackCtrl.tEESpec.byPackCell; i++)
    {
      g_tBatPackInfo.tCell.wVolt[i] = DVC1124_Calc_VCell(i);
      SumVol += g_tBatPackInfo.tCell.wVolt[i];
    }

    for (j = g_tBatPackCtrl.tEESpec.byPackCell; j < N_PACK_CELL; j++)
    {
      g_tBatPackInfo.tCell.wVolt[j] = 0;
    }
#endif

    g_tBatPackInfo.tCell.wVolMin = g_tBatPackInfo.tCell.wVolMax = g_tBatPackInfo.tCell.wVolt[0];
    for (cellIndex = g_tBatPackCtrl.tEESpec.byPackCell - 1; cellIndex >= 0; cellIndex--)
    {
      cellVoltage = g_tBatPackInfo.tCell.wVolt[cellIndex];
      if (cellVoltage > g_tBatPackInfo.tCell.wVolMax)
      {
        g_tBatPackInfo.tCell.wVolMax = cellVoltage;
        if (g_tBatPackInfo.tCell.wVolMin == 0)
          g_tBatPackInfo.tCell.wVolMin = g_tBatPackInfo.tCell.wVolMax;
      }
      else if (cellVoltage < g_tBatPackInfo.tCell.wVolMin && cellVoltage > 0)
      {
        g_tBatPackInfo.tCell.wVolMin = cellVoltage;
      }
    }
  }

  g_tBatPackInfo.wBatVolt = SumVol / 100;
  g_tBatPackInfo.wCapVolt = g_tBatPackInfo.wBatVolt;
  g_tBatPackInfo.tCell.wVolMean = SumVol / g_tBatPackCtrl.tEESpec.byPackCell;
}

float ActualCurrent(float fCurrent)
{
  float cal_current;
  cal_current = fCurrent * 10.0f - g_tBatPackCtrl.tEESpec.wBatCurOffset - g_wBatCurDrift;
  if (cal_current < 0.0f)
  {
    cal_current = cal_current * g_tBatPackCtrl.tEESpec.wCapVoltOffset / 4096.0f;
  }
  else
  {
    cal_current = cal_current * g_tBatPackCtrl.tEESpec.wBatVoltOffset / 4096.0f;
  }
  return cal_current / 10.0f;
}

void Current_Acquisition(void)
{
  float fCurrent;
  if (DVC1124_ReadRegs(AFE_ADDR_R(2), 6))
  {
    fCurrent = DVC1124_Calc_CurrentWithCC2(CurrentSenseResistance_mR);
    g_fRawCur = fCurrent;
    g_fSampleCur = ActualCurrent(fCurrent);

    bDSGING = 0;
    bCHGING = 0;

    if (eBasicDiag==g_tBatPackCtrl.eDiagMode)
    {
      g_tBatPackInfo.eCtrlMode = eBmDiag;
    }
		else if(eProDiag==g_tBatPackCtrl.eDiagMode)
		{
			g_tBatPackInfo.eCtrlMode = eBmPowerOff;
		}
    else
    {
      if (g_fSampleCur > 0.4)
      {
        bDSGING = 1;
        g_tBatPackInfo.eCtrlMode = eBmQdsg;
      }
      else if (g_fSampleCur < -0.4)
      {
        bCHGING = 1;
        g_tBatPackInfo.eCtrlMode = eBmQchg;
      }
      else
      {
        g_tBatPackInfo.eCtrlMode = eBmWaiting;
        if (pro_flg.ProByte != 0 || waitpro != 0)
        {
          g_tBatPackInfo.eCtrlMode = eBmErr;
        }
      }
    }

    g_tBatPackInfo.wCur = g_fSampleCur * 10;
    g_tBatPackInfo.wAbsCur = ABS(g_tBatPackInfo.wCur);
    CurDriftCalib_Process();
  }
}

u8 bOTC;
static u32 uiBalMaskFlags = 0, uiBalMaskFlags_Prepared = 0;

void OverTempProtect(u8 temp)
{
  float t;
  u16 VCT;
  if (DVC1124_ReadRegs(AFE_ADDR_R(13), 2))
  {
    VCT = (g_AfeRegs.R13_14.VCT_H << 8) | g_AfeRegs.R13_14.VCT_L;
    t = VCT * 0.24467f - 271.3f;
  }

  if (t >= temp)
    bOTC = 1;
  else
    bOTC = 0;
}

void Balance_Contrl(u32 vlaue)
{
  if (vlaue < 0xffffff)
  {
    g_AfeRegs.R103_R105.CB[0] = vlaue >> 16;
    g_AfeRegs.R103_R105.CB[1] = vlaue >> 8;
    g_AfeRegs.R103_R105.CB[2] = vlaue & 0xff;
    DVC1124_WriteRegs(AFE_ADDR_R(103), 3);
  }
}

void BalanceProcess(void)
{
  u32 newBals = 0;
#if (BMS_MODEL == MODEL_17S100AQ)
  u32 lsBlan = 0;
#endif
  u8 i;
  if (DVC1124_ReadRegs(AFE_ADDR_R(103), 3))
  {
    uiBalMaskFlags = (g_AfeRegs.R103_R105.CB[0] << 16) + (g_AfeRegs.R103_R105.CB[1] << 8) + g_AfeRegs.R103_R105.CB[2];
  }
  if (!bOTC && (g_tBatPackInfo.tCell.wVolMax > g_tBatPackCtrl.tEESpec.wBalanceVolt) && (g_tBatPackInfo.tCell.wVolMax - g_tBatPackInfo.tCell.wVolMin) >= g_tBatPackCtrl.tEESpec.wBalanceDiff)
  {
    for (i = 0; i < g_tBatPackCtrl.tEESpec.byPackCell; i++)
    {
      if ((g_tBatPackInfo.tCell.wVolt[i] - g_tBatPackInfo.tCell.wVolMin) >= g_tBatPackCtrl.tEESpec.wBalanceDiff)
      {
#if (BMS_MODEL == MODEL_17S100AQ)
        if (i < 10)
        {
          newBals |= (1 << i);
        }
        else
        {
          newBals |= (1 << (2*i - 9));
        }
        lsBlan |= (1 << i);
#else
        newBals |= (1 << i);
#endif
      }
    }
    g_tBatPackInfo.tBmDiagVal.tBits.u2BalanceFlg = 1;
  }
  else
  {
    g_tBatPackInfo.tBmDiagVal.tBits.u2BalanceFlg = 0;
  }

#if (BMS_MODEL == MODEL_17S100AQ)
  g_tBatPackInfo.tCell.lstBlance[0] = lsBlan & 0xff;
  g_tBatPackInfo.tCell.lstBlance[1] = lsBlan >> 8;
  g_tBatPackInfo.tCell.lstBlance[2] = lsBlan >> 16;
#else
  g_tBatPackInfo.tCell.lstBlance[0] = newBals & 0xff;
  g_tBatPackInfo.tCell.lstBlance[1] = newBals >> 8;
  g_tBatPackInfo.tCell.lstBlance[2] = newBals >> 16;
#endif

  if (newBals != uiBalMaskFlags)
  {
    if (newBals == uiBalMaskFlags_Prepared)
    {
      uiBalMaskFlags = uiBalMaskFlags_Prepared;
      Balance_Contrl(uiBalMaskFlags);
    }
    else
      uiBalMaskFlags_Prepared = newBals;
  }
  else
    uiBalMaskFlags_Prepared = uiBalMaskFlags;
}

void Blance_Cell(void)
{
  OverTempProtect(100);
  BalanceProcess();
}

void COW_Ctrl(FunctionalState Cmd)
{
  g_AfeRegs.R109.COW = Cmd;
  DVC1124_WriteRegs(AFE_ADDR_R(109), 1);
}

void BrokenDetect(void)
{
  uint8_t i,j,k;
  static bool num_ctr = true;
  bool broken = false;
  int uw_cellvotage[24] = {0};
  if (DVC1124_ReadRegs(AFE_ADDR_R(109), 1))
  {
    if (g_AfeRegs.R109.COW == 0)
    {
      COW_Ctrl(ENABLE);
      CleanError();
      if (DVC1124_ReadRegs(AFE_ADDR_R(29), g_tBatPackCtrl.tEESpec.byPackCell * 2))
        COW_Ctrl(DISABLE);
      for (i = 0; i < g_tBatPackCtrl.tEESpec.byPackCell; i++)
      {
        uw_cellvotage[i] = DVC1124_Calc_VCell(i);
        if (uw_cellvotage[i] == 0)
        {
          broken = true;
        }
      }
    }
  }

  if (broken)
  {
    waitpro |= (0x01 << waitpro_Vol);
  }
  else
  {
    waitpro &= ~(0x01 << waitpro_Vol);
  }

  if (uw_cellvotage[g_tBatPackCtrl.tEESpec.byPackCell - 1] != 0 || (uw_cellvotage[g_tBatPackCtrl.tEESpec.byPackCell - 1] == 0 && uw_cellvotage[g_tBatPackCtrl.tEESpec.byPackCell - 2] != 0 ))
  {
    if (num_ctr && (g_tBatPackCtrl.tEESpec.wDiagFltSet & (1<<eDiagPowerCmd)))
    {
      bool allCellPresent = true;
      for (j = 0; j < g_tBatPackCtrl.tEESpec.byPackCell; j++)
      {
        if (uw_cellvotage[j] == 0)
        {
          allCellPresent = false;
          bool AllZero = true;
          for (k = j; k < g_tBatPackCtrl.tEESpec.byPackCell - 1; k++)
          {
            if (uw_cellvotage[k] != 0)
            {
              AllZero = false;
              break;
            }
          }
          if (AllZero)
          {
            num_ctr = false;
            g_bCellCountDone = true;
            g_tBatPackCtrl.tEESpec.byPackCell = j;
            AFEParam_Config();
            pro_flg.ProByte = 0;
          }
          break;
        }
      }
      if (allCellPresent)
      {
        g_bCellCountDone = true;
      }
    }
		if(!(g_tBatPackCtrl.tEESpec.wDiagFltSet & (1<<eDiagPowerCmd)))
			g_bCellCountDone= true;
		if(g_bCellCountDone)
		{
			Voltage_Acquisition();
    g_tFaultDataParam.tBattVolt[0].wUpperVal = g_tFaultDataParam.tCellVolt[0].wUpperVal*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell/100u;
    g_tFaultDataParam.tBattVolt[0].wLowerVal = g_tFaultDataParam.tCellVolt[0].wLowerVal*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell/100u;
    g_tFaultDataParam.tBattVolt[1].wUpperVal = g_tFaultDataParam.tCellVolt[1].wUpperVal*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell/100u;
    g_tFaultDataParam.tBattVolt[1].wLowerVal = g_tFaultDataParam.tCellVolt[1].wLowerVal*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell/100u;
    g_tFaultDataParam.tBattVolt[2].wUpperVal = g_tFaultDataParam.tCellVolt[2].wUpperVal*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell/100u;
      if(g_tFaultDataParam.tBattVolt[2].wUpperVal>MAX_VOLT_BATT_HIGH_UPPER)
      g_tFaultDataParam.tBattVolt[2].wUpperVal=MAX_VOLT_BATT_HIGH_UPPER;
      g_tFaultDataParam.tBattVolt[2].wLowerVal = g_tFaultDataParam.tCellVolt[2].wLowerVal*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell/100u;
      if(g_tFaultDataParam.tBattVolt[2].wLowerVal>MAX_VOLT_BATT_HIGH_LOWER)
      g_tFaultDataParam.tBattVolt[2].wLowerVal=MAX_VOLT_BATT_HIGH_LOWER;
      g_tFaultDataParam.tBattVolt[3].wUpperVal = g_tFaultDataParam.tCellVolt[3].wUpperVal*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell/100u;
      g_tFaultDataParam.tBattVolt[3].wLowerVal = g_tFaultDataParam.tCellVolt[3].wLowerVal*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell/100u;
      g_tFaultDataParam.tBattVolt[4].wUpperVal = g_tFaultDataParam.tCellVolt[4].wUpperVal*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell/100u;
      g_tFaultDataParam.tBattVolt[4].wLowerVal = g_tFaultDataParam.tCellVolt[4].wLowerVal*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell/100u;
      g_tFaultDataParam.tBattVolt[5].wUpperVal = g_tFaultDataParam.tCellVolt[5].wUpperVal*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell/100u;
      g_tFaultDataParam.tBattVolt[5].wLowerVal = g_tFaultDataParam.tCellVolt[5].wLowerVal*g_tBatPackCtrl.tEESpec.byLecuNum*g_tBatPackCtrl.tEESpec.byPackCell/100u;
		}
  }
}
void Heating_Ctr(void)
{
  if ((bCHGING || bDSGING) && g_tBatPackInfo.tCell.byTempMin <= TEMP_CELL_HOTCHG_LOW_UPPER_III ) //充电状态低于-5度开启
  {
    GPIO_SetBits(HEAT_PORT, HEAT_HTCTRL_PIN);
    g_tDiagnosticMsg.uRelaySt.tBits.bBm1 = 1;
  }
  if (pro_flg.tBits.Uv_Flg || (g_tBatPackInfo.tCell.byTempMin >= TEMP_CELL_HOTCHG_LOW_LOWER_III && !(waitpro &(0x01 << waitpro_temp)))) //放电截止或者高于10度关闭 
  {
    GPIO_ResetBits(HEAT_PORT, HEAT_HTCTRL_PIN);
    g_tDiagnosticMsg.uRelaySt.tBits.bBm1 = 0;
    if (GPIO_ReadInputDataBit(HTDET_PORT,HTDET_PIN))
    {
      GPIO_SetBits(HEAT_PORT, HEAT_FSEN_PIN);
    }
  }
}


void AFE_Handle(void)
{
  static uint16_t Brokentime = 0;
  static uint8_t num = 0;
	if(shallow_sleeping||deep_sleeping) return;
  if (++num >= 5)
  {
    num = 0;
    Voltage_Acquisition();
    Current_Acquisition();
    Temperature_Handle();
    Protection_handle();
    if(g_tBatPackCtrl.tEESpec.wDiagFltSet & (1<<eDiagBalance))
    {
      Blance_Cell();
    }
    Fet_Status();
  }

  if (++Brokentime >= 250)
  {
    Brokentime = 0;
    BrokenDetect();
    if (g_tBatPackCtrl.tEESpec.wDiagFltSet & (1<<eDiagHotComm) && g_tBatPackInfo.eCtrlMode != eBmDiag)
    {
      Heating_Ctr();
    }

    if (updata_ble)
    {
      updata_ble = false;
      NVIC_SystemReset();
    }
  }
}

/**
 * @brief  读取AFE唤醒原因
 * @retval AFE R0故障标志寄存器值，bit对应：SCD/OCC2/OCD2/OCC1/OCD1/CUV/COV/IWF
 *         0表示无故障（电流超过CWT阈值唤醒）
 * @note   同时读取R1获取PD(电池包检测)状态，存入g_byAfeWakeStatus
 */
uint8_t ReadAfeWakeReason(void)
{
    if (DVC1124_ReadRegs(AFE_ADDR_R(1), 1))
    { 
        return g_AfeRegs.R1.CST;         // 返回故障标志
    }
    return 0;
}