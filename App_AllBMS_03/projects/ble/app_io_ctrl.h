#include "n32wb43x.h"


#ifndef APP_IO_CTRL_H_
#define APP_IO_CTRL_H_
//#include "n32wb03x.h"


///use for hci 
#define USARTy            UART4
#define USARTy_GPIO       GPIOB
#define USARTy_CLK        RCC_APB2_PERIPH_UART4
#define USARTy_GPIO_CLK   RCC_APB2_PERIPH_GPIOB
#define USARTy_RxPin      GPIO_PIN_15
#define USARTy_TxPin      GPIO_PIN_14
#define USARTy_Rx_GPIO_AF GPIO_AF6_UART4
#define USARTy_Tx_GPIO_AF GPIO_AF6_UART4
#define USARTy_APBxClkCmd RCC_EnableAPB2PeriphClk
#define USARTy_IRQn       UART4_IRQn
#define USARTy_IRQHandler UART4_IRQHandler

#define GPIO_APBxClkCmd   RCC_EnableAPB2PeriphClk
#define USART_APBxClkCmd  RCC_EnableAPB2PeriphClk

#define USARTy_DAT_Base       (UART4_BASE + 0x04)
#define USARTy_Rx_DMA_Channel DMA_CH1
#define USARTy_Rx_DMA_REMAP   DMA_REMAP_UART4_RX

extern uint16_t  rx_out_pos;
extern void app_uart_init(void);
extern uint16_t get_unhdl_data_len(void);  //store_data_pos;
extern void app_send_byte_array(uint8_t * buf, uint16_t length);
extern void fetch_data(uint8_t recv_len , uint8_t * recv_data);
extern uint8_t fetch_one_byte_data(void);

#endif ///APP_IO_CTRL_H_
