#ifndef __PORT_H__
#define __PORT_H__


#include "n32wb43x.h"
#include "n32wb43x_gpio.h"
#include "n32wb43x_it.h"
#include "bms_config.h"     /* EN_4GCOMM: PB7 归 4G(USART1_RX) 时摘除 CAN 唤醒 */

#define DCDCEN_PORT        GPIOC
#define DCDCEN_PIN       GPIO_PIN_3

/* 注:新板 M5V(SP0M18 buck)/M3V3(AMS1117)由硬件自使能,非 MCU 控制;PC1/PC0 实为 P4 连接器信号(疑 MACC/MPRE_DSG)。
 * 固件仅在 Port_Config 置上电稳定态,已移除休眠/深睡电源轨误操作。语义/极性重定义待 PCB 网表确认(见 plans 第2层)。 */
#define M5V_CTRL_PORT      GPIOC
#define M5V_CTRL_PIN       GPIO_PIN_1

#define M3V3_CTRL_PORT     GPIOC
#define M3V3_CTRL_PIN      GPIO_PIN_0

#define LED_PORT           GPIOC
#define LED_PIN          GPIO_PIN_5

#define AFE_PORT         GPIOC
#define AFE_MCUIO_PIN    GPIO_PIN_2	
#define YDG              GPIO_PIN_13

#define HEAT_PORT              GPIOA
#define HEAT_HTCTRL_PIN        GPIO_PIN_5
#define FSEN_PORT         		GPIOA
#define HEAT_FSEN_PIN          GPIO_PIN_8


#define HTDET_PORT              GPIOD
#define HTDET_PIN          GPIO_PIN_8

#define TIME_1S           1000
#define SHALLOW_SLEEP_TIMEOUT  (60 * TIME_1S)   // 1 minute idle -> sleep

// Wake-up source pins for shallow sleep

#define WAKE_AFE_PORT         GPIOC
#define WAKE_AFE_PIN          GPIO_PIN_4         // AFE (PC4) EXTI4 falling
#define WAKE_AFE_EXTI_LINE    EXTI_LINE4
#define WAKE_AFE_PORT_SRC     GPIOC_PORT_SOURCE
#define WAKE_AFE_PIN_SRC      GPIO_PIN_SOURCE4

#define WAKE_BLE_PORT         GPIOC
#define WAKE_BLE_PIN          GPIO_PIN_7         // BLE (PC7) polled

#define WAKE_KEY_PORT         GPIOB
#define WAKE_KEY_PIN          GPIO_PIN_2         // Key (PB2) polled


#define WAKE_UART_PORT        GPIOD
#define WAKE_UART_PIN         GPIO_PIN_2         // UART5 RX (PD2) via UART RX interrupt

#define WAKE_CHG_PORT         GPIOA
#define WAKE_CHG_PIN          GPIO_PIN_6         // Charger PA6 polled
#define WAKE_LOAD_PORT        GPIOA
#define WAKE_LOAD_PIN         GPIO_PIN_7         // Load PA7 polled

void PortInit(GPIO_Module* GPIOx, uint16_t Pin, GPIO_ModeType GPIO_Mode);
void Port_Config(void);
void System_operation(void);
void Sleep_WakeupConfig(void);
void Sleep_PeripheralDisable(void);
void Sleep_PeripheralRestore(void);
void Shallow_Sleep(void);
void Deep_Sleep(void);

extern bool shallow_sleeping;
extern bool deep_sleeping;

#endif
