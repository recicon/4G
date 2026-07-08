# BMS 统一代码库

本项目将2个BMS产品型号的代码合并为一个统一的代码库，通过编译时宏定义区分不同型号。

## 支持的型号

| 型号 | 电芯数 | 版本 |
|------|--------|------|
| BMS_17S100AQ | 17S | V1.4 |
| BMS_24S100AQ | 24S | V1.6 |

## 项目结构

```
BMS_Unified/
├── firmware/              # MCU固件库（公共）
├── middlewares/           # 中间件（公共）
├── projects/
│   ├── bsp/               # BSP层代码
│   │   ├── inc/           # 头文件
│   │   │   ├── bms_config.h   # **产品型号配置文件**
│   │   │   └── UserDef.h      # 用户定义头文件
│   │   └── src/           # 源代码
│   │       ├── DVC1124.c      # AFE驱动（含条件编译）
│   │       └── Protection.c   # 保护逻辑（含条件编译）
│   ├── ble/               # BLE应用代码
│   ├── inc/               # 项目头文件
│   ├── src/               # 主程序代码
│   └── MDK-ARM/           # Keil项目文件
│       └── BMS_Unified.uvprojx
├── build.bat              # 命令行编译脚本
└── README.md
```

## 编译方法

### 方法1：使用Keil GUI

1. 打开 `projects/MDK-ARM/BMS_Unified.uvprojx`
2. 在Project窗口选择对应的Target：
   - `BMS_17S100AQ` - 17S100AQ型号
   - `BMS_24S100AQ` - 24S100AQ型号
3. 点击Build (F7) 编译

### 方法2：使用命令行

```bash
# 编译24S100AQ型号
build.bat 24S100AQ

# 编译17S100AQ型号
build.bat 17S100AQ
```

注意：需要先修改 `build.bat` 中的 `KEIL_PATH` 为实际的Keil安装路径。

## 配置说明

所有产品相关的配置参数都在 `projects/bsp/inc/bms_config.h` 中定义：

### 基本参数
- `N_PACK_CELL` - 电芯数量
- `N_BATT_CAPACITY` - 电池容量
- `PACK_CHG_MAX_CUR` - 最大充电电流
- `PACK_DSG_MAX_CUR` - 最大放电电流
- `VERSION_PRODUCT` - 产品版本字符串

### AFE电流采样参数
- `CURR_LSB_CC2` - CC2采样分辨率 (V/bit)
- `CURR_SENSE_RESISTANCE_mR` - 采样电阻值 (mR)

### 温度采集参数
- `TEMP_REGAI_VOLT` - 温度基准电压 (mV)

## 代码差异处理

### 17S特殊处理

17S型号的AFE电芯映射与24S不同，主要差异在以下文件：

**DVC1124.c** - 电压采集和均衡控制：
```c
#if (BMS_MODEL == MODEL_17S100AQ)
    // 17S特殊电芯映射: 电芯10-17对应AFE位置11,13,15,17,19,21,23
    uint8_t mapping[7] = {11, 13, 15, 17, 19, 21, 23};
#else
    // 24S标准模式
#endif
```

**Protection.c** - 电芯数量配置：
```c
#if (BMS_MODEL == MODEL_17S100AQ)
    // 17S: 支持4-17节，使用查找表
    uint8_t cm_l_table[] = {0x55, 0xD5, 0xF5, 0xFD, 0x54, 0xD4, 0xF4, 0xFC, 0xFE};
#else
    // 24S: 支持4-24节，使用位计算
#endif
```

## MCU信息

- 芯片：N32WB43x (Cortex-M4)
- Flash：128KB
- RAM：32KB
- BLE：内置蓝牙协议栈

## 原始项目

原项目文件保留在以下目录：
- `D:\AIcode\all\BMS_17S100AQV1.4`
- `D:\AIcode\all\BMS_24S100AQV1.6`
