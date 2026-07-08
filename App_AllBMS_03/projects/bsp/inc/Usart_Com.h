#ifndef __USART_COM_H__
#define __USART_COM_H__
#include "n32wb43x_usart.h"
#include "n32wb43x_rcc.h"
#include"port.h"
#include "software_i2c.h"

#define UART_TX_MAX_SIZE				(256)	
#define UART_RD_MAX_SIZE				(530)	


#define USART485            USART3
#define USART485_GPIO       GPIOC
#define USART485_CLK        RCC_APB1_PERIPH_USART3
#define USART485_GPIO_CLK   RCC_APB2_PERIPH_GPIOC
#define USART485_RxPin      GPIO_PIN_11
#define USART485_TxPin      GPIO_PIN_10
#define USART485_Rx_GPIO_AF GPIO_AF5_USART3
#define USART485_Tx_GPIO_AF GPIO_AF5_USART3
#define USART485_APBxClkCmd RCC_EnableAPB1PeriphClk
#define USART485_IRQn       USART3_IRQn
#define USART485_IRQHandler USART3_IRQHandler
#define UARTTxRxm_PORT    GPIOA
#define UARTTxRxm_PIN     GPIO_PIN_15  


#define UART                UART5
#define UART_CLK            RCC_APB2_PERIPH_UART5
#define UART_GPIO_CLK      (RCC_APB2_PERIPH_GPIOD | RCC_APB2_PERIPH_GPIOC)
#define UART_RxPin          GPIO_PIN_2
#define UART_RxPort         GPIOD
#define UART_TxPin          GPIO_PIN_12
#define UART_TxPort         GPIOC
#define UART_Rx_GPIO_AF     GPIO_AF6_UART5
#define UART_Tx_GPIO_AF     GPIO_AF6_UART5
#define UART_APBxClkCmd     RCC_EnableAPB2PeriphClk
#define UART_IRQn           UART5_IRQn
#define UART_IRQHandler     UART5_IRQHandler



#define ARRAY_SZ( X )  (sizeof(X) / sizeof((X)[0]))

typedef struct
{
	uint8_t wbuf[UART_TX_MAX_SIZE];
	uint8_t rbuf[UART_RD_MAX_SIZE];
	uint16_t w_index;
	int16_t r_index;
	uint16_t w_num;
	uint16_t Timeout_ms;
	uint16_t Need_ms;
} sUARTMsgType;

void Usart485_Init(void);
void RS485_Send_Data(uint8_t *buf, uint16_t len);
void Uart_Init(void);

extern sUARTMsgType g_sMainRtxMsg;
extern sUARTMsgType g_sBleRtxMsg;
extern sUARTMsgType g_sUartRtxMsg;


#endif
