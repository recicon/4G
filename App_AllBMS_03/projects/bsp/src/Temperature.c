#include "Temperature.h"
uint16_t Rpu = 0; 
tDiagnoseSample g_tDiagnoseSample;

const uint32_t NTC_10K3950_RTable1[N_PT100_TABLE_MAX] = 
{
	330617,309468,289899,271736,254843,239107,224433,210742,197960,186024,       /* -40 ~ -31℃  */
	174874,164455,154717,145613,137100,129135,121683,114706,108172,102050,       /* -30 ~ -21℃  */
	96312, 90932, 85885, 81148, 76700, 72523, 68597, 64907, 61437, 58171,        /* -20 ~ -11℃  */
	55098, 52205, 49480, 46912, 44492, 42210, 40059, 38028, 36112, 34304,        /* -10 ~ -01℃  */
	32596, 30982, 29458, 28017, 26655, 25366, 24147, 22994, 21902, 20868,        /*  00 ~  09℃  */
	19889, 18961, 18082, 17248, 16457, 15707, 14996, 14320, 13679, 13070,        /*  10 ~  19℃  */
	12491, 11942, 11419, 10922, 10449, 10000, 9572,  9164,  8776,  8407,         /*  20 ~  29℃  */
	8055,  7719,  7400,  7095,  6804,  6527,  6262,  6009,  5768,  5538,         /*  30 ~  39℃  */
	5318,  5108,  4907,  4715,  4532,  4357,  4189,  4029,  3875,  3728,         /*  40 ~  49℃  */
	3588,  3453,  3324,  3201,  3083,  2969,  2861,  2757,  2657,  2561,         /*  50 ~  59℃  */
	2469,  2381,  2297,  2216,  2138,  2064,  1992,  1923,  1857,  1794,         /*  60 ~  69℃  */
	1733,  1674,  1618,  1563,  1511,  1461,  1413,  1367,  1322,  1279,         /*  70 ~  79℃  */
	1238,  1198,  1160,  1123,  1087,  1053,  1020,  988,   957,   928,          /*  80 ~  89℃  */
	899,   872,   845,   820,   795,   771,   748,   726,   704,   684,          /*  90 ~  99℃  */
	664,   644,   626,   607,   590,   573,   557,   541,   526,   511,          /*  100 ~ 109℃ */
	497,   483,   469,   456,   444,   432,   420,   408,   397,   387, 376,       /*  110 ~ 120℃ */
//  366,   357,   347,   338,   329,   319,   310,   302,   295,          /*  121 ~ 129℃ */
//	287,   280,   273,   266,   259,   252,   246,   240,                        /*  130 ~ 137℃ */
};

/**
 * @brief 根据NTC电阻值计算温度
 * @param resistance 测量的电阻值
 * @return 温度值（单位：摄氏度） 偏移40
 */
u32 calculate_temperature(uint32_t resistance) {
		u32 temp_low, temp_high, res_low, res_high, res_measured, temperature;
		int i = 0;
   
    // 处理超出范围的情况
    if (resistance >= NTC_10K3950_RTable1[0]) {
        return 255; // 低于最低测量温度
    }
    if (resistance <= NTC_10K3950_RTable1[N_PT100_TABLE_MAX-1]) {
        return 255; // 高于最高测量温度
    }
    
    // 查找电阻值在表中的位置

    while (i < N_PT100_TABLE_MAX-1 && 
           resistance < NTC_10K3950_RTable1[i]) {
        i++;
    }
    
    if (i > 0) {
        i--; 
        temp_low = -40 + i;
        temp_high = -39 + i;
        res_low = (float)NTC_10K3950_RTable1[i];
        res_high = (float)NTC_10K3950_RTable1[i+1];
        res_measured = (float)resistance;
        temperature = temp_low + (temp_high - temp_low) * 
                          (res_measured - res_low) / (res_high - res_low);
													return temperature + 40;
    }
    
    return 255; // 默认返回值0xff
}

/**
	* @说明	读取NFRT计算上拉电阻
	* @参数	
	* @返回值	
	* @注	
*/
void Read_NFRT (void){
DVC1124_ReadRegs(AFE_ADDR_R(126),1);
Rpu=g_AfeRegs.R126.F1RT*25+6800;//欧姆

}

bool HSFM_Control(bool mode){
	
	g_AfeRegs.R85.HSFM=mode;

	return DVC1124_WriteRegs(AFE_ADDR_R(85),1);
}

void InterruptShield_Config(uint8_t mode){

	memset((uint8_t *)&g_AfeRegs+121,mode,1);
	DVC1124_WriteRegs(AFE_ADDR_R(121), 1);
}

/**
	* @说明	GP1、GP4、GP5、GP6管脚复用配置
	* @参数	
	* @返回值	
	* @注	
*/
void GPM_Setup (void){

HSFM_Control(1);//禁止高边
g_AfeRegs.R116.GP1M|=0x01;   //GP1 温度检测
g_AfeRegs.R116.GP2M = 0x07;  //GP2 预放电低边输出 PDSG
g_AfeRegs.R116.GP3M = 0x06;  //GP3 INT

g_AfeRegs.R117.GP4M|=0x01;   //GP4 温度检测
g_AfeRegs.R117.GP5M = 0x07;  //GP5 充电低边输出 CHG
g_AfeRegs.R117.GP6M = 0x07;  //GP6 放电低边输出 DSG

DVC1124_WriteRegs(AFE_ADDR_R(116),2);
Read_NFRT();

g_AfeRegs.R101.CWT=40;//0.4mV 休眠电流唤醒阈值
DVC1124_WriteRegs(AFE_ADDR_R(101),1);

InterruptShield_Config(0x7F); ////GP中断触发选择

}

float DVC1124_Calc_BatTemp(uint8_t GP){
float V1P8t,VGPt,Tp_Value,GP_res;
uint16_t V1P8=(g_AfeRegs.R15_16.V1P8_H<<8)|g_AfeRegs.R15_16.V1P8_L;
uint16_t GPn_T=(g_AfeRegs.R17_28.VGP[GP].VGP_H<<8)|g_AfeRegs.R17_28.VGP[GP].VGP_L;

V1P8t=V1P8*0.1;//V1P8 * _lsbVCELL *1000 单位mv

VGPt=GPn_T*0.1;//VGP1 * _lsbVCELL *1000 单位mv

GP_res=(VGPt/V1P8t*Rpu)/(1-VGPt/V1P8t);//热敏电阻计算值 
Tp_Value = calculate_temperature(GP_res);
return Tp_Value;
}



void ADC_RCC_Configuration(void)
{
	/* Enable peripheral clocks */
    /* Enable GPIOA clocks */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA, ENABLE);
    /* Enable ADC clocks */
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_ADC, ENABLE);
        
 /* RCC_ADCHCLK_DIV16*/
    ADC_ConfigClk(ADC_CTRL3_CKMOD_AHB, RCC_ADCHCLK_DIV16);
    RCC_ConfigAdc1mClk(RCC_ADC1MCLK_SRC_HSE, RCC_ADC1MCLK_DIV8);  //selsect HSE as RCC ADC1M CLK Source
}

void ADC_GPIO_Configuration(void)
{
    GPIO_InitType GPIO_InitStructure;

    GPIO_InitStruct(&GPIO_InitStructure);
    /* PA1 PA2  as analog input --------*/
    GPIO_InitStructure.Pin       = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Analog;
    GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);
}

void ADC_Initial(void)
{
  ADC_InitType ADC_InitStructure;
  ADC_RCC_Configuration();
  ADC_GPIO_Configuration();

  ADC_DeInit(ADC);
  ADC_InitStructure.MultiChEn = DISABLE;
  ADC_InitStructure.ContinueConvEn = DISABLE;
  ADC_InitStructure.ExtTrigSelect = ADC_EXT_TRIGCONV_NONE;
  ADC_InitStructure.DatAlign = ADC_DAT_ALIGN_R;
  ADC_InitStructure.ChsNumber = 1;
  ADC_Init(ADC, &ADC_InitStructure);
  ADC_Enable(ADC, ENABLE);
  /* Check ADC Ready */
  while (ADC_GetFlagStatusNew(ADC, ADC_FLAG_RDY) == RESET)
    ;
  /* Start ADC1 calibration */
  ADC_StartCalibration(ADC);
  /* Check the end of ADC1 calibration */
  while (ADC_GetCalibrationStatus(ADC))
    ;
}

uint16_t MA_Filter(tag_maf *maf,uint16_t input)
{
    uint8_t i = 0;
	uint32_t sum = 0;
	
	uint16_t max = input;
	uint16_t min = input; 
	maf->Buf[maf->u8Pt] = input;
	
	if(maf->u8DatLen > 0)
	{
		for(i = 0; i < maf->u8DatLen; i++)		//求和与最值
		{
			sum += maf->Buf[i];
			
			if(maf->Buf[i] > max)
			{
				max = maf->Buf[i];
			}
			
			if(maf->Buf[i] < min)
			{
				min = maf->Buf[i];
			}
		}
		
		if(maf->u8DatLen > 2)					//去最值求平均
		{
			sum = sum - max - min;

			sum /= (uint32_t)(maf->u8DatLen - 2);
		}
		else
		{
			sum /= (maf->u8DatLen);
		}

		if(maf->u8Pt >= (maf->u8DatLen - 1))	//调整指针
		{
			maf->u8Pt = 0;
		}
		else
		{
			maf->u8Pt++;
		}
	}
	else
	{
		sum = 0;
	}

	return (uint16_t)sum;
}

int8_t CAL_ResConvert(uint32_t u32PullUpRes, uint16_t u16RegBin, uint16_t u16ResBin, uint32_t *pu32Res)
{
	int8_t i8ret = 0;
	
	if((u16RegBin - u16ResBin) > 0)
	{
		*pu32Res = (u32PullUpRes * u16ResBin)/(u16RegBin - u16ResBin);
	}
	else
	{
		i8ret = -1;						//错误参数
	}
	
	return i8ret;	
}


uint16_t ADC_GetData(uint8_t ADC_Channel)
{
    uint16_t dat;
    ADC_ConfigRegularChannel(ADC, ADC_Channel, 1, ADC_SAMP_TIME_55CYCLES5);
    /* Start ADC Software Conversion */
    ADC_EnableSoftwareStartConv(ADC,ENABLE);
    while(ADC_GetFlagStatus(ADC,ADC_FLAG_ENDC)==0){
    }
    ADC_ClearFlag(ADC,ADC_FLAG_ENDC);
    ADC_ClearFlag(ADC,ADC_FLAG_STR);
    dat=ADC_GetDat(ADC);
    return dat;
}

void Mos_Temper(void)
{
  static tag_maf sMafTsReg	= {{0}, REGVOLFLT_CNT, 0};
  static tag_maf ETSAi = {{0}, TSFLT_CNT, 0};
  static tag_maf ENTC3 = {{0}, TSFLT_CNT, 0};
  static tag_maf ENTC4 = {{0}, TSFLT_CNT, 0};
  uint16_t voltage[4] = {0}; 
  int8_t  i8tmp  = 0;	
  uint32_t u32tmp = 0;
	uint16_t ntc3,ntc4;

  voltage[0] = ADC_GetData(ADC_CH_2_PA1);
	voltage[1] = ADC_GetData(ADC_CH_3_PA2);
	voltage[2] = ADC_GetData(ADC_CH_4_PA3);
	voltage[3] = ADC_GetData(ADC_CH_5_PA4);
	

	g_tDiagnoseSample.wTRegAi = MA_Filter(&sMafTsReg, voltage[0]);
	g_tDiagnoseSample.wETsAi = MA_Filter(&ETSAi, voltage[1]);
	ntc3 = MA_Filter(&ENTC3, voltage[2]);
	ntc4 = MA_Filter(&ENTC4, voltage[3]);

	i8tmp = CAL_ResConvert(TEMP_UPRES, (g_tDiagnoseSample.wTRegAi << 1), g_tDiagnoseSample.wETsAi, &u32tmp);
	if (0 == i8tmp)
	{
		g_tBatPackInfo.byTemp1 = calculate_temperature(u32tmp);
	}

	if (g_tBatPackCtrl.tEESpec.byPackTemp > 2)
	{
		i8tmp = CAL_ResConvert(TEMP_UPRES, (g_tDiagnoseSample.wTRegAi << 1), ntc3, &u32tmp);
		if (0 == i8tmp)
		{
			g_tBatPackInfo.tCell.byTemp[2] = calculate_temperature(u32tmp);
		}
		if (g_tBatPackCtrl.tEESpec.byPackTemp > 3)
		{
			i8tmp = CAL_ResConvert(TEMP_UPRES, (g_tDiagnoseSample.wTRegAi << 1), ntc4, &u32tmp);
			if (0 == i8tmp)
			{
				g_tBatPackInfo.tCell.byTemp[3] = calculate_temperature(u32tmp);
			}
		}
	}
}

void Temperature_Handle(void)
{
	int8_t tempIndex;
	int Tempdata;
  uint8_t i,j;

	if (DVC1124_ReadRegs(AFE_ADDR_R(15), 14))
	{
		if (g_tBatPackCtrl.tEESpec.byPackTemp > 2)
		{
			j = 2;
		}else{
			j = g_tBatPackCtrl.tEESpec.byPackTemp;
		}
		
    for ( i = 0; i < j; i++)
    {
      g_tBatPackInfo.tCell.byTemp[i] = DVC1124_Calc_BatTemp(GP1 + i*3);
    }
  
	}

     Mos_Temper(); //mos温度
		     //计算最大最小温度
      g_tBatPackInfo.tCell.byTempMin = g_tBatPackInfo.tCell.byTempMax = g_tBatPackInfo.tCell.byTemp[0]; // 初始化
      for (tempIndex =  g_tBatPackCtrl.tEESpec.byPackTemp - 1; tempIndex >= 0; tempIndex--)
      {
        Tempdata = g_tBatPackInfo.tCell.byTemp[tempIndex];
        if (Tempdata > g_tBatPackInfo.tCell.byTempMax)
        {
          g_tBatPackInfo.tCell.byTempMax = Tempdata;
          if (g_tBatPackInfo.tCell.byTempMin == 0)
            g_tBatPackInfo.tCell.byTempMin = g_tBatPackInfo.tCell.byTempMax;
        }
        else if (Tempdata < g_tBatPackInfo.tCell.byTempMin && Tempdata > 0)
        {
          g_tBatPackInfo.tCell.byTempMin = Tempdata;
        }
      }
}

