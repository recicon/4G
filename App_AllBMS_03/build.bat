@echo off
REM BMS统一项目编译脚本
REM 使用方法: build.bat [型号]
REM 型号: 17S100AQ, 24S100AQ
REM 示例: build.bat 24S100AQ

setlocal

REM 设置Keil路径（请根据实际安装路径修改）
set KEIL_PATH=D:\Program Files\Keil_v5\UV4\UV4.exe
if not exist "%KEIL_PATH%" (
    echo Keil not found at %KEIL_PATH%
    echo Please modify KEIL_PATH in this script
    pause
    exit /b 1
)

REM 设置项目路径
set PROJECT_PATH=%~dp0projects\MDK-ARM\BMS_Unified.uvprojx

REM 检查参数
if "%1"=="" (
    echo Available targets:
    echo   17S100AQ - 17S 100A BMS
    echo   24S100AQ - 24S 100A BMS
    echo.
    echo Usage: build.bat [target]
    echo Example: build.bat 24S100AQ
    pause
    exit /b 1
)

REM 设置目标
set TARGET=BMS_%1

echo Building %TARGET%...
echo.

REM 编译项目
"%KEIL_PATH%" -j0 -b "%PROJECT_PATH%" -t "%TARGET%" -o build_log.txt

REM 检查编译结果
if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo Output: projects\MDK-ARM\Objects\%1.bin
) else (
    echo Build failed! Check build_log.txt for details.
)

type build_log.txt
pause
