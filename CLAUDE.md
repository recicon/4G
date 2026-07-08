# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Bare-metal firmware for a battery-management system (BMS) built on the **N32WB43x** MCU
(Nations Technologies, Cortex-M4, 128 KB Flash / 32 KB RAM, integrated BLE). A single
unified codebase produces two product variants — **17S** and **24S** (17- vs 24-cell packs) —
selected at compile time. All firmware lives under `App_AllBMS_03/`; `doc/` holds Altium
hardware design files (`.SchDoc`/`.PcbDoc`/`.PrjPcb`) and is not built.

Source comments and `App_AllBMS_03/README.md` are in Chinese; identifiers are English
Hungarian notation (see Conventions).

## Build

Toolchain is **Keil MDK-ARM** (armcc). There is no host build, no test framework, and no
CI — firmware is verified by building in Keil and flashing to hardware. Do not invent test
commands.

- **Project file:** `App_AllBMS_03/projects/MDK-ARM/BMS_Unified.uvprojx`
- **Two Keil targets**, differing only by a preprocessor define:
  - `BMS_24S100AQ` → `BMS_MODEL=MODEL_24S100AQ`
  - `BMS_17S100AQ` → `BMS_MODEL=MODEL_17S100AQ`
- **GUI build:** open the project, pick the target, F7.
- **CLI build:** `cd App_AllBMS_03 && build.bat 24S100AQ` (or `17S100AQ`). Edit `KEIL_PATH`
  in `build.bat` first to point at the local `UV4.exe`. Output lands in
  `projects/MDK-ARM/Objects/`; build log is `build_log.txt`.

## Model selection & variant handling

This is the single most important thing to understand before editing anything.

- **`projects/bsp/inc/bms_config.h` is the product-configuration hub.** `BMS_MODEL`
  (injected by the Keil target's `-D`) picks a `#if` block that sets cell count
  (`N_PACK_CELL`), capacity, current limits, version strings, current-sense/temperature
  calibration, etc. Common (model-independent) parameters — battery chemistry
  (`BAT_TYPE`), pack type, feature switches, protection thresholds, bootloader layout —
  live in the shared section below the per-model blocks. **Change product behavior here
  first**, not by scattering constants through the code.
- **Only two source files carry real `#if (BMS_MODEL == ...)` divergence:**
  `projects/bsp/src/DVC1124.c` (AFE cell-to-channel mapping — 17S uses a non-contiguous
  map `{11,13,15,17,19,21,23}` for cells 10–17) and `projects/bsp/src/Protection.c`
  (cell-count enable logic — 17S uses a lookup table, 24S computes bitmasks). When adding a
  variant or touching cell topology, these are the files to audit.
- **Feature switches** in `bms_config.h`: `EN_BOOTLOADER`, `EN_BLUCOMM`, `EN_4GCOMM`.
  Note `EN_4GCOMM` is currently **0** — despite the repo being named `4G`, 4G comms are
  compiled out; BLE (`EN_BLUCOMM=1`) is the active wireless link.

## Runtime architecture

**Bare-metal cooperative super-loop — there is no RTOS.** `middlewares/rt-thread/` is
vendored but **not referenced by the application** (no `rtconfig.h`, no `#include
<rtthread.h>` under `projects/`); ignore it unless deliberately wiring RT-Thread in.

Control flow lives entirely in `projects/src/main.c`:

1. `Init_Handle()` — one-shot bring-up of every subsystem in order (clock, ports, UART/485,
   ADC, AFE I2C, SIF, EEPROM I2C, app state, fault levels, AFE params, CAN, BLE, current-drift
   calibration, flash/EEPROM memory).
2. `init_all_tasks()` — registers periodic work in a tiny software scheduler:
   `register_timer_task(interval_ms, fn)` fills a fixed `g_tasks[10]` array;
   `execute_timer_tasks(now)` dispatches any task whose interval has elapsed.
   **To add periodic work, register it here** (e.g. `AFE_Handle` @20 ms, `SocEstimation`
   @50 ms, `ExecBleMsgProcess` @50 ms, `LogPeriodicRecord` @1 s). The array is capped at 10 —
   registrations beyond that are silently dropped.
3. The `while(1)` loop calls `execute_timer_tasks()` then the always-run steps:
   `MosCtrl()`, `ble_loop()`, `CanTxRx()`, `Shallow_Sleep()`, `Deep_Sleep()`.

**Central data model** (globals threaded through nearly every module; declared `extern` in
`projects/bsp/inc/App.h`, structs in `UserDef.h`):
- `g_tBatPackInfo` (`tBatPackInfo`) — live measured state: per-cell voltages/temps, current,
  SOC, control mode, fault levels, power limits.
- `g_tBatPackCtrl` (`tBatPackCtrl`) — control state + persisted EEPROM params
  (`tEERunParam` run stats, `tEESpecParam` spec/calibration). CRC-checked.
- `g_tFaultBitInfo`, `g_tFaultDataParam` — fault flags and per-fault upper/lower thresholds.

## Code layout (`App_AllBMS_03/`)

- `firmware/` — **vendor MCU libraries, do not edit**: `CMSIS/` (core + N32WB43x device,
  startup, linker script), `n32wb43x_std_periph_driver/`, `n32wb43x_usbfs_driver/`,
  `n32wb43x_algo_lib/` + `n32wb43x_periph_lib/` (prebuilt `.lib`: AES/DES/HASH/RNG, RCC
  trimming).
- `projects/bsp/` — the board/application layer, where almost all real work happens:
  - `DVC1124.c` AFE driver (cell voltage acquisition, balancing, current sampling over
    software I2C), `Protection.c` (fault detection & levels), `Soc.c` (SOC estimation),
    `Temperature.c`, `Port.c` (GPIO/pin map — see `Port.h` for the hardware pinout),
    `MemoryModule.c` + `spi_flash.c` + `software_i2c.c` (EEPROM/flash persistence),
    `ModbusPro.c`/`ModbusProEx.c` + `Usart_Com.c` (host protocol over 485),
    `User_Can_Config.c` (CAN), `SifModule.c` (SIF single-wire output), `TimeModule.c`/
    `time_config.c` (ticks/RTC), `App.c` (app init, fault-level tables, message dispatch,
    OTA/log records, current-drift calibration).
- `projects/ble/` — BLE application on the Nations stack: `ble.c` (`ble_init`/`ble_loop`),
  `app_proto_*` (framed command protocol), `app_uart.c`, `app_flash.c`, `app_user_svc0.c`
  (GATT service), `app_timer.c`.
- `projects/src/` — `main.c` (scheduler + entry) and `n32wb43x_it.c` (ISRs).
- `projects/inc/` — `main.h` (aggregate include used everywhere).

## Firmware update (OTA / bootloader)

Gated by `EN_BOOTLOADER` in `bms_config.h`; flash map is defined in `App.h`. Layout:
bootloader at flash base (`BOOT_SIZE 0x4000`), application at `FLASH_BASE + BOOT_SIZE`, with
update flags/param and a run-address slot in the reserved region (`UPDATE_PRAM 0x1F800`).
The update handshake/transfer uses the `en_packet_cmd_t` / `en_packet_status_t` command set
(handshake, erase, download, CRC-check, jump-to-app) — see `App.h`.

## Conventions

- **Hungarian prefixes:** `by`=uint8, `w`=uint16, `dw`=uint32, `b`=bit/bool, `f`=float,
  `e`=enum, `t`=struct/typedef, `u`=union, `g_`=global. Match the surrounding style.
- Physical-unit fixed-point encodings are pervasive (currents/capacity in 0.1 A / 0.1 Ah,
  temperatures offset by +40 °C, voltages in mV). Check the neighboring `#define` comments
  in `bms_config.h` before assuming raw units.
- Persisted structs (`tEERunParam`, `tEESpecParam`, `tFaultDataParam`, `tBleName`) end with a
  `wCrc` — keep CRC computation/validation in sync when changing their fields.
