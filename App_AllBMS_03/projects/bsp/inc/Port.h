#ifndef __PORT_H__
#define __PORT_H__


#include "n32wb43x.h"
#include "n32wb43x_gpio.h"
#include "n32wb43x_it.h"

#define DCDCEN_PORT        GPIOD
#define DCDCEN_PIN       GPIO_PIN_8

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
#define FSEN_PORT         		GPIOB
#define HEAT_FSEN_PIN          GPIO_PIN_1


#define HTDET_PORT              GPIOC
#define HTDET_PIN          GPIO_PIN_3

#define TIME_1S           1000
#define SHALLOW_SLEEP_TIMEOUT  (60 * TIME_1S)   // 1 minute idle -> sleep

// Wake-up source pins for shallow sleep
#define WAKE_RS485_PORT       GPIOA
#define WAKE_RS485_PIN        GPIO_PIN_8         // RS485 wake (PA8) -> EXTI8
#define WAKE_RS485_EXTI_LINE  EXTI_LINE8
#define WAKE_RS485_PORT_SRC   GPIOA_PORT_SOURCE
#define WAKE_RS485_PIN_SRC    GPIO_PIN_SOURCE8

#define WAKE_AFE_PORT         GPIOC
#define WAKE_AFE_PIN          GPIO_PIN_4         // AFE (PC4) EXTI4 falling
#define WAKE_AFE_EXTI_LINE    EXTI_LINE4
#define WAKE_AFE_PORT_SRC     GPIOC_PORT_SOURCE
#define WAKE_AFE_PIN_SRC      GPIO_PIN_SOURCE4

#define WAKE_BLE_PORT         GPIOC
#define WAKE_BLE_PIN          GPIO_PIN_7         // BLE (PC7) polled

#define WAKE_KEY_PORT         GPIOB
#define WAKE_KEY_PIN          GPIO_PIN_0         // Key (PB0) polled

#define WAKE_CAN_PORT         GPIOB
#define WAKE_CAN_PIN          GPIO_PIN_7         // CAN wake (PB7) EXTI7 falling
#define WAKE_CAN_EXTI_LINE    EXTI_LINE7
#define WAKE_CAN_PORT_SRC     GPIOB_PORT_SOURCE
#define WAKE_CAN_PIN_SRC      GPIO_PIN_SOURCE7

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
