#include "spi_flash.h"
#include "n32wb43x_it.h"

/* 全双工收发一字节：等发送缓冲空 -> 写入 -> 等接收非空 -> 读走接收字节。
 * 读走 RX 是必须的：SPI 每发 1 字节同时收 1 字节，命令/地址阶段产生的接收字节
 * 若不消费，会残留在接收寄存器里，导致后续读取的数据整体错位、CRC 校验失败。 */
static uint8_t SpiFlash_Transfer(uint8_t out)
{
    while (SPI_I2S_GetStatus(SPI_FLASH_SPI, SPI_I2S_TE_FLAG) == RESET);
    SPI_I2S_TransmitData(SPI_FLASH_SPI, out);
    while (SPI_I2S_GetStatus(SPI_FLASH_SPI, SPI_I2S_RNE_FLAG) == RESET);
    return (uint8_t)SPI_I2S_ReceiveData(SPI_FLASH_SPI);
}

/* 等总线传输真正完成（BUSY 清零）后再拉高 CS。
 * TE/TXE 置位仅表示数据进了移位寄存器，此时拉 CS 会截断最后一个字节：
 * WREN 被截断会导致写使能位(WEL)不置位，后续 Page Program / Sector Erase 被静默忽略。 */
static void SpiFlash_CsHigh(void)
{
    while (SPI_I2S_GetStatus(SPI_FLASH_SPI, SPI_I2S_BUSY_FLAG) != RESET);
    SPI_FLASH_CS_HIGH();
}

static void SpiFlash_WriteEnable(void)
{
    SPI_FLASH_CS_LOW();
    SpiFlash_Transfer(CMD_WREN);
    SpiFlash_CsHigh();
}

static uint8_t SpiFlash_ReadSR(void)
{
    uint8_t sr;
    SPI_FLASH_CS_LOW();
    SpiFlash_Transfer(CMD_RDSR);
    sr = SpiFlash_Transfer(0xFF);
    SpiFlash_CsHigh();
    return sr;
}

/* 发送命令 + 3 字节地址；CS 保持拉低，供调用方继续收发数据后再 SpiFlash_CsHigh()。 */
static void SpiFlash_SendCmdAddr(uint8_t byCmd, uint32_t dwAddr)
{
    SPI_FLASH_CS_LOW();
    SpiFlash_Transfer(byCmd);
    SpiFlash_Transfer((uint8_t)(dwAddr >> 16));
    SpiFlash_Transfer((uint8_t)(dwAddr >> 8));
    SpiFlash_Transfer((uint8_t)dwAddr);
}

void SpiFlash_WaitBusy(void)
{
    while (SpiFlash_ReadSR() & 0x01);
}

void SpiFlash_Init(void)
{
    GPIO_InitType GPIO_InitStructure;
    SPI_InitType SPI_InitStructure;

    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB | RCC_APB2_PERIPH_SPI1 | RCC_APB2_PERIPH_GPIOA, ENABLE);

    GPIO_InitStruct(&GPIO_InitStructure);
    GPIO_InitStructure.Pin       = SPI_FLASH_CS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pull = GPIO_No_Pull;
    GPIO_InitPeripheral(SPI_FLASH_CS_GPIO, &GPIO_InitStructure);
    SPI_FLASH_CS_HIGH();

    GPIO_InitStructure.Pin            = SPI_FLASH_SCK_PIN | SPI_FLASH_MISO_PIN;
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF1_SPI1;
    GPIO_InitPeripheral(SPI_FLASH_GPIO, &GPIO_InitStructure);

		 GPIO_InitStructure.GPIO_Alternate = GPIO_AF0_SPI1;
    GPIO_InitStructure.Pin            = SPI_FLASH_MOSI_PIN;
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
    GPIO_InitPeripheral(SPI_FLASH_GPIO, &GPIO_InitStructure);

    SPI_InitStruct(&SPI_InitStructure);
    SPI_InitStructure.DataDirection = SPI_DIR_DOUBLELINE_FULLDUPLEX;
    SPI_InitStructure.SpiMode       = SPI_MODE_MASTER;
    SPI_InitStructure.DataLen       = SPI_DATA_SIZE_8BITS;
    SPI_InitStructure.CLKPOL        = SPI_CLKPOL_LOW;
    SPI_InitStructure.CLKPHA        = SPI_CLKPHA_FIRST_EDGE;
    SPI_InitStructure.NSS           = SPI_NSS_SOFT;
    SPI_InitStructure.BaudRatePres  = SPI_BR_PRESCALER_4;
    SPI_InitStructure.FirstBit      = SPI_FB_MSB;
    SPI_Init(SPI_FLASH_SPI, &SPI_InitStructure);

    SPI_Enable(SPI_FLASH_SPI, ENABLE);
}

/* 读 JEDEC ID（厂商 + 容量），用于上电自检确认 SPI 物理通信正常。
 * ZD25Q16 应返回非 0x00/0xFF 的有效 ID；全 0 或全 FF 表示通信异常。 */
uint8_t SpiFlash_ReadID(uint8_t *pId)
{
    SPI_FLASH_CS_LOW();
    SpiFlash_Transfer(CMD_RDID);
    pId[0] = SpiFlash_Transfer(0xFF);   /* Manufacturer ID */
    pId[1] = SpiFlash_Transfer(0xFF);   /* Memory type */
    pId[2] = SpiFlash_Transfer(0xFF);   /* Capacity */
    SpiFlash_CsHigh();
    return 1;
}

uint8_t SpiFlash_Read(uint32_t dwAddr, uint8_t *pBuf, uint32_t dwLen)
{
    uint32_t i;

    if (dwLen == 0) return 1;

    SpiFlash_SendCmdAddr(CMD_READ, dwAddr);

    for (i = 0; i < dwLen; i++)
        pBuf[i] = SpiFlash_Transfer(0xFF);

    SpiFlash_CsHigh();
    return 1;
}

uint8_t SpiFlash_Write(uint32_t dwAddr, const uint8_t *pBuf, uint32_t dwLen)
{
    uint32_t i, remaining, chunk;
    uint32_t offset = 0;

    while (offset < dwLen)
    {
        remaining = dwLen - offset;
        chunk = (remaining > SPI_FLASH_PAGE_SIZE) ? SPI_FLASH_PAGE_SIZE : remaining;

        uint32_t pageEnd = (dwAddr + offset) & ~(SPI_FLASH_PAGE_SIZE - 1);
        pageEnd += SPI_FLASH_PAGE_SIZE;
        if ((dwAddr + offset + chunk) > pageEnd)
            chunk = pageEnd - (dwAddr + offset);

        SpiFlash_WriteEnable();
        SpiFlash_SendCmdAddr(CMD_PP, dwAddr + offset);

        for (i = 0; i < chunk; i++)
            SpiFlash_Transfer(pBuf[offset + i]);

        SpiFlash_CsHigh();
        SpiFlash_WaitBusy();

        offset += chunk;
    }
    return 1;
}

uint8_t SpiFlash_EraseSector(uint32_t dwAddr)
{
    SpiFlash_WriteEnable();
    SpiFlash_SendCmdAddr(CMD_SE, dwAddr);
    SpiFlash_CsHigh();
    SpiFlash_WaitBusy();
    return 1;
}
