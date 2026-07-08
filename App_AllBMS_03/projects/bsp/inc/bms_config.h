/**
 * @file bms_config.h
 * @brief BMS产品型号配置文件
 *
 * 通过定义BMS_MODEL选择产品型号，编译时自动配置相应参数
 *
 * 使用方法：
 *   方式1：在编译选项中定义 -DBMS_MODEL=MODEL_24S100AQ
 *   方式2：取消下方注释选择型号
 */

#ifndef __BMS_CONFIG_H__
#define __BMS_CONFIG_H__

/*============================================================================
 *                          产品型号选择
 *============================================================================*/
// 取消注释选择对应型号（仅选择一个）
// #define BMS_MODEL           MODEL_17S100AQ
// #define BMS_MODEL           MODEL_24S100AQ

/*============================================================================
 *                          型号定义
 *============================================================================*/
#define MODEL_17S100AQ       1
#define MODEL_24S100AQ       2

// 默认型号（未定义时使用）
#ifndef BMS_MODEL
    #define BMS_MODEL       MODEL_24S100AQ
    #warning "BMS_MODEL未定义，使用默认型号 MODEL_24S100AQ"
#endif

/*============================================================================
 *                          产品配置表
 *============================================================================*/
#if (BMS_MODEL == MODEL_17S100AQ)
    /* ========== 17S100AQ 配置 ========== */
    #define PRODUCT_VER                 1u
    #define VERSION_PRODUCT             {"H17S100A4TA"}
    #define VERSION_MAJOR               0
    #define VERSION_MINOR               1
    #define VERSION_BUILD               4

    /* 硬件参数 */
    #define N_PACK_CELL                 17          /* 电芯数量 */
    #define N_PACK_TEMP                 4           /* 温度采集点数 */
    #define N_BATT_CAPACITY             750         /* 电池容量 0.1Ah */
    #define PACK_CHG_MAX_CUR            3200        /* 最大充电电流 0.1A */
    #define PACK_DSG_MAX_CUR            3200        /* 最大放电电流 0.1A */

    /* AFE电流采样参数 */
    #define CURR_LSB_CC2               (0.3125e-6)  /* CC2 LSB: V/bit */
    #define CURR_SENSE_RESISTANCE_mR   (0.25)       /* 采样电阻 mR */

    /* 温度采集参数 */
    #define TEMP_REGAI_VOLT            3300         /* 温度基准电压 mV */

#elif (BMS_MODEL == MODEL_24S100AQ)
    /* ========== 24S100AQ 配置 ========== */
    #define PRODUCT_VER                 1u
    #define VERSION_PRODUCT             {"H24S100A4TA"}
    #define VERSION_MAJOR               0
    #define VERSION_MINOR               1
    #define VERSION_BUILD               7

    /* 硬件参数 */
    #define N_PACK_CELL                 24          /* 电芯数量 */
    #define N_PACK_TEMP                 4           /* 温度采集点数 */
    #define N_BATT_CAPACITY             750         /* 电池容量 0.1Ah */
    #define PACK_CHG_MAX_CUR            3200        /* 最大充电电流 0.1A */
    #define PACK_DSG_MAX_CUR            3200        /* 最大放电电流 0.1A */

    /* AFE电流采样参数 */
    #define CURR_LSB_CC2               (0.3125e-6)  /* CC2 LSB: V/bit */
    #define CURR_SENSE_RESISTANCE_mR   (0.25)       /* 采样电阻 mR */

    /* 温度采集参数 */
    #define TEMP_REGAI_VOLT            3300         /* 温度基准电压 mV */

#else
    #error "未知的BMS型号，请检查BMS_MODEL定义"
#endif

/*============================================================================
 *                          公共配置
 *============================================================================*/
/* AFE配置 */
#define N_LECU_NUM              1               /* AFE数量 */

/* 功能开关 */
#define EN_BOOTLOADER           1               /* 使能Bootloader */
#define EN_BLUCOMM              1               /* 使能蓝牙 */
#define EN_4GCOMM               0               /* 使能4G */

/* 循环配置 */
#define N_PACK_LOOP_TIME        3000            /* 主循环周期ms */
#define N_PACK_LOOP_DEC         0.8f

/* 并联数 */
#define N_BATT_PARALLEL         1

/* 电池包类型 */
#define BATTERY_PACK_EV         1               /* 电动车电池包 */
#define BATTERY_PACK_HS         2               /* 储能电池包 */
#define BATTERY_PACK_TYPE       BATTERY_PACK_HS

/* 电池类型 */
#define BAT_TYPE_FELI           0               /* 磷酸铁锂 */
#define BAT_TYPE_NCM            1               /* 三元锂 */
#define BAT_TYPE                BAT_TYPE_FELI

/* 兼容旧定义 */
#define FeLi90Ah                BAT_TYPE_FELI
#define NcmLi90Ah               BAT_TYPE_NCM

/* 最大值定义 */
#define N_PACK_CELL_MAX         32              /* 最大电芯数 */
#define N_PACK_TEMP_MAX         12              /* 最大温度采集点 */

/* 密码定义 */
#define PASSWORD_DIAG           0xC051
#define PASSWORD_PRO_DIAG       0xC052
#define PASSWORD_XCP            0xF051
#define PASSWORD_BMS_ID         0xF052

/* 版本号计算宏 */
#define PROCESS_VERSION(cbMainVer,cbSubVer,cbBuildVer)    \
    ((uint32_t)(((PRODUCT_VER) << 24) | ((cbMainVer) << 16) | ((cbSubVer) << 8) | (cbBuildVer)))

#define VERSION_PROCESS         PROCESS_VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD)

/*============================================================================
 *                          电池参数配置
 *============================================================================*/
 #define MAX_VOLT_BATT_HIGH_UPPER				876
#define MAX_VOLT_BATT_HIGH_LOWER				852
/* 磷酸铁锂参数 */
#define FELI_RATED_CELL_VOLT            3200
#define FELI_BMS_BALANCE_VOLT           3300
#define FELI_BMS_BALANCE_DIFF           15
#define FELI_SLEFCHEK_CELL_VOLTAGE_UPPER    3700
#define FELI_SLEFCHEK_CELL_VOLTAGE_LOWER    2350
#define FELI_SLEFCHEK_TEMPERATURE_UPPER      (75 + 40)
#define FELI_SLEFCHEK_TEMPERATURE_LOWER      (-20 + 40)
#define FELI_VOLT_CELL_LOW_CHG          2000

/* 三元锂参数 */
#define NCM_RATED_CELL_VOLT             3800
#define NCM_BMS_BALANCE_VOLT            3910
#define NCM_BMS_BALANCE_DIFF            50
#define NCM_SLEFCHEK_CELL_VOLTAGE_UPPER     4300
#define NCM_SLEFCHEK_CELL_VOLTAGE_LOWER     2800
#define NCM_SLEFCHEK_TEMPERATURE_UPPER       (63 + 40)
#define NCM_SLEFCHEK_TEMPERATURE_LOWER       (-20 + 40)
#define NCM_VOLT_CELL_LOW_CHG           2500

/* SOC配置 */
#define SOC_CHG_COMP_LIGHTLY_LOWER      95

/*============================================================================
 *                          常用宏定义
 *============================================================================*/
#ifndef ABS
#define ABS(x)                  ((x) > 0 ? (x) : -(x))
#endif
#ifndef MIN
#define MIN(x, y)               ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y)               ((x) > (y) ? (x) : (y))
#endif

/*============================================================================
 *                          调试信息
 *============================================================================*/
#if (BMS_MODEL == MODEL_17S100AQ)
    #define BMS_MODEL_NAME      "BMS_17S100AQ"
#elif (BMS_MODEL == MODEL_24S100AQ)
    #define BMS_MODEL_NAME      "BMS_24S100AQ"
#endif

#endif /* __BMS_CONFIG_H__ */
