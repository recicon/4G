#ifndef __PROTECTION_H__
#define __PROTECTION_H__
#include"DVC1124.h"
#include "Temperature.h"

//单节欠压释放电压
#define OVRVOL 2400         //2400mv

//单节过压保护释放延时(100mS)
#define OVR_DELAY_CNT    20 //2S
#define OV_DELAY_CNT     2000 //AFE 2S
//单节过压释放电压
#define UVRVOL 3600         //3600mv
//单节欠压保护释放延时(100mS)
#define UVR_DELAY_CNT 600    //60S
#define UV_DELAY_CNT 2000    //2S

//充电低温
#define UTC_TEMP        30  //偏移40，-10℃
#define UTC_DELAY_CNT   50    //5S 保护
#define UTCR_TEMP       35   //-5℃
#define UTCR_DELAY_CNT  71    //5S 恢复
//充电高温
#define OTC_TEMP       105  //偏移， 65℃
#define OTC_DELAY_CNT   50    //5S 保护
#define OTCR_TEMP       95   //55℃
#define OTCR_DELAY_CNT  50   //5S 恢复
//放电低温
#define UTD_TEMP      20  //偏移40，-20℃
#define UTD_DELAY_CNT 20    //2S 保护
#define UTDR_TEMP     30   //-10℃
#define UTDR_DELAY_CNT 20    //2S 恢复
//放电高温
#define OTD_TEMP       115  //偏移， 75℃
#define OTD_DELAY_CNT 20    //2S 保护
#define OTDR_TEMP       105   //65℃
#define OTDR_DELAY_CNT 20    //2S 恢复

//充电过流
#define OCC1_DELAY_CNT  100    //10S
#define OCC1R_DELAY_CNT 300   //30S
#define OCC2R_DELAY_CNT 70    //7.0S
//放电过流
#define OCD1_DELAY_CNT  500    //50S
#define OCD1R_DELAY_CNT 300    //30S
#define OCD2R_DELAY_CNT 70    //7.0S

#define MOSTEMP_PRO    (105 + 40)    //mos高温105
#define MOSTEMP_RE     (60 + 40)    //mos高温恢复60
#define TEMP_BOUND_ALARM            (-5 + 40)    

#define CURoffset    1000

//复充禁止解除阈值
#define RECHG_SOC_REL      9800   //SOC<98% 解除禁充 (0-10000 量纲)
#define RECHG_DSGCUR_REL   20     //放电>2A 解除禁充 (0.1A 量纲, 正=放电)
#define SCD_COOLDOWN_MS          2000
#define SCD_PDSG_CHECK_MS         200
#define SCD_DSG_VERIFY_MS        1000
#define SCD_MAX_RETRIES             3

enum{
	waitpro_temp = 0, 
	waitpro_Vol,      
};

typedef enum    // SCD恢复状态
{
	SCD_STATE_IDLE = 0,
	SCD_STATE_COOLDOWN,
	SCD_STATE_PDSG_CHECK,
	SCD_STATE_DSG_VERIFY,
	SCD_STATE_LATCHED,
}ScdRecoveryState;

typedef enum						// AFE保护类型
{
	AFE_FLAG_UV = 0,  //欠压
	AFE_FLAG_OV ,			//过压
	AFE_FLAG_OCD1,		//1级放电
	AFE_FLAG_OCD2	,		//2级放电
	AFE_FLAG_OCC1	,		//1级充电
	AFE_FLAG_OCC2	,		//2级充电
	AFE_FLAG_UTC,			//充电低温
	AFE_FLAG_OTC,			//充电高温
	AFE_FLAG_UTD,			//放电低温
	AFE_FLAG_OTD,			//放电高温
	AFE_FLAG_SCD,			//短路
}AFE_ProtectType;

typedef union
{
    uint16_t ProByte;
    struct
    {
			uint16_t ShortFlg : 1; // 短路故障
			uint16_t Ov_Flg : 1;	 // 过压故障
			uint16_t Uv_Flg : 1;	 // 欠压故障
			uint16_t Occ1_Flg : 1; // 充电1级故障
			uint16_t Occ2_Flg : 1; // 充电2级故障
			uint16_t Ocd1_Flg : 1; // 放电1级故障
			uint16_t Ocd2_Flg : 1; // 放电2级故障
			uint16_t Utc_Flg : 1;	 // 充电低温
			uint16_t Otc_Flg : 1;	 // 充电高温
			uint16_t Utd_Flg : 1;	 // 放电低温
			uint16_t Otd_Flg : 1;	 // 放电高温
			uint16_t Rsv : 5;		 // 保留
		}tBits;
}uProFlgVal;   

bool AFEParam_Config(void);
void Protection_handle(void);

bool CHG_FETControl(u8 mode);
bool DSG_FETControl(u8 mode);

void Fet_Status(void);

void MosCtrl(void);
void ProtectSCD(void);

extern uProFlgVal pro_flg;
extern uint8_t waitpro;
extern bool g_bScdRecoveryActive;

#endif
