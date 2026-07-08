#include"Protection.h"
#include "User_Can_Config.h"

uint16_t	usUVDelayCnt = 0;			// 单节UV恢复延时计数
uint16_t	usUVRDelayCnt = 0;			// 单节UV恢复延时计数
uint16_t	usOVRDelayCnt = 0;			// 单节OV恢复延时计数

uint16_t	usUTCDelayCnt = 0;     //充电低温保护延时计时
uint16_t	usUTCRDelayCnt = 0;     //充电低温恢复延时计时
uint16_t	usOTCDelayCnt = 0;     //充电高温保护延时计时
uint16_t	usOTCRDelayCnt = 0;     //充电高温恢复延时计时

uint16_t	usUTDDelayCnt = 0;     //放电低温保护延时计时
uint16_t	usUTDRDelayCnt = 0;     //放电低温恢复延时计时
uint16_t	usOTDDelayCnt = 0;     //放电高温保护延时计时
uint16_t	usOTDRDelayCnt = 0;     //放电高温恢复延时计时

uint16_t	usOCC1DelayCnt = 0;			// OCC1延时计数
uint16_t	usOCD1DelayCnt = 0;			// OCD1延时计数
uint16_t	usOCC1RDelayCnt = 0;			// OCC1恢复延时计数
uint16_t	usOCD1RDelayCnt = 0;			// OCD1恢复延时计数
uint16_t	usOCC2RDelayCnt = 0;			// OCC2恢复延时计数
uint16_t	usOCD2RDelayCnt = 0;			// OCD2恢复延时计数

uint8_t waitpro = 0;

static ScdRecoveryState g_eScdState = SCD_STATE_IDLE;
static uint64_t g_u64ScdTimer = 0;
static uint8_t  g_byScdRetryCount = 0;
bool g_bScdRecoveryActive = false;

bool CleanFlg(AFE_ProtectType AFE_Protect)
{

  switch (AFE_Protect)
  {
  case AFE_FLAG_UV:
    g_AfeRegs.R0.bitmap.CUV = 0;
    break;
      case AFE_FLAG_OV:
     g_AfeRegs.R0.bitmap.COV = 0;
    break;
      case AFE_FLAG_OCD1:
     g_AfeRegs.R0.bitmap.OCD1 = 0;
    break;
      case AFE_FLAG_OCD2:
  g_AfeRegs.R0.bitmap.OCD2 = 0;
    break;
      case AFE_FLAG_OCC1:
  g_AfeRegs.R0.bitmap.OCC1 = 0;
    break; 
          case AFE_FLAG_OCC2:
  g_AfeRegs.R0.bitmap.OCC2 = 0;
    break;
      case AFE_FLAG_SCD:
  g_AfeRegs.R0.bitmap.SCD = 0;
    break;
  default:
    break;
  }
	while(!DVC1124_WriteRegs(AFE_ADDR_R(0),1))
  {
      return ERROR;
  }
  return SUCCESS;
}

bool CellNum_Config(uint8_t num)
{
#if (BMS_MODEL == MODEL_17S100AQ)
  /* 17S: Use lookup table for cells 14-17 */
  uint8_t cm_l_table[] = {0x55, 0xD5, 0xF5, 0xFD, 0x54, 0xD4, 0xF4, 0xFC, 0xFE};

  if (num >= 4 && num <= 17)
  {
    if (num >= 14 && num <= 17)
    {
      g_AfeRegs.R106.CM_H = cm_l_table[17 - num];
      g_AfeRegs.R107.CM_M = 0x54;
      g_AfeRegs.R108.CM_L = 0;
    }
    else if (num >= 9 && num <= 13)
    {
      g_AfeRegs.R106.CM_H = 0xFF;
      g_AfeRegs.R107.CM_M = cm_l_table[17 - num];
      g_AfeRegs.R108.CM_L = 0;
    }
    else if (num >= 5 && num <= 8)
    {
      g_AfeRegs.R106.CM_H = 0xFF;
      g_AfeRegs.R107.CM_M = 0xFF;
      g_AfeRegs.R108.CM_L = (1 << 8) - (1 << num);
    }
    return DVC1124_WriteRegs(AFE_ADDR_R(106), 3);
  }
  else
    return ERROR;
#else
  /* 24S: Use direct bit calculation */
  if (num >= 4 && num <= 24)
  {
    if (num >= 17 && num <= 24)
    {
      g_AfeRegs.R106.CM_H = (1 << 8) - (1 << (num - 16));
      g_AfeRegs.R107.CM_M = 0;
      g_AfeRegs.R108.CM_L = 0;
    }
    else if (num >= 9 && num <= 16)
    {
      g_AfeRegs.R106.CM_H = 0xFF;
      g_AfeRegs.R107.CM_M = (1 << 8) - (1 << (num - 8));
      g_AfeRegs.R108.CM_L = 0;
    }
    else if (num >= 4 && num <= 8)
    {
      g_AfeRegs.R106.CM_H = 0xFF;
      g_AfeRegs.R107.CM_M = 0xFF;
      g_AfeRegs.R108.CM_L = (1 << 4) - (1 << (num - 4));
    }
    return DVC1124_WriteRegs(AFE_ADDR_R(106), 3);
  }
  else
    return ERROR;
#endif
}

/**
	* @说明	一级放电过流配置
	* @参数	u16 阈值 250-63750uv 步进 250uv
					u16 DLY  8-2040ms 步进 8ms
	* @返回值	成功/失败
	* @注	
*/
float ReverseCurrent(u16 nfCurrent)
{
    float cal_current;
    cal_current  = ((uint32_t)nfCurrent<<12) / g_tBatPackCtrl.tEESpec.wCapVoltOffset;
    cal_current += (g_tBatPackCtrl.tEESpec.wBatCurOffset + g_wBatCurDrift);
    return cal_current;
}

bool OCD1_Config(u16 TH,u16 DLY){
uint32_t u32TH;
if (TH == 0)
{
  u32TH = 0;
}
else{
  u32TH  =(uint32_t)ReverseCurrent(TH) * CurrentSenseResistance_mR * 100 - CURoffset; 
}

if(u32TH>63750)
    TH =  63750;
else
    TH =  u32TH;
if(TH>=250&&TH<=63750&&DLY>=8&&DLY<=2040){//未超量程
g_AfeRegs.R89.OCD1T=TH/250;
g_AfeRegs.R91.OCD1D=DLY/8-1;
return DVC1124_WriteRegs(AFE_ADDR_R(89),1)&DVC1124_WriteRegs(AFE_ADDR_R(91),1);
}
else if(TH==0){
g_AfeRegs.R89.OCD1T=0;
return DVC1124_WriteRegs(AFE_ADDR_R(89),1);
}
else
return ERROR;
}
/**
	* @说明	二级放电过流配置
	* @参数	u16 TH 4-256mv 步进4mv
					u16 DLY 4-1020ms 步进4ms
					bool enable enable/disable
	* @返回值	成功/失败
	* @注	
*/
bool OCD2_Config(u16 TH,u16 DLY,bool enable){//
    
TH = (u16)ReverseCurrent(TH) * CurrentSenseResistance_mR/10;
if(TH>256)//最大值
       TH=256; 
if(!enable){//关闭OCD2
	g_AfeRegs.R94.OCD2E=0;
	return DVC1124_WriteRegs(AFE_ADDR_R(94),1);
}
else{
	if(TH<=256&&DLY<=1020){//未超量程
	g_AfeRegs.R94.OCD2T=TH/4-1;
	g_AfeRegs.R96.OCD2D=DLY/4-1;
	g_AfeRegs.R94.OCD2E=enable;
	return DVC1124_WriteRegs(AFE_ADDR_R(94),1)&DVC1124_WriteRegs(AFE_ADDR_R(96),1);
	}
	else
	return ERROR;
}
}
/**
	* @说明	一级充电过流配置
	* @参数	u16 阈值 250-63750uv 步进 250uv
					u16 DLY  8-2040ms 步进 8ms
	* @返回值	成功/失败
	* @注	
*/
bool OCC1_Config(u16 TH,u16 DLY){
 uint32_t u32TH;
 if (TH == 0)
{
  u32TH = 0;
}
else{
  u32TH  =(u16)ReverseCurrent(TH) * CurrentSenseResistance_mR * 100 - CURoffset; 
}
if(u32TH>63750)
    TH =  63750;
else
    TH =  u32TH;
if(TH>=250&&TH<=63750&&DLY>=8&&DLY<=2040){//未超量程
g_AfeRegs.R90.OCC1T=TH/250;
g_AfeRegs.R92.OCC1D=DLY/8-1;
return DVC1124_WriteRegs(AFE_ADDR_R(90),1)&DVC1124_WriteRegs(AFE_ADDR_R(92),1);
}
else if(TH==0)//关闭
{
g_AfeRegs.R90.OCC1T=0;
return DVC1124_WriteRegs(AFE_ADDR_R(90),1);
}
else
return ERROR;
}
/**
	* @说明	二级充电过流配置
	* @参数	u16 TH 4-128mv 步进4mv
					u16 DLY 4-1020ms 步进4ms
					bool enable enable/disable
	* @返回值	成功/失败
	* @注	
*/
bool OCC2_Config(u16 TH,u16 DLY,bool enable){//
    TH = (u16)ReverseCurrent(TH) * CurrentSenseResistance_mR/10;
    if(TH>128)//最大值
       TH=128; 
if(!enable){//关闭OCC2
	g_AfeRegs.R95.OCC2E=0;
	return DVC1124_WriteRegs(AFE_ADDR_R(95),1);
}
else{
	if(TH<=128&&DLY<=1020){//未超量程
	g_AfeRegs.R95.OCC2T=TH/4-1;
	g_AfeRegs.R97.OCC2D=DLY/4-1;
	g_AfeRegs.R95.OCC2E=enable;
	return DVC1124_WriteRegs(AFE_ADDR_R(95),1)&DVC1124_WriteRegs(AFE_ADDR_R(97),1);
	}
	else
	return ERROR;
}
}

/**
	* @说明	过压配置
	* @参数	u16 TH 501-4595mv 步进1mv
					u16 DLY 200-8000ms 200-1000步进100, 1000-8000步进1000
	* @返回值	成功/失败
	* @注	
*/
bool OV_Config(u16 TH,u16 DLY,bool enable){
	if(!enable)
	{
		g_AfeRegs.R112.COVT_H=0;
		g_AfeRegs.R113.COVT_L=0;
		return DVC1124_WriteRegs(AFE_ADDR_R(112),2);
	}
	else
	{
	if(TH>500&&TH<=4595&&DLY>=200&&DLY<=8000){//未超量程
		TH=TH-500;
		if(DLY>1000)//1s以上步进1s
		g_AfeRegs.R113.COVD=DLY/1000+7;	
		else
		g_AfeRegs.R113.COVD=DLY/100-2;	//1s以下步进100ms	
	g_AfeRegs.R112.COVT_H=TH>>4;//u12 高8位
	g_AfeRegs.R113.COVT_L=TH&0x0f;//u12 低4位
	return DVC1124_WriteRegs(AFE_ADDR_R(112),2);
	}
	else
	return ERROR;
	}
}
/**
	* @说明	欠压配置
	* @参数	u16 TH 1-4095mv 步进1mv
					u16 DLY 200-8000ms 200-1000步进100, 1000-8000步进1000
	* @返回值	成功/失败
	* @注	
*/
bool UV_Config(u16 TH,u16 DLY,bool enable){
	if(!enable)
	{
		g_AfeRegs.R114.CUVT_H=0;
		g_AfeRegs.R115.CUVT_L=0;
		return DVC1124_WriteRegs(AFE_ADDR_R(114),2);
	}
	else{
	if(TH>=1&&TH<=4095&&DLY>=200&&DLY<=8000){//未超量程
		if(DLY>1000)//1s以上步进1s
		g_AfeRegs.R115.CUVD=DLY/1000+7;	
		else
		g_AfeRegs.R115.CUVD=DLY/100-2;//1s以下步进100ms	
		g_AfeRegs.R114.CUVT_H=TH>>4;//u12 高8位
		g_AfeRegs.R115.CUVT_L=TH&0x0f;//u12 低4位
		return DVC1124_WriteRegs(AFE_ADDR_R(114),2);
	}
	else
	return ERROR;
	}

}
/**
	* @说明	放电短路配置
	* @参数	u16 阈值 10-640mv 步进 10mv
					float DLY  0-1992us 步进 7.81us
	* @返回值	成功/失败
	* @注	
*/
bool SCD_Config(u16 TH,float DLY,bool enable){
    
    TH = (u16)ReverseCurrent(TH) * CurrentSenseResistance_mR/10;    
    if(TH>310)//最大值
       TH=310; 
	if(!enable){//关闭SCD
		g_AfeRegs.R98.SCDE=0;
		return DVC1124_WriteRegs(AFE_ADDR_R(98),1);
	}
	else{
		if(TH>=10&&TH<=310&&DLY<=1992){//未超量程
		g_AfeRegs.R98.SCDT=TH/10;
		g_AfeRegs.R99.SCDD=DLY/7.81f;
		g_AfeRegs.R98.SCDE=enable;
		return DVC1124_WriteRegs(AFE_ADDR_R(98),2);
		}
		else
		return ERROR;
}

}

/**
	* @说明	功能参数配置
	* @参数	
	* @返回值	
	* @注	
*/
bool AFEParam_Config(void){
  bool Result = SUCCESS;

  if (CellNum_Config(g_tBatPackCtrl.tEESpec.byPackCell) == ERROR) //串数
    Result = ERROR;
  if (OCD1_Config(0, 2000) == ERROR) //  一级放电过流关闭，使用软件方式
    Result = ERROR;
  if (OCC1_Config(0, 2000) == ERROR) //一级充电过流关闭，使用软件方式
    Result = ERROR;
  if (OV_Config(g_tFaultDataParam.tCellVolt[2].wUpperVal>g_tBatPackCtrl.tEESpec.wCellVolMax?g_tBatPackCtrl.tEESpec.wCellVolMax:g_tFaultDataParam.tCellVolt[2].wUpperVal, OV_DELAY_CNT, ENABLE) == ERROR) // 3750mv 2000ms 过压
    Result = ERROR;
  if (UV_Config(g_tFaultDataParam.tCellVolt[5].wUpperVal<g_tBatPackCtrl.tEESpec.wCellVolMin?g_tBatPackCtrl.tEESpec.wCellVolMin:g_tFaultDataParam.tCellVolt[5].wUpperVal, UV_DELAY_CNT, ENABLE) == ERROR) // 2300mv 2000ms   欠压
    Result = ERROR;
  if (SCD_Config(2*g_tBatPackCtrl.tEESpec.wPackDischgMax>2*PACK_DSG_MAX_CUR?2*PACK_DSG_MAX_CUR:2*g_tBatPackCtrl.tEESpec.wPackDischgMax, 12, ENABLE) == ERROR) // 150mv 0ms ��路600A
    Result = ERROR;
  if (OCD2_Config(g_tBatPackCtrl.tEESpec.wPackDischgMax, 500, ENABLE) == ERROR) //  500ms 二级放电过流 220A
    Result = ERROR;
  if (OCC2_Config(g_tBatPackCtrl.tEESpec.wPackChgMax, 500, ENABLE) == ERROR) //  500ms 二级充电过流 220A
    Result = ERROR;
  CleanError();
   return Result;
}
/**
	* @˵��	�������ܲδ���
	* @����	
	* @����ֵ	
	* @ע	
*/
uProFlgVal pro_flg;
static bool s_bReChgInhibit = false;   // 复充禁止锁存：过压后禁充(掉电不保存)



/**
	* @说明	预充驱动开关
	* @参数	bool 开/关
	* @返回值	成功/失败
	* @注	
*/
bool PCHG_FETControl(bool abel){

	g_AfeRegs.R81.PCHGC=abel;

	return DVC1124_WriteRegs(AFE_ADDR_R(81),1);
}
/**
	* @说明	预放驱动开关
	* @参数	bool 开/关
	* @返回值	成功/失败
	* @注	
*/
bool PDSG_FETControl(bool abel){

	g_AfeRegs.R81.PDSGC=abel;

	return DVC1124_WriteRegs(AFE_ADDR_R(81),1);
}

/**
	* @说明	充电驱动开关
	* @参数	u8 模式
	* @返回值	成功/失败
	* @注	
*/
bool CHG_FETControl(u8 mode){

	if(mode==2)
	g_AfeRegs.R84.CBDM=0;

	g_AfeRegs.R81.CHGC=mode;

	return DVC1124_WriteRegs(AFE_ADDR_R(81),4);
}

/**
	* @说明	放电驱动开关
	* @参数	u8 模式
	* @返回值	成功/失败
	* @注	
*/
bool DSG_FETControl(u8 mode){

	if(mode==2)
	g_AfeRegs.R83.DBDM=0;

	g_AfeRegs.R81.DSGC=mode;

	return DVC1124_WriteRegs(AFE_ADDR_R(81),3);
}



bool dsg,chg,pdsg;
void Fet_Status(void)
{
  
	if (DVC1124_ReadRegs(AFE_ADDR_R(6), 1))
	{
		if (g_AfeRegs.R4_6.DSGF)
		{
			dsg = true;
		}
		else
		{
			dsg = false;
		}

		if (g_AfeRegs.R4_6.CHGF)
		{
			chg = true;
		}
		else
		{
      chg = false;
    }
    if (g_AfeRegs.R4_6.PDSGF)
    {
      pdsg = true;
    }
    else
    {

      pdsg = false;
    }
  }

  if (g_tBatPackInfo.tBmDiagVal.tBits.u1AfeCommFlg == 0)
  {
    g_tDiagnosticMsg.uRelaySt.tBits.bPos = chg;
    g_tDiagnosticMsg.uRelaySt.tBits.bNeg = dsg;
		g_tDiagnosticMsg.uRelaySt.tBits.bPre = pdsg;
  }
  
 

}

void MosCtrl(void)
{
	if(eBmDiag==g_tBatPackInfo.eCtrlMode) return;
	if(eBmPowerOff==g_tBatPackInfo.eCtrlMode)
  {
    CHG_FETControl(FET_Close);
    DSG_FETControl(FET_Close);
    PDSG_FETControl(false);
  }
  else
  {
  static uint64_t pdsg_time = 0;
  if (!g_bScdRecoveryActive)
  {
  if ((pro_flg.tBits.ShortFlg || pro_flg.tBits.Uv_Flg || pro_flg.tBits.Ocd1_Flg || pro_flg.tBits.Ocd2_Flg  || pro_flg.tBits.Utd_Flg || pro_flg.tBits.Otd_Flg
  || (waitpro &(0x01 << waitpro_temp)) || (waitpro & (0x01 << waitpro_Vol)))&&!bCHGING)
  {
    if (dsg)
    {
      dsg = 0;
      DSG_FETControl(FET_Close);
    }
    if (pdsg && GetTick_ms()  > pdsg_time + 500) //预放超时
    {
       pdsg = 0;
        PDSG_FETControl(false);
    }
  }
  else
  {
    if (!dsg)
    {
      
      if (!pdsg)
      {
        pdsg_time = GetTick_ms();
        pdsg = 1;
        PDSG_FETControl(true);
          }
          else if (GetTick_ms() > pdsg_time + 200)
      {
        pdsg = 0;
        PDSG_FETControl(false);
        dsg = 1;
        DSG_FETControl(FET_OPEN);
      }
        }else if(pdsg)
        {
          pdsg = 0;
          PDSG_FETControl(false);
    }
  }
 }
  if ((pro_flg.tBits.Ov_Flg || s_bReChgInhibit|| pro_flg.tBits.Occ1_Flg || pro_flg.tBits.Utc_Flg 
  || pro_flg.tBits.Otc_Flg || (waitpro &(0x01 << waitpro_temp)) || (waitpro & (0x01 << waitpro_Vol)))&&!bDSGING)
  {
    if (chg)
    {
      chg = 0;
      CHG_FETControl(FET_Close);
    }
  }
  else
  {
    if (!chg)
    {
      chg = 1;
      CHG_FETControl(FET_OPEN);
    }
  }
}
  
}

//  单节电池欠压保护检测
void ProtectUV(void)
{
	if(!pro_flg.tBits.Uv_Flg)
	{
        //低温欠压软件保护
     if(g_tBatPackInfo.tCell.wVolMin < g_tBatPackCtrl.tEESpec.wCellVolMin&& g_tBatPackInfo.tCell.byTempMin <  TEMP_BOUND_ALARM&&!bCHGING)
     {
        if (++usUVDelayCnt >= 20)
        {
          pro_flg.tBits.Uv_Flg = 1;
          usUVDelayCnt = 0;
          usUVRDelayCnt = 0;
        }
      }
      else
      {
        usUVRDelayCnt = 0;
      }  
	}
	else
	{
        if(bCHGING)//充电解除 ReadChgDet()||
        {
             if(CleanFlg(AFE_FLAG_UV))		// 清零AFE中的标志位
            {
                pro_flg.tBits.Uv_Flg = 0; 
                usUVRDelayCnt = 0;
            }
        }
		else if(g_tBatPackInfo.tCell.wVolMin > g_tFaultDataParam.tCellVolt[5].wLowerVal)				// UV恢复电压判定 
        {			
            if(++usUVRDelayCnt >= UVR_DELAY_CNT)				// UV恢复延时判定  
            {
                if(CleanFlg(AFE_FLAG_UV))		// 清零AFE中的标志位
                {
                    pro_flg.tBits.Uv_Flg = 0; 
                    usUVRDelayCnt = 0;
                }
            }
        }
        else
        {
            usUVRDelayCnt = 0;
        }
	}  
}

//单节电池过压保护检测
void ProtectOV(void)
{
	if(!pro_flg.tBits.Ov_Flg)
	{
		usOVRDelayCnt = 0;
	}
	else
	{
		if(bDSGING)//放电解除 ReadDsgDet()||
        {
             if(CleanFlg(AFE_FLAG_OV))		// 清零AFE中的标志位
            {
                pro_flg.tBits.Ov_Flg = 0; 
                usUVRDelayCnt = 0;
            }
        }
		else if(g_tBatPackInfo.tCell.wVolMax < g_tFaultDataParam.tCellVolt[2].wLowerVal)				// OV恢复电压判定 
        {			
            if(++usOVRDelayCnt >= OVR_DELAY_CNT)				// OV恢复延时判定  
            {
                if(CleanFlg(AFE_FLAG_OV))		// 清零AFE中的标志位
                {
                    pro_flg.tBits.Ov_Flg = 0; 
                    usOVRDelayCnt = 0;
                }
            }
        }
        else
        {
            usOVRDelayCnt = 0;
        }
	}  
}
void ProtectReChgInhibit(void)
{
	if (pro_flg.tBits.Ov_Flg)          // 出现过压即锁存禁止复充
	{
		s_bReChgInhibit = true;
	}
	else if (s_bReChgInhibit)          // OV 解除后才判定解除条件
	{
		if (g_tBatPackInfo.wCur > RECHG_DSGCUR_REL      // 放电>2A
			|| g_tBatPackInfo.wSocRef < RECHG_SOC_REL)  // 或 SOC<98%
		{
			s_bReChgInhibit = false;
        }
	}  
}

void ProtectOCC1(void)
{
	if(!pro_flg.tBits.Occ1_Flg)
	{
      if(g_tBatPackInfo.wAbsCur > g_tFaultDataParam.tBattCurr[2].fUpperVal * g_tBatPackCtrl.tEESpec.wBatCapacity && bCHGING)
     {
        if (++usOCC1DelayCnt >= OCC1_DELAY_CNT)
        {
          pro_flg.tBits.Occ1_Flg = 1;
          usOCC1DelayCnt = 0;
          usOCC1RDelayCnt = 0;
        }
      }
      else
      {
        usOCC1DelayCnt = 0;
      }  

	}
	else
	{

        if(bDSGING)//放电解除	ReadDsgDet()||
        {
             if(CleanFlg(AFE_FLAG_OCC1))		// 清零AFE中的标志位
                {
                    pro_flg.tBits.Occ1_Flg = 0; 
                    usOCC1RDelayCnt = 0;
                }
        }   
		else if(g_tBatPackInfo.wAbsCur < g_tFaultDataParam.tBattCurr[2].fLowerVal * g_tBatPackCtrl.tEESpec.wBatCapacity)			
        {			
            if(++usOCC1RDelayCnt >= OCC1R_DELAY_CNT)				
            {
                if(CleanFlg(AFE_FLAG_OCC1))		// 清零AFE中的标志位
                {
                    pro_flg.tBits.Occ1_Flg = 0; 
                    usOCC1RDelayCnt = 0;
                }
            }
        }
        else
        {
          usOCC1RDelayCnt = 0;
        }
	}  
}

void ProtectOCD1(void)
{
	if(!pro_flg.tBits.Ocd1_Flg)
	{     
      if((g_tBatPackInfo.wAbsCur > (g_tFaultDataParam.tBattCurr[5].fUpperVal * g_tBatPackCtrl.tEESpec.wBatCapacity)) && bDSGING)
      {
        if (++usOCD1DelayCnt >= OCD1_DELAY_CNT)
        {
          pro_flg.tBits.Ocd1_Flg = 1;
          usOCD1DelayCnt = 0;
          usOCD1RDelayCnt = 0;
        }
      }
      else
      {
        usOCD1DelayCnt = 0;
      }  
	}
	else
	{
  
        if(bCHGING)//充电解除 ReadChgDet()||
        {
             if(CleanFlg(AFE_FLAG_OCD1))		// 清零AFE中的标志位
            {
                pro_flg.tBits.Uv_Flg = 0; 
                usUVRDelayCnt = 0;
            }
        }
		else if(g_tBatPackInfo.wAbsCur < g_tFaultDataParam.tBattCurr[5].fLowerVal * g_tBatPackCtrl.tEESpec.wBatCapacity)			
        {			
            if(++usOCD1RDelayCnt >= OCD1R_DELAY_CNT)				
            {
                if(CleanFlg(AFE_FLAG_OCD1))		// 清零AFE中的标志位
                {
                    pro_flg.tBits.Ocd1_Flg = 0; 
                    usOCD1RDelayCnt = 0;
                }
            }
        }
        else
        {
          usOCD1RDelayCnt = 0;
        }
	}  
}

void ProtectOCC2(void)
{
 	if(!pro_flg.tBits.Occ2_Flg)
	{
		usOCC2RDelayCnt = 0;
	}
	else
	{
    if ( bDSGING ) //放电解除 ReadDsgDet() ||
    {
      if (CleanFlg(AFE_FLAG_OCC2)) // 清零AFE中的标志位
      {
        pro_flg.tBits.Occ2_Flg = 0;
        usOCC2RDelayCnt = 0;
      }
    }

     else if(!bCHGING)				// && !ReadChgDet()
        {			
            if(++usOCC2RDelayCnt >= OCC2R_DELAY_CNT)				
            {
                if(CleanFlg(AFE_FLAG_OCC2))		// 清零AFE中的标志位
                {
                    pro_flg.tBits.Occ2_Flg = 0; 
                    usOCC2RDelayCnt = 0;
                }
            }
        }
        else
        {
          usOCC2RDelayCnt = 0;
        }
	}  
}


void ProtectOCD2(void)
{
 	if(!pro_flg.tBits.Ocd2_Flg)
	{
		usOCD2RDelayCnt = 0;
	}
	else
	{
        if(bCHGING)//充电解除 ReadChgDet()||
        {
             if(CleanFlg(AFE_FLAG_OCD2))		// 清零AFE中的标志位
            {
                pro_flg.tBits.Uv_Flg = 0; 
                usUVRDelayCnt = 0;
            }
        }
		else if(!bDSGING )		//		&& !ReadDsgDet()
        {			
            if(++usOCD2RDelayCnt >= OCD2R_DELAY_CNT)				
            {
                if(CleanFlg(AFE_FLAG_OCD2))		// 清零AFE中的标志位
                {
                    pro_flg.tBits.Ocd2_Flg = 0; 
                    usOCD2RDelayCnt = 0;
                }
            }
        }
        else
        {
          usOCD2RDelayCnt = 0;
        }
	}  
}

//充电温度
void ProtectUTC_OTC(void)
{

    if (!pro_flg.tBits.Utc_Flg)
    {
      if (g_tBatPackInfo.tCell.byTempMin <  g_tFaultDataParam.tCellTemp[9].byUpperVal && bCHGING)
      {
        if (++usUTCDelayCnt >= UTC_DELAY_CNT)
        {
          pro_flg.tBits.Utc_Flg = 1;
          usUTCDelayCnt = 0;
          usUTCRDelayCnt = 0;
        }
      }
      else
      {
        usUTCDelayCnt = 0;
      }
    }
    else
    {
      if (g_tBatPackInfo.tCell.byTempMin > g_tFaultDataParam.tCellTemp[9].byLowerVal)
      {
        if (++usUTCRDelayCnt >= UTCR_DELAY_CNT)
        {
          pro_flg.tBits.Utc_Flg = 0;
          usUTCDelayCnt = 0;
          usUTCRDelayCnt = 0;
        }
      }
      else
      {
        usUTCRDelayCnt = 0;
      }
    }
    //OTC 充电高温
      if (!pro_flg.tBits.Otc_Flg)
    {
      if (g_tBatPackInfo.tCell.byTempMax > g_tFaultDataParam.tCellTemp[8].byUpperVal && !(waitpro &(0x01 << waitpro_temp))&& bCHGING)
      {
        if (++usOTCDelayCnt >= OTC_DELAY_CNT)
        {
          pro_flg.tBits.Otc_Flg = 1;
          usOTCDelayCnt = 0;
          usOTCRDelayCnt = 0;
        }
      }
      else
      {
        usOTCDelayCnt = 0;
      }
    }
    else
    {
      if (g_tBatPackInfo.tCell.byTempMax < g_tFaultDataParam.tCellTemp[8].byLowerVal)
      {
        if (++usOTCRDelayCnt >= OTCR_DELAY_CNT)
        {
          pro_flg.tBits.Otc_Flg = 0;
          usOTCDelayCnt = 0;
          usOTCRDelayCnt = 0;
        }
      }
      else
      {
        usOTCRDelayCnt = 0;
      }
    }
  }



//放电温度
void ProtectUTD_OTD(void)
{

    if (!pro_flg.tBits.Utd_Flg )
    {
      if (g_tBatPackInfo.tCell.byTempMin < g_tFaultDataParam.tCellTemp[5].byUpperVal && bDSGING)
      {
        if (++usUTDDelayCnt >= UTD_DELAY_CNT)
        {
          pro_flg.tBits.Utd_Flg = 1;
          usUTDDelayCnt = 0;
          usUTDRDelayCnt = 0;
        }
      }
      else
      {
        usUTDDelayCnt = 0;
      }
    }
    else
    {
      if (g_tBatPackInfo.tCell.byTempMin > g_tFaultDataParam.tCellTemp[5].byLowerVal)
      {
        if (++usUTDRDelayCnt >= UTDR_DELAY_CNT)
        {
          pro_flg.tBits.Utd_Flg = 0;
          usUTDDelayCnt = 0;
          usUTDRDelayCnt = 0;
        }
      }
      else
      {
        usUTDRDelayCnt = 0;
      }
    }
    //OTD 放电高温
      if (!pro_flg.tBits.Otd_Flg)
    {
      if (g_tBatPackInfo.tCell.byTempMax > (g_tFaultDataParam.tCellTemp[2].byUpperVal - 2) && !(waitpro &(0x01 << waitpro_temp)) && bDSGING)
      {
        if (++usOTDDelayCnt >= OTD_DELAY_CNT)
        {
          pro_flg.tBits.Otd_Flg = 1;
          usOTDDelayCnt = 0;
          usOTDRDelayCnt = 0;
        }
      }
      else
      {
        usOTDDelayCnt = 0;
      }
    }
    else
    {
      if (g_tBatPackInfo.tCell.byTempMax < g_tFaultDataParam.tCellTemp[2].byLowerVal)
      {
        if (++usOTDRDelayCnt >= OTDR_DELAY_CNT)
        {
          pro_flg.tBits.Otd_Flg = 0;
          usOTDDelayCnt = 0;
          usOTDRDelayCnt = 0;
        }
      }
      else
      {
        usOTDRDelayCnt = 0;
      }
    }
}


void ProtectWait(void)
{
  static uint8_t time = 0;
  if (++time >= 20) //1.4S
  {
    time = 0;
  if(g_tBatPackInfo.eCtrlMode > eBmPowerOn)
  {
    if ( g_tBatPackInfo.tCell.byTempMax == 0XFF || g_tBatPackInfo.byTemp1 == 0XFF ||  g_tBatPackInfo.byTemp1 > MOSTEMP_PRO) //温度
    {
      waitpro |= (0x01 << waitpro_temp);
    }else if(g_tBatPackInfo.byTemp1 < MOSTEMP_RE){

      waitpro &= ~(0x01 << waitpro_temp);
    }    
  }
	
  }
  
}

void DIAG_FaultLevelTranProcess(void)
{
   
    memset((uint8_t *)&g_tBatPackInfo.tBmFltVal, 0x00, sizeof(g_tBatPackInfo.tBmFltVal) / sizeof(uint8_t));      
    protect_sata = 0;

    g_tBatPackInfo.eFltLevel = eBmFltNone;
    if (g_AfeRegs.R0.bitmap.SCD)
    {
        g_tBatPackInfo.tBmFltVal.tBits.u1ShortFlg = 1;
        g_tBatPackInfo.eFltLevel = eBmFltLevel3;
    }
    else
    {
      pro_flg.tBits.ShortFlg = 0;
    }

    if (g_tBatPackInfo.wSocShow <= (g_tFaultDataParam.tBattSoc[0].byUpperVal*100))
    {
        g_tBatPackInfo.tBmFltVal.tBits.uSOCFlg = 1;
        g_tBatPackInfo.eFltLevel = eBmFltLevel1;
    }
    

    // 温度故障
    if (pro_flg.tBits.Utc_Flg || pro_flg.tBits.Otc_Flg || pro_flg.tBits.Utd_Flg || pro_flg.tBits.Otd_Flg || (waitpro &(0x01 << waitpro_temp)))
    {
        g_tBatPackInfo.tBmFltVal.tBits.u1TempFlg = 1;
     
        g_tBatPackInfo.eFltLevel = eBmFltLevel3;
       
    }

    // 放电故障

    if (pro_flg.tBits.Ocd1_Flg || pro_flg.tBits.Ocd2_Flg || pro_flg.tBits.Utd_Flg || pro_flg.tBits.Otd_Flg)
    {
        g_tBatPackInfo.tBmFltVal.tBits.u1DisChgFlg = 1;
      
        g_tBatPackInfo.eFltLevel = eBmFltLevel3;
        
        if (pro_flg.tBits.Ocd1_Flg || pro_flg.tBits.Ocd2_Flg)
        {
          protect_sata |= (1 << 5);
        }
        
      if (pro_flg.tBits.Utd_Flg)
      {
        	protect_sata |= (1 << 9);
      }
      if (pro_flg.tBits.Otd_Flg)
      {
        	protect_sata |= (1 << 8);
      }
      
    }

    // 充电故障
    if (pro_flg.tBits.Occ1_Flg || pro_flg.tBits.Occ2_Flg || pro_flg.tBits.Utc_Flg || pro_flg.tBits.Otc_Flg)
    {
        g_tBatPackInfo.tBmFltVal.tBits.u1ChgFlg = 1;
      
        g_tBatPackInfo.eFltLevel = eBmFltLevel3;

        if (pro_flg.tBits.Occ1_Flg || pro_flg.tBits.Occ2_Flg)
        {
          protect_sata |= (1 << 0);
        }
        if (pro_flg.tBits.Otc_Flg)
        {
          protect_sata |= (1 << 3);
        }
        
        if (pro_flg.tBits.Utc_Flg)
        {
          protect_sata |= (1 << 4);
        }
        
    }

      // 总电压故障
      if (g_tBatPackInfo.wBatVolt > g_tFaultDataParam.tBattVolt[2].wUpperVal || g_tBatPackInfo.wBatVolt < g_tFaultDataParam.tBattVolt[5].wUpperVal)
       {
           g_tBatPackInfo.tBmFltVal.tBits.u1BatFlg = 1;
           g_tBatPackInfo.eFltLevel = eBmFltLevel3;
       }

    // 单体电压故障

    if (pro_flg.tBits.Uv_Flg  || pro_flg.tBits.Ov_Flg || (waitpro &(0x01 << waitpro_Vol)))
    {
        g_tBatPackInfo.tBmFltVal.tBits.u1CellFlg = 1;
        g_tBatPackInfo.eFltLevel = eBmFltLevel3;
        if (pro_flg.tBits.Ov_Flg)
        {
          protect_sata |= (1 << 1);
		      protect_sata |= (1 << 2);
        }

        if (pro_flg.tBits.Uv_Flg )
        {
          protect_sata |= (1 << 6);
          protect_sata |= (1 << 7);
        }
    }

    ExecLogErrStRecord();

}

void ProtectSCD(void)
{
    DVC1124_ReadRegs(AFE_ADDR_R(0), 1);

    switch (g_eScdState)
    {
    case SCD_STATE_IDLE:
        if (pro_flg.tBits.ShortFlg)
        {
            CleanFlg(AFE_FLAG_SCD);
            g_byScdRetryCount = 0;
            g_u64ScdTimer = GetTick_ms();
            g_eScdState = SCD_STATE_COOLDOWN;
        }
        break;

    case SCD_STATE_COOLDOWN:
        if (GetTick_ms() < g_u64ScdTimer + SCD_COOLDOWN_MS)
            break;
        if (GPIO_ReadInputDataBit(WAKE_LOAD_PORT, WAKE_LOAD_PIN))
        {
            pro_flg.tBits.ShortFlg = 0;
            g_byScdRetryCount = 0;
            g_bScdRecoveryActive = false;
            g_eScdState = SCD_STATE_IDLE;
            break;
        }
        g_bScdRecoveryActive = true;
        PDSG_FETControl(true);
        g_u64ScdTimer = GetTick_ms();
        g_eScdState = SCD_STATE_PDSG_CHECK;
        break;

    case SCD_STATE_PDSG_CHECK:
        if (g_AfeRegs.R0.bitmap.SCD)
        {
            PDSG_FETControl(false);
            g_bScdRecoveryActive = false;
            g_byScdRetryCount++;
            CleanFlg(AFE_FLAG_SCD);
            if (g_byScdRetryCount < SCD_MAX_RETRIES)
            {
                g_u64ScdTimer = GetTick_ms();
                g_eScdState = SCD_STATE_COOLDOWN;
            }
            else
            {
                pro_flg.tBits.ShortFlg = 1;
                g_eScdState = SCD_STATE_LATCHED;
            }
            break;
        }
        if (GetTick_ms() >= g_u64ScdTimer + SCD_PDSG_CHECK_MS)
        {
            PDSG_FETControl(false);
            g_bScdRecoveryActive = false;
            pro_flg.tBits.ShortFlg = 0;
            g_u64ScdTimer = GetTick_ms();
            g_eScdState = SCD_STATE_DSG_VERIFY;
        }
        break;

    case SCD_STATE_DSG_VERIFY:
        if (g_AfeRegs.R0.bitmap.SCD)
        {
            pro_flg.tBits.ShortFlg = 1;
            CleanFlg(AFE_FLAG_SCD);
            g_byScdRetryCount++;
            if (g_byScdRetryCount < SCD_MAX_RETRIES)
            {
                g_u64ScdTimer = GetTick_ms();
                g_eScdState = SCD_STATE_COOLDOWN;
            }
            else
            {
                g_eScdState = SCD_STATE_LATCHED;
            }
            break;
        }
        if (GetTick_ms() >= g_u64ScdTimer + SCD_DSG_VERIFY_MS)
        {
            pro_flg.tBits.ShortFlg = 0;
            g_byScdRetryCount = 0;
            g_bScdRecoveryActive = false;
            g_eScdState = SCD_STATE_IDLE;
        }
        break;

    case SCD_STATE_LATCHED:
        if (bCHGING || GPIO_ReadInputDataBit(WAKE_LOAD_PORT, WAKE_LOAD_PIN))
        {
            pro_flg.tBits.ShortFlg = 0;
            g_byScdRetryCount = 0;
            g_bScdRecoveryActive = false;
            g_eScdState = SCD_STATE_IDLE;
        }
        break;
		default:
			break;
    }
}


void Protection_handle(void)
{
  if (DVC1124_ReadRegs(AFE_ADDR_R(0), 1))
  {                                   // ������Ϣ
    if (g_AfeRegs.R0.bitmap.SCD == 1) // 短路
      pro_flg.tBits.ShortFlg = 1;
    if (g_AfeRegs.R0.bitmap.OCC2 == 1) // 2级充电
      pro_flg.tBits.Occ2_Flg = 1;
    if (g_AfeRegs.R0.bitmap.OCD2 == 1) // 2级放电
      pro_flg.tBits.Ocd2_Flg = 1;
    // if (g_AfeRegs.R0.bitmap.OCC1 == 1) // 1级充电
    //   pro_flg.tBits.Occ1_Flg = 1;
    // if (g_AfeRegs.R0.bitmap.OCD1 == 1) // 1���ŵ�
    //   pro_flg.tBits.Ocd1_Flg = 1;
    if (g_AfeRegs.R0.bitmap.CUV == 1) // Ƿѹ
      pro_flg.tBits.Uv_Flg = 1;
    if (g_AfeRegs.R0.bitmap.COV == 1) // 过压
      pro_flg.tBits.Ov_Flg = 1;
  }

  ProtectSCD();
  ProtectUV();
  ProtectOV();
  ProtectReChgInhibit();   // 复充禁止锁存判定
  ProtectUTC_OTC();
  ProtectUTD_OTD();
  ProtectOCC1();
  ProtectOCD1();
  ProtectOCD2();
  ProtectWait();
  DIAG_FaultLevelTranProcess();
}


