#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

#include "n32wb43x.h"
#include "n32wb43x_spi.h"
#include "n32wb43x_gpio.h"
#include "n32wb43x_rcc.h"

/* SPI1 总线(GPIOB): PB3=SCK, PB4=MISO, PB5=MOSI; CS=PA0(移出 PB6, 让出 4G USART1_TX) */
#define SPI_FLASH_SPI           SPI1
#define SPI_FLASH_GPIO          GPIOB
#define SPI_FLASH_CS_GPIO       GPIOA       /* CS 独立在 GPIOA(PA0), 与总线 GPIOB 分离 */
#define SPI_FLASH_SCK_PIN       GPIO_PIN_3
#define SPI_FLASH_MISO_PIN      GPIO_PIN_4
#define SPI_FLASH_MOSI_PIN      GPIO_PIN_5
#define SPI_FLASH_CS_PIN        GPIO_PIN_0

#define SPI_FLASH_CS_LOW()      GPIO_ResetBits(SPI_FLASH_CS_GPIO, SPI_FLASH_CS_PIN)
#define SPI_FLASH_CS_HIGH()     GPIO_SetBits(SPI_FLASH_CS_GPIO, SPI_FLASH_CS_PIN)

/* ZD25Q16 commands */
#define CMD_WREN                0x06
#define CMD_WRDI                0x04
#define CMD_RDSR                0x05
#define CMD_READ                0x03
#define CMD_PP                  0x02
#define CMD_SE                  0x20
#define CMD_CE                  0xC7
#define CMD_RDID                0x9F

#define SECTOR_SIZE             4096
#define SPI_FLASH_PAGE_SIZE     256

void SpiFlash_Init(void);
uint8_t SpiFlash_ReadID(uint8_t *pId);
uint8_t SpiFlash_Read(uint32_t dwAddr, uint8_t *pBuf, uint32_t dwLen);
uint8_t SpiFlash_Write(uint32_t dwAddr, const uint8_t *pBuf, uint32_t dwLen);
uint8_t SpiFlash_EraseSector(uint32_t dwAddr);
void SpiFlash_WaitBusy(void);

#endif
