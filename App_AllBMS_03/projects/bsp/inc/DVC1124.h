#ifndef __DVC1124_H__
#define __DVC1124_H__
#include <stdio.h>
#include <string.h>
#include "n32wb43x_i2c.h"
#include "n32wb43x_it.h"
#include"port.h"
#include"Protection.h"
#include "Temperature.h"
#include"App.h"
#include "software_i2c.h"
#include "bms_config.h"

enum {				 //CHG DSG MODE
FET_Close=1,	 //关闭FET驱动
FET_Close_BFCO,//关闭FET驱动，但允许在电流大于续流阈值开启FET驱动
FET_OPEN
};		

#define ADDRESS 						0x40//IIC设备地址40 C0 C2 C4 C6
#define sEE_FLAG_TIMEOUT ((uint32_t)0x1000)
#define sEE_LONG_TIMEOUT ((uint32_t)(100 * sEE_FLAG_TIMEOUT))


#define	 AFE_MAX_CELL_CNT               24 
#define  AFE_MAX_GP_CNT                 6
#define  AFE_REG_VALUE(Rn)              *((uint8_t *)&g_AfeRegs.Rn)
#define  AFE_ADDR_R(n)                   n
#define _lsbVCELL                       0.0001 //VADC测量分辨率（最小刻度对应的电压值 V/bit）
#define _lsbVCELL_signed                0.0002 //VADC测量分辨率（最小刻度对应的电压值 V/bit）
#define _lsbCC1                        (5e-6)//CC1的测量分辨率（最小刻度对应的电压值）
/* 使用bms_config.h中的型号相关参数 */
#define _lsbCC2                        CURR_LSB_CC2//CC2�Ĳ����ֱ��ʣ���С�̶ȶ�Ӧ�ĵ�ѹֵ��
#define  CurrentSenseResistance_mR     CURR_SENSE_RESISTANCE_mR  //������������ֵ����λmR)


//------------------------------------------------------------------------------
#pragma pack (push,1)
enum{GP1=0,GP2,GP3,GP4,GP5,GP6};//GPn

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
typedef struct{
uint8_t cleanR0;
}CleanFlag;
typedef struct{
 union {struct{
   uint8_t SCD:1;   //REGMODE01/R0[0]/0x00/SCD_FLAG, 放电短路标识位;0，未发生放电短路;1，已发生放电短路，将该bit置0即可清除
   uint8_t OCC2:1;  //REGMODE01/R0[1]/0x00/OCIC2_FLAG, 2级充电过流标识位;0，未发生2级充电过流;1，已发生2级充电过流，将该bit置0即可清除
   uint8_t OCD2:1;  //REGMODE01/R0[2]/0x00/OCID2_FLAG, 2级放电过流标识位;0，未发生2级放电过流;1，已发生2级放电过流，将该bit置0即可清除
   uint8_t OCC1:1;  //REGMODE01/R0[3]/0x00/OCIC1_FLAG, 1级充电过流标识位;0，未发生1级充电过流;1，已发生1级充电过流，将该bit置0即可清除
   uint8_t OCD1:1;  //REGMODE01/R0[4]/0x00/OCID1_FLAG, 1级放电过流标识位;0，未发生1级放电过流;1，已发生1级放电过流，将该bit置0即可清除
   uint8_t CUV:1;   //REGMODE01/R0[5]/0x00/CUV_FLAG, 电池欠压警告;0，未发生电池欠压;1，已发生电池欠压，将该bit置0即可清除
   uint8_t COV:1;   //REGMODE01/R0[6]/0x00/COV_FLAG, 电池过压标识位;0，未发生电池过压;1，已发生电池过压，将该bit置0即可清除
   uint8_t IWF:1;   //REGMODE01/R0[7]/0x00/I2C_WDTO_FLAG, I2C看门狗溢出标识位;0, 未发生I2C看门狗溢出;1, 已发生I2C看门狗溢出，将该bit置0即可清除
	}bitmap;uint8_t cleanflag;
	}R0;
  struct{//R1
   uint8_t CST:4;  //REGMODE03/R1[3:0]/0x00/CHIP_ST,芯片状态标识位;0000, 芯片从关机状态被唤醒;0001, 芯片从休眠状态被I2C通信唤醒;0010, 芯片从休眠状态被定时唤醒;0011, 芯片从休眠状态被放电电流唤醒;0100, 芯片从休眠状态被充电电流唤醒;0101, 芯片从休眠状态被2级放电过流唤醒;0110, 芯片从休眠状态被2级充电过流唤醒;0111, 芯片从休眠状态被放电短路唤醒;1000, 芯片从休眠状态被充电器唤醒;1110, 使芯片进入休眠状态;1111, 使芯片进入关断状态
   uint8_t CC2F:1; //REGMODE00/R1[4]/0x00/CC2_FLAG, CADC CC1转换完成标识位，I2C读取该地址后会自动清零;0, CADC CC2未完成转换;1, CADC CC2已完成转换
   uint8_t CC1F:1; //REGMODE00/R1[5]/0x00/CC1_FLAG, CADC CC1转换完成标识位，I2C读取该地址后会自动清零;0, CADC CC1未完成转换;1, CADC CC1已完成转换
   uint8_t VADF:1; //REGMODE00/R1[6]/0x00/VADC_FLAG, VADC转换完成标识位，I2C读取该地址后会自动清零;0, VADC未完成转换;1, VADC已完成转换
   uint8_t PD:1;   //REGMODE00/R1[7]/0x00/PACK_DET, 充电器检测标识位;0, 未检测到充电器;1, 已检测到充电器
  }R1;
  struct{//CC1: CADC CC1电流值[15:0], 有符号二进制补码, LSB=5μV
    uint8_t CC1_H;  //REGMODE00/R2[7:0]/0x00/CC1电流值高字节
    uint8_t CC1_L;  //REGMODE00/R3[7:0]/0x00/CC1电流值低字节
  }R2_3;
  struct{//CC2: CADC CC2电流值[19:0], 有符号二进制补码, LSB=5/16μV
    uint8_t CC2_H;  //REGMODE00/R4[7:0]/0x00/CC2电流值高字节
    uint8_t CC2_M;  //REGMODE00/R5[7:0]/0x00/CC2电流值中字节
    uint8_t CHGF:1; //REGMODE00/R6[0]/0x00/CHG_FLAG, CHG驱动输出标识位;0, CHG驱动输出已关闭;1, CHG驱动输出已开启
    uint8_t DSGF:1; //REGMODE00/R6[1]/0x00/DSG_FLAG, DSG驱动输出标识位;0, DSG驱动输出已关闭;1, DSG驱动输出已开启
    uint8_t PCHGF:1;//REGMODE00/R6[2]/0x00/PCHG_FLAG, PCHG驱动输出标识位;0, PCHG驱动输出已关闭;1, PCHG驱动输出已开启
    uint8_t PDSGF:1;//REGMODE00/R6[3]/0x00/PDSG_FLAG, PDSG驱动输出标识位;0, PDSG驱动输出已关闭;1, PDSG驱动输出已开启
    uint8_t CC2_L:4;//REGMODE00/R6[7:4]/0x00/CC2电流值低字节
  }R4_6;
  struct{//VBAT电压值[15:0], 无符号二进制，LSB=12.8mV
    uint8_t VBAT_H; //REGMODE00/R7[7:0]/0x00/VBAT电压值高8位
    uint8_t VBAT_L; //REGMODE00/R8[7:0]/0x00/VBAT电压值低8位
  }R7_8;
  struct{//VPACK电压值[15:0], 无符号二进制，LSB=12.8mV
    uint8_t VPK_H; //REGMODE00/R9[7:0]/0x00/VPK电压值高8位
    uint8_t VPK_L; //REGMODE00/R10[7:0]/0x00/VPK电压值低8位
  }R9_10;
  struct{//VLOAD电压值[15:0], 无符号二进制，LSB=12.8mV
    uint8_t VLD_H; //REGMODE00/R11[7:0]/0x00/VPK电压值高8位
    uint8_t VLD_L; //REGMODE00/R12[7:0]/0x00/VPK电压值低8位
  }R11_12;
  struct{//VCT芯片内核温度值[15:0], 无符号二进制, 芯片内核温度=VCT*0.24467-271.03
    uint8_t VCT_H; //REGMODE00/R13[7:0]/0x00/VCT电压值高8位
    uint8_t VCT_L; //REGMODE00/R14[7:0]/0x00/VCT电压值低8位
  }R13_14;
  struct{//V1P8电压值[15:0], 无符号二进制，LSB=100μV
    uint8_t V1P8_H; //REGMODE00/R15[7:0]/0x00/V1P8电压值高8位
    uint8_t V1P8_L; //REGMODE00/R16[7:0]/0x00/V1P8电压值低8位
  }R15_16;
  struct{struct{//GPn电压值[15:0], 无符号二进制，LSB=100μV
    uint8_t VGP_H; //REGMODE00/R17[7:0]/0x00/GPn电压值高8位
    uint8_t VGP_L; //REGMODE00/R18[7:0]/0x00/GPn电压值低8位
		}VGP[6]; //R17_28
	}R17_28;
  struct{struct{//CELLn电压值[15:0], 无符号二进制，LSB=100μV
    uint8_t VCELL_H; //REGMODE00/R29[7:0]/0x00/CELLn电压值高8位
    uint8_t VCELL_L; //REGMODE00/R30[7:0]/0x00/CELLn电压值低8位
  }VCELLS[24];//R29_R76
	}R29_76;
  struct{//VADC直流失调电压值[15:0], 无符号二进制，LSB=100μV
    uint8_t VVOS_H; //REGMODE00/R77[7:0]/0x00/VADC直流失调电压值高8位
    uint8_t VVOS_L; //REGMODE00/R78[7:0]/0x00/VADC直流失调电压值低8位
  }R77_78;
  struct{//CADC直流失调电压值[15:0],有符号二进制，LSB=5μV
    uint8_t CVOS_H; //REGMODE00/R79[7:0]/0x00/CADC直流失调电压值高8位
    uint8_t CVOS_L; //REGMODE00/R80[7:0]/0x00/CADC直流失调电压值低8位
  }R79_80;
  struct{
    uint8_t CHGC:2;  //REGMODE03/R81[1:0]/0x00/CHG_CTRL[1:0], CHG驱动输出控制位;00/01, 关闭CHG驱动输出;10, 关闭CHG驱动输出，但允许在放电电流大于FET体二极管续流阈值时开启CHG驱动输出;11, 开启CHG驱动输出
    uint8_t DSGC:2;  //REGMODE03/R81[3:2]/0x00/DSG_CTRL[1:0], DSG驱动输出控制位;00/01, 关闭DSG驱动输出;10, 关闭DSG驱动输出，但允许在充电电流大于FET体二极管续流阈值时开启DSG驱动输出;11, 开启DSG驱动输出
    uint8_t DSGM:1;  //REGMODE03/R81[4]/0x00/DSG_MODE, DSG驱动输出模式控制位;0, DSG为电荷泵驱动输出模式;1, DSG为源随驱动输出模式
    uint8_t PCHGC:1; //REGMODE03/R81[5]/0x00/PCHG_CTRL, PCHG驱动输出控制位;0, 关闭PCHG驱动输出;1, 开启PCHG驱动输出
    uint8_t PDSGC:1; //REGMODE03/R81[6]/0x00/PDSG_CTRL, PDSG驱动输出控制位;0, 关闭PDSG驱动输出;1, 开启PDSG驱动输出
    uint8_t LDPU:1;  //REGMODE03/R81[7]/0x00/LOAD_PULLUP, LOAD上拉控制位;0, 关闭LOAD上拉;1, 开启LOAD上拉
  }R81;
  struct{
    uint8_t DPC:5;  //REGMODE03/R82[4:0]/0x10/DSG_PD_CTRL[4:0], DSG驱动输出下拉强度控制位，数值越大，下拉能力越强
		uint8_t R82_RVD:2;
    uint8_t PDWM:1; //REGMODE03/R82[7]/0x01/PDSG_I2C_WDTO_MASK, I2C超时关闭PDSG驱动输出屏蔽位;0, I2C超时关闭PDSG驱动输出;1, I2C超时不影响PDSG驱动输出状态
  }R82;
  struct{
    uint8_t DBDM:1;  //REGMODE03/R83[0]/0x00/DSG_BODY_DIODE_MASK, 放电NFET体二极管保护屏蔽位;0, 充电电流大于放电NFET体二极管续流阈值时自动开启DSG驱动输出;1, 充电电流大于放电NFET体二极管续流阈值时不影响DSG驱动输出状态
    uint8_t DPDM:1;  //REGMODE03/R83[1]/0x00/DSG_PDSG_MASK, PDSG开启时关闭DSG驱动输出屏蔽位;0, PDSG开启时关闭DSG驱动输出;1, PDSG开启时不影响DSG驱动输出状态
    uint8_t DDM:1;   //REGMODE03/R83[2]/0x00/DSG_DOFFN_MASK, DON(DSG_OFF_N)关闭DSG驱动输出屏蔽位;0, DON输入为0时关闭DSG驱动输出;1, DON输入不影响DSG驱动输出状态
    uint8_t DWM:1;   //REGMODE03/R83[3]/0x00/DSG_I2C_WDTO_MASK, I2C超时关闭DSG驱动输出屏蔽位;0, I2C超时关闭DSG驱动输出;1, I2C超时不影响DSG驱动输出
    uint8_t PCDM:1;  //REGMODE03/R83[4]/0x01/PCHG_DON_MASK, DON(DSG_OFF_N)关闭PCHG驱动输出屏蔽位;0, DON输入为0时关闭PCHG驱动输出;1, DON输入不影响PCHG驱动输出状态
    uint8_t PCCM:1;  //REGMODE03/R83[5]/0x00/PCHG_CON_MASK, CON(CHG_OFF_N)关闭PCHG驱动输出屏蔽位;0, CON输入为0时关闭PCHG驱动输出;1, CON输入不影响PCHG驱动输出状态
    uint8_t PCWM:1;  //REGMODE03/R83[6]/0x00/PCHG_I2C_WDTO_MASK, I2C超时关闭PCHG驱动输出屏蔽位;0, I2C超时关闭PCHG驱动输出;1, I2C超时不影响PCHG驱动输出状态
    uint8_t PDDM:1;  //REGMODE03/R83[7]/0x00/PDSG_DON_MASK, DON(DSG_OFF_N)关闭PDSG驱动输出屏蔽位;0, DON输入为0时关闭PDSG驱动输出;1, DON输入不影响PDSG驱动输出状态
  }R83;
  struct{
    uint8_t CBDM:1;  //REGMODE03/R84[0]/0x00/CHG_BODY_DIODE_MASK, 充电NFET体二极管保护屏蔽位;0, 放电电流大于充电NFET体二极管续流阈值时自动开启CHG驱动输出;1, 放电电流大于充电NFET体二极管续流阈值时不影响CHG驱动输出状态
    uint8_t CPCM:1;  //REGMODE03/R84[1]/0x00/CHG_PCHG_MASK, PCHG开启时关闭CHG驱动输出屏蔽位;0, PCHG开启时关闭CHG驱动输出;1, PCHG开启时不影响CHG驱动输出状态
    uint8_t CCM:1;   //REGMODE03/R84[2]/0x00/CHG_CON_MASK, CON(CHG_OFF_N)关闭CHG驱动输出屏蔽位;0, CON输入为0时关闭CHG驱动输出;1, CON输入不影响CHG驱动输出状态
    uint8_t CDM:1;   //REGMODE03/R84[3]/0x01/CHG_DON_MASK, DON(DSG_OFF_N)关闭CHG驱动输出屏蔽位;0, DON输入为0时关闭CHG驱动输出;1, DON输入不影响CHG驱动输出状态
    uint8_t CSM:1;   //REGMODE03/R84[4]/0x01/CHG_SCD_MASK, 放电短路时关闭CHG驱动输出屏蔽位;0, 放电短路时关闭CHG驱动输出;1, 放电短路时不影响CHG驱动输出
    uint8_t CO2M:1;  //REGMODE03/R84[5]/0x01/CHG_OCD2_MASK, 2级放电过流时关闭CHG驱动输出屏蔽位;0, 2级放电过流时关闭CHG驱动输出;1, 2级放电过流时不影响CHG驱动输出
    uint8_t CO1M:1;  //REGMODE03/R84[6]/0x01/CHG_OCD1_MASK, 1级放电过流时关闭CHG驱动输出屏蔽位;0, 1级放电过流时关闭CHG驱动输出;1, 1级放电过流时不影响CHG驱动输出
    uint8_t CWM:1;   //REGMODE03/R84[7]/0x00/CHG_I2C_WDTO_MASK, I2C超时关闭CHG驱动输出屏蔽位;0, I2C超时关闭CHG驱动输出;1, I2C超时不影响CHG驱动输出
  }R84;

  struct{
    uint8_t CAMZ:1;  //REGMODE02/R85[0]/0x00/CADC_MZ, CADC手动校零控制位;0, 无效;1, 开启一次CADC校准，该bit会自动复位为0
		uint8_t R85_RVD1:1;
    uint8_t CAES:1;  //REGMODE03/R85[2]/0x01/CADC_EN_SLEEP, 芯片休眠状态下CADC使能控制位;0, 芯片休眠状态下关闭CADC;1, 芯片休眠状态下开启CADC
    uint8_t CAEW:1;  //REGMODE03/R85[3]/0x01/CADC_EN_WORK, 芯片工作状态下CADC使能控制位;0, 芯片工作状态下关闭CADC;1, 芯片工作状态下开启CADC
		uint8_t R85_RVD4_6:3;
    uint8_t HSFM:1;  //REGMODE03/R85[7]/0x00/HSFET_MASK, 高边NFET驱动输出屏蔽位;0, 允许高边NFET驱动输出;1, 屏蔽高边NFET驱动输出
  }R85;
  struct{
  	uint8_t C1OS:2;  //REGMODE03/R86[1:0]/0x03/CC1_OSR_SLEEP[1:0], 芯片休眠状态下CC1过采样率控制位;00, 128;01, 256;10, 512;11, 1024
  	uint8_t C1OW:2;  //REGMODE03/R86[3:2]/0x03/CC1_OSR_WORK[1:0], 芯片工作状态下CC1过采样率控制位;00, 128;01, 256;10, 512;11, 1024
  uint8_t R86_RVD:4;	
  }R86;  
  uint8_t R87;
  uint8_t R88;
  struct{
		uint8_t OCD1T; //REGMODE03/R89[7:0]/0x00/OCD1_TH[7:0], 1级放电过流保护阈值控制位, V(OCD1_TH)=OCD1T*0.25mV
  }R89; 
  struct{
		uint8_t OCC1T; //REGMODE03/R90[7:0]/0xFF/OCC1_TH[7:0], 1级充电过流保护阈值控制位, V(OCC1_TH)=OCD1T*0.25mV
  }R90; 
  struct{
		uint8_t OCD1D; //REGMODE03/R91[7:0]/0x00/OCD1_DLY[7:0], 1级放电过流保护延迟控制位, T(OCD1_DLY)=OCD1D*8ms
  }R91; 
  struct{
		uint8_t OCC1D; //REGMODE03/R92[7:0]/0x00/OCC1_DLY[7:0], 1级充电过流保护延迟控制位, T(OCD1_DLY)=OCD1D*8ms
  }R92; 
	uint8_t R93;
  struct{
    uint8_t OCD2T:6;  //REGMODE03/R94[5:0]/0x00/OCD2_TH[5:0], 2级放电过流检测阈值控制位, 阈值电压=(OCD2T+1)*4mV
    uint8_t OCD2E:1;  //REGMODE03/R94[6]/0x01/OCD2_EN, 2级放电过流检测使能控制位;0, 关闭;1, 开启
    uint8_t R94_RVD:1;
  }R94;
  struct{
    uint8_t OCC2T:6;  //REGMODE03/R95[5:0]/0x00/OCC2_TH[7:0], 2级充电过流检测阈值控制位, 阈值电压=(OCC2T+1)*4mV
    uint8_t OCC2E:1;  //REGMODE03/R95[6]/0x01/OCC2_EN, 2级充电过流检测使能控制位;0, 关闭;1, 开启
    uint8_t R95_RVD:1;
  }R95;
  struct{  
   	uint8_t OCD2D;//REGMODE03/R96[7:0]/0x00/OCD2_DLY[7:0], 2级放电过流保护延迟控制位, 延迟时间=OCD2D*4ms  
  }R96;
  struct{  
  	uint8_t OCC2D;//REGMODE03/R97[7:0]/0x00/OCC2_DLY[7:0], 2级充电过流保护延迟控制位, 延迟时间=OCC2D*4ms 
  }R97;
  struct{
    uint8_t SCDT:6;  //REGMODE03/R98[5:0]/0x00/SCD_VTH[5:0], 放电短路检测阈值控制位, 阈值电压=SCDT*10mV
    uint8_t SCDE:1;  //REGMODE03/R98[6]/0x01/SCD_EN, 放电短路检测使能控制位;0, 关闭;1, 开启
    uint8_t R98_RVD:1;
  }R98;
  struct{  
		uint8_t SCDD;//REGMODE03/R99[7:0]/0x00/SCD_DLY[7:0], 放电短路检测延迟控制位, 延迟时间=SCDD*7.81us
  }R99;
  uint8_t R100;
  struct{    
		uint8_t CWT;   //REGMODE03/R101[7:0]/0x00/CURRENT_WKUP_TH[7:0], 芯片休眠状态下电流唤醒阈值控制位, 阈值电压=CWT*10μV
  }R101;
  struct{  
		uint8_t BDPT;  //REGMODE03/R102[7:0]/0x00/BODY_DIODE_PRT_TH[7:0], 充/放电NFET体二极管续流保护阈值控制位，阈值电压=BDPT*40μV
  }R102;
  struct{  
		uint8_t CB[3]; //REGMODE03/R103[7:0],R104[7:0],R105[7:0]/0x00/CELL_BAL[17:1], 第n节电池被动均衡控制位;0, 关闭第n节电池被动均衡;1, 开启第n节电池被动均衡
  }R103_R105;
  struct{
    uint8_t CM_H;    //REGMODE03/R106[7:0]/0xAA/第24~17节电池保护屏蔽控制位;0, 开启第n节电池保护;1, 关闭第n节电池保护, 同时默认关闭第n节电池电压测量
  }R106;			
  struct{
    uint8_t CM_M;    //REGMODE03/R107[7:0]/0x55/第16~9节电池保护屏蔽控制位
  }R107;
  struct{
    uint8_t V1P8M:1; //REGMODE03/R108[0]/0x00/V1P8_MASK, V1P8电压测量屏蔽控制位;0, 开启V1P8电压测量;1, 关闭V1P8电压测量
    uint8_t CTM:1;   //REGMODE03/R108[1]/0x00/CORE_TEMP_MASK, 芯片核心温度测量屏蔽控制位;0, 开启芯片核心温度测量;1, 关闭芯片核心温度测量
    uint8_t LDM:1;   //REGMODE03/R108[2]/0x00/LOAD_MASK, LOAD电压测量屏蔽控制位;0, 开启LOAD电压测量;1, 关闭LOAD电压测量
    uint8_t PKM:1;   //REGMODE03/R108[3]/0x00/PACK_MASK, PACK电压测量屏蔽控制位;0, 开启PACK电压测量;1, 关闭PACK电压测量
    uint8_t CM_L:4;    //REGMODE03/R108[7:4]/0x00/第8~5节电池保护屏蔽控制位
  }R108;
  struct{
    uint8_t CVS:1; //REGMODE03/R109[0]/0x00/CELL_VOLTAGE_SIGNED, 电池电压有符号数显示控制位;0, 电池电压以无符号数显示，LSB=100μV;1, 电池电压以有符号数显示，LSB=200μV
    uint8_t CMM:1; //REGMODE03/R109[1]/0x00/ELL_MASK_MEAS, 屏蔽电池电压测量控制位;0, 关闭屏蔽电池电压测量;1, 开启屏蔽电池电压测量
    uint8_t COW:1; //REGMODE03/R109[2]/0x00/CELL_OPEN_WIRE, 电池采集线断线检测控制位;0, 关闭;1, 开启断线检测，该bit会在1s后自动复位为0
    uint8_t CPVS:3;//REGMODE03/R109[5:3]/0x05/CP_VS[2:0], 电荷泵输出电压控制位;000, 关闭;001, 6V;010, 7V;011, 8V;100, 9V;101, 10V;110, 11V;111, 12V
    uint8_t CUWM:1;//REGMODE03/R109[6]/0X00/CUV_WATCHDOG_MODE,电池欠压看门狗屏蔽控制位;0, 开启; 1 关闭
		uint8_t R109_RVD:1;
  }R109;
 struct{
  uint8_t VAO:2;  //REGMODE03/R110[1:0]/0x01/VADC_OSR[1:0], VADC过采样率控制位;00, 64;01, 128;10, 256;11, 512
  uint8_t R110_RVD:2;
  uint8_t VAMP:2; //REGMODE03/R110[5:4]/0x00/VADC_MEAS_PERIOD[1:0], VADC同步测量周期控制位;00, 每1个CC2周期VADC测量1次;01, 每2个CC2周期VADC测量1次;10, 每4个CC2周期VADC测量1次;11, 每8个CC2周期VADC测量1次
  uint8_t VASM:1; //REGMODE03/R110[6]/0x01/VADC_SYNC_MEAS_EN, VADC与CADC CC2同步测量控制位;0, VADC连续测量;1, VADC与CADC CC2同步测量
  uint8_t VAE:1;  //REGMODE03/R110[7]/0x01/VADC_EN, VADC使能控制位;0, 关闭;1, 开启
}R110;
  uint8_t R111;
  struct{
    uint8_t COVT_H;  //REGMODE03/R112[7:0]/0x00/12位CELL_OV_TH的高8位，过压保护阈值电压=COVT*1mV+500mV
  }R112;
  struct{
    uint8_t COVD:4;  //REGMODE03/R113[3:0]/0x00/CELL_OV_DLY[3:0], 电池过压检测延迟控制位;0000, 200ms;0001, 300ms;0010, 400ms;0011, 500ms;0100, 600ms;0101, 700ms;0110, 800ms;0111, 900ms;1000, 1s;1001, 2s;1010, 3s;1011, 4s;1100, 5s;1101, 6s;1110, 7s;1111, 8s
    uint8_t COVT_L:4;//REGMODE03/R113[7:4]/0x00/12位CELL_OV_TH的低4位
  }R113;
  struct{
    uint8_t CUVT_H;  //REGMODE03/R114[7:0]/0x00/12位电池欠压检测阈值控制位的高8位，欠压保护阈值电压=CUVT*1mV
  }R114;
  struct{
    uint8_t CUVD:4;  //REGMODE03/R115[3:0]/0x00/CELL_UV_DLY[3:0], 电池欠压检测延迟控制位;0000, 200ms;0001, 300ms;0010, 400ms;0011, 500ms;0100, 600ms;0101, 700ms;0110, 800ms;0111, 900ms;1000, 1s;1001, 2s;1010, 3s;1011, 4s;1100, 5s;1101, 6s;1110, 7s;1111, 8s
    uint8_t CUVT_L:4;//REGMODE03/R115[7:4]/0x00/12位电池欠压检测阈值控制位的低4位
  }R115;
  struct{
    uint8_t GP3M:3;  //REGMODE03/R116[2:0]/0x00/GP3_MODE[2:0], GP3模式控制位;000关闭;001热敏电阻检测输入;010模拟电压检测输入;011NA;100SCD_Q;101 OCD1_Q;110 INT;111 PCHG_LS
    uint8_t GP2M:3;  //REGMODE03/R116[5:3]/0x00/GP2_MODE[2:0], GP2模式控制位;000关闭;001热敏电阻检测输入;010模拟电压检测输入;011NA;100 HALF_CLK;101 PACK_DET;110 INT;111 PDSG_LS
    uint8_t GP1M:2;  //REGMODE03/R116[7:6]/0x00/GP1_MODE[1:0], GP1模式控制位;00关闭;01热敏电阻检测输入;10模拟电压检测输入;11 CON(CHG_OFF_N), CHG驱动硬线控制，低电平关闭CHG驱动输出，高电平不影响CHG驱动输出状态
  }R116;

  struct{
    uint8_t GP6M:3; //REGMODE03/R117[2:0]/0x00/GP6_MODE[2:0], GP6模式控制位;000关闭;001热敏电阻检测输入;010模拟电压检测输入;011 NA;100 OCD2_Q;101, 0;110, INT;111, DSG_LS
    uint8_t GP5M:3; //REGMODE03/R117[5:3]/0x00/GP5_MODE[2:0], GP5模式控制位;000关闭;001热敏电阻检测输入;010模拟电压检测输入;011 NA;100 OCC1_Q;101, OCD2_Q;110, INT;111, CHG_LS
    uint8_t GP4M:2; //REGMODE03/R117[7:6]/0x00/GP4_MODE[1:0], GP4模式控制位;00关闭;01热敏电阻检测输入;10模拟电压检测输入;11, DON(DSG_OFF_N), DSG驱动硬线控制，低电平关闭DSG驱动输出，高电平不影响DSG驱动输出状态
  }R117;
  struct{
    uint8_t COTT:7;  //REGMODE03/R118[6:0]/0x00/CORE_OVER_TEMP_TH[6:0], 芯片内核过温检测阈值控制位,;0, 关闭芯片内核过温检测;其他, 阈值温度=(1466+COTT*2)*0.24467-271.03
    uint8_t COTF:1;  //REGMODE00/R118[7]/0x00/CORE_OVER_TEMP_FLAG, 芯片内核过温关机标识位;0, 未发生过芯片内核过温;1, 已发生过芯片内核过温，该bit会在读取后自动复位为0
  }R118;
  struct{
    uint8_t IWT:3;   //REGMODE03/R119[2:0]/0x00/I2C_WATCHDOG_TIMER[2:0], I2C看门狗定时器控制位;0XX, 关闭定时器;100, 定时器设为4s;101, 定时器设为8s;110, 定时器设为16s;111, 定时器设为32s
    uint8_t IWTS:1;  //REGMODE00/R119[3]/0x00/I2C_WATCHDOG_TIMEOUT_STATE, I2C看门狗超时状态位;0, 未超时;1, 已超时
    uint8_t R119_RVD4:1;
    uint8_t V3P3M:1; //REGMODE03/R119[5]/0x00/V3P3_WDTO_MODE, I2C超时重启V3P3控制位;0, I2C超时不影响V3P3输出;1, I2C超时后V3P3输出关闭1s后重启
    uint8_t V3P3EW:1; //REGMODE03/R119[6]/0x01/V3P3_EN_WORKING, 工作模式下V3P3输出控制位;0, 关闭V3P3输出;1, 开启V3P3输出
    uint8_t V3P3ES:1; //REGMODE03/R119[7]/0x00/V3P3_EN_SLEEPING, 休眠模式下V3P3输出控制位;0, 关闭V3P3输出;1, 开启V3P3输出
  }R119;
  struct{
    uint8_t TWSE:4; //REGMODE03/R120[3:0]/0x00/TIMED_WAKEUP_TIMER[3:0], 定时唤醒定时器控制位;0000, 关闭;0001, 10s;0010, 20s;0011, 30s;0100, 40s;0101, 50s;0110, 1min;0111, 2min;1000, 3min;1001, 4min;1010, 5min;1011, 6min;1100, 7min;1101, 8min;1110, 9min;1111, 10min
    uint8_t R120_RVD:3;
    uint8_t TIWK:1; //REGMODE00/R120[7]/0x00/TIMED_WAKEUP_STATE, 定时唤醒状态位;0, 关闭;1, 开启
  }R120;
  struct{
    uint8_t ISCM:1;  //REGMODE03/R121[0]/0x00/INT_SCD_MASK, 放电短路中断输出屏蔽控制位;0, 放电短路后中断输出低电平1ms;1, 放电短路后中断无输出
    uint8_t IOC2M:1; //REGMODE03/R121[1]/0x00/INT_OC2_MASK, 2级过流(包含2级放电过流和2级充电过流)中断输出屏蔽控制位;0, 2级过流后中断输出低电平1ms;1, 2级过流后中断无输出
    uint8_t IOC1M:1; //REGMODE03/R121[2]/0x00/INT_OC1_MASK, 1级过流(包含1级放电过流和1级充电过流)中断输出屏蔽控制位;0, 1级过流后中断输出低电平1ms;1, 1级过流后中断无输出
    uint8_t ICUM:1;  //REGMODE03/R121[3]/0x00/CUV_MASK, 电池欠压中断输出屏蔽控制位;0, 电池欠压后中断输出低电平1ms;1, 电池欠压后中断无输出
    uint8_t ICOM:1;  //REGMODE03/R121[4]/0x00/INT_COV_MASK, 电池过压中断输出屏蔽控制位;0, 电池过压后中断输出低电平1ms;1, 电池过压后中断无输出
    uint8_t ICCM:1;  //REGMODE03/R121[5]/0x00/INT_CADC_CC2_MASK, CADC CC2转换结束中断输出屏蔽控制位;0, CADC CC2转换结束后中断输出低电平1ms;1, CADC CC2转换结束后中断无输出
    uint8_t IVOM:1;  //REGMODE03/R121[6]/0x00/INT_VADC_OVER_MASK, VADC转换结束中断输出屏蔽控制位;0, VADC转换结束后中断输出低电平1ms;1, VADC转换结束后中断无输出
    uint8_t IWM:1;   //REGMODE03/R121[7]/0x00/INT_WKUP_MASK, 唤醒中断输出屏蔽控制位;0, 芯片唤醒后中断输出低电平1ms;1, 芯片唤醒后中断无输出
  }R121;
	uint8_t R122;
	uint8_t R123;
	uint8_t R124;
	uint8_t R125;
	struct{
		uint8_t F1RT; //REGMODE13/R126[7:0]/0x80/FUSE1_RES_TRIM[7:0], 存储在FUSE1中电阻修调值;电阻值=6800Ω+F1RT*25Ω
	}R126;
	uint8_t R127;
	uint8_t R128;
	uint8_t R129;
	uint8_t R130;
	uint8_t R131;
	uint8_t R132;
	uint8_t R133;
	uint8_t R134;
	uint8_t R135;
	uint8_t R136;
	uint8_t R137;
	uint8_t R138;
	uint8_t R139;
	uint8_t R140;
	uint8_t R141;
	uint8_t R142;
  struct{
		uint8_t CV;//REGMODE03/R143[7:0]/0x00/CHIP_VERSION[7:0], 芯片版本
	}R143;
	uint8_t R144;
}TAFERegs;
//---------------------------------------------------------------------------
#pragma pack (pop)
//用户自定义的AFE初始化寄存器配置数据 （从寄存器R81~R1210)
static const unsigned char DVC1124_PresetConfigRegData_R81To121[]={
	0x00,0x9E,0xFF,0xFF,0x2C,0xFF,0x28,0x05,0x40,	 	 	//R81-89
	0x40,0x18,0x18,0xB0,0x47,0xC7,0x18,0x18,0x50,0x10,//R90-99
	0x88,0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x68,//R100-109
	0xCD,0x28,0xE7,0x48,0x5D,0xC8,0x00,0x00,0x00,0xc0,//R110-119
	0x00,0x00															 						//R120-121
};
//------------------------------------------------------------------------------

void AFE_Iicinit(void);
void DVC1124_Sleep(void);
void DVC1124_Offmos(void);
void CleanError(void);
bool DVC1124_InitRegs(void);
bool DVC1124_WriteRegs(uint8_t regAddr,uint8_t regLen);
bool DVC1124_ReadRegs(uint8_t regAddr,uint8_t regLen);

void AFE_Handle(void);
void CurrentAutoCalibrate(void);
uint8_t ReadAfeWakeReason(void);
extern TAFERegs g_AfeRegs;
extern bool bCHGING;
extern bool bDSGING;
extern bool g_bCellCountDone;

extern float g_fSampleCur;
extern float g_fRawCur;


#endif
