/*****************************************************************************
 * Copyright (c) 2019, Nations Technologies Inc.
 *
 * All rights reserved.
 * ****************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Nations' name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY NATIONS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL NATIONS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************/

/**
 * @file main.c
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2019, Nations Technologies Inc. All rights reserved.
 */
#include "main.h"
#include "n32g030.h"
#include "log.h"

#include <string.h>

/** @addtogroup N32G43X_StdPeriph_Examples
 * @{
 */

/** @addtogroup DMA_SPI_RAM
 * @{
 */

typedef enum
{
    FAILED = 0,
    PASSED = !FAILED
} Status;

#define BUFFER_SIZE    32
#define CRC_POLYNOMIAL 7

SPI_InitType SPI_InitStructure;
DMA_InitType DMA_InitStructure;
uint8_t SPI_Master_Rx_Buffer[BUFFER_SIZE], SPI_Slave_Rx_Buffer[BUFFER_SIZE];
volatile uint8_t SPI_MASTERCRCValue = 0, SPI_SLAVECRCValue = 0;
volatile Status TransferStatus1 = FAILED, TransferStatus2 = FAILED;

uint8_t SPI_Master_Tx_Buffer[BUFFER_SIZE] = {0xf1, 0x2e, 0xf1, 0xaf, 0x08, 0xa2, 0x41, 0xb4, 0xc8, 0x19, 0x38,
                                             0xb7, 0xd9, 0xdb, 0x9a, 0x64, 0x1f, 0xd6, 0x99, 0x9c, 0x0f, 0xae,
                                             0x84, 0xab, 0xda, 0x12, 0x95, 0x6c, 0xdb, 0xec, 0x06, 0x08};

uint8_t SPI_Slave_Tx_Buffer[BUFFER_SIZE] = {0x2c, 0x99, 0xd7, 0x26, 0xb0, 0xe5, 0xb2, 0xfc, 0xee, 0x88, 0x3f,
                                            0xde, 0xa4, 0x37, 0x87, 0xc9, 0xb2, 0x9c, 0xce, 0xc8, 0x2c, 0x22,
                                            0x6b, 0xfe, 0xba, 0x49, 0x94, 0x0a, 0x47, 0x5a, 0xb7, 0x89};

void RCC_Configuration(void);
void GPIO_Configuration(void);
Status Buffercmp(uint8_t* pBuffer, uint8_t* pBuffer1, uint16_t BufferLength);

/**
 * @brief  Main program
 */
int main(void)
{
    uint8_t i;
    log_init();

    log_info("\n this is a SPI slave DMA to RAM Demo.\n");

    /* System Clocks Configuration */
    RCC_Configuration();

    /* Configure the GPIO ports */
    GPIO_Configuration();

    /* SPI_SLAVE configuration
     * ------------------------------------------------*/
    SPI_InitStructure.DataDirection = SPI_DIR_DOUBLELINE_FULLDUPLEX;
    SPI_InitStructure.SpiMode       = SPI_MODE_SLAVE;
    SPI_InitStructure.DataLen       = SPI_DATA_SIZE_8BITS;
    SPI_InitStructure.CLKPOL        = SPI_CLKPOL_LOW;
    SPI_InitStructure.CLKPHA        = SPI_CLKPHA_SECOND_EDGE;
    SPI_InitStructure.NSS           = SPI_NSS_SOFT;
    SPI_InitStructure.BaudRatePres  = SPI_BR_PRESCALER_4;
    SPI_InitStructure.FirstBit      = SPI_FB_MSB;
    SPI_InitStructure.CRCPoly       = CRC_POLYNOMIAL;
    SPI_Init(SPI_SLAVE, &SPI_InitStructure);

    /* SPI_SLAVE_RX_DMA_CHANNEL configuration
     * ----------------------------------*/
    DMA_DeInit(SPI_SLAVE_RX_DMA_CHANNEL);
    DMA_InitStructure.PeriphAddr = (uint32_t)&SPI1->DAT;
    DMA_InitStructure.MemAddr    = (uint32_t)SPI_Slave_Rx_Buffer;
    DMA_InitStructure.Direction  = DMA_DIR_PERIPH_SRC;
    DMA_InitStructure.BufSize        = BUFFER_SIZE;
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.CircularMode   = DMA_MODE_NORMAL;
    DMA_InitStructure.Priority       = DMA_PRIORITY_VERY_HIGH;
    DMA_InitStructure.Mem2Mem        = DMA_M2M_DISABLE;
    DMA_Init(SPI_SLAVE_RX_DMA_CHANNEL, &DMA_InitStructure);
    DMA_RequestRemap(DMA_REMAP_SPI1_RX,DMA,SPI_SLAVE_RX_DMA_CHANNEL,ENABLE);

    /* SPI_SLAVE_TX_DMA_CHANNEL configuration
     * ----------------------------------*/
    DMA_DeInit(SPI_SLAVE_TX_DMA_CHANNEL);
    DMA_InitStructure.PeriphAddr = (uint32_t)&SPI1->DAT;
    DMA_InitStructure.MemAddr    = (uint32_t)SPI_Slave_Tx_Buffer;
    DMA_InitStructure.Direction  = DMA_DIR_PERIPH_DST;
    DMA_InitStructure.Priority   = DMA_PRIORITY_MEDIUM;
    DMA_Init(SPI_SLAVE_TX_DMA_CHANNEL, &DMA_InitStructure);
    DMA_RequestRemap(DMA_REMAP_SPI1_TX,DMA,SPI_SLAVE_TX_DMA_CHANNEL,ENABLE);

    /* Enable SPI_SLAVE DMA Tx request */
    SPI_I2S_EnableDma(SPI_SLAVE, SPI_I2S_DMA_TX, ENABLE);
    /* Enable SPI_SLAVE DMA Rx request */
    SPI_I2S_EnableDma(SPI_SLAVE, SPI_I2S_DMA_RX, ENABLE);

    /* Enable SPI_SLAVE CRC calculation */
    SPI_EnableCalculateCrc(SPI_SLAVE, ENABLE);

    /* Enable SPI_SLAVE */
    SPI_Enable(SPI_SLAVE, ENABLE);

    /* Enable DMA channels */
    DMA_EnableChannel(SPI_SLAVE_RX_DMA_CHANNEL, ENABLE);
    DMA_EnableChannel(SPI_SLAVE_TX_DMA_CHANNEL, ENABLE);

    /* Transfer complete */
    while (!DMA_GetFlagStatus(SPI_SLAVE_RX_DMA_FLAG, SPI_SLAVE_DMA))
        ;
    while (!DMA_GetFlagStatus(SPI_SLAVE_TX_DMA_FLAG, SPI_SLAVE_DMA))
        ;

    /* Wait for SPI_SLAVE data reception: CRC transmitted by SPI_MASTER */
    while (SPI_I2S_GetStatus(SPI_SLAVE, SPI_I2S_RNE_FLAG) == RESET)
        ;

    /* Check the correctness of written dada */
    TransferStatus1 = Buffercmp(SPI_Slave_Rx_Buffer, SPI_Master_Tx_Buffer, BUFFER_SIZE);

    /* Test on the SPI_SLAVE CRCR ERROR flag */
    if ((SPI_I2S_GetStatus(SPI_SLAVE, SPI_CRCERR_FLAG)) != RESET)
    {
        TransferStatus1 = FAILED;
    }

    /* Read SPI_SLAVE received CRC value */
    SPI_SLAVECRCValue = SPI_I2S_ReceiveData(SPI_SLAVE);

    if (TransferStatus1 != FAILED)
    {
        log_info("SPI slave passed.\n");
        log_info("slave recv data = ");
        for(i = 0; i< BUFFER_SIZE; i++)
        {
            log_info("%02x", SPI_Slave_Rx_Buffer[i]);
        }
        log_info("\r\n");
    }
    else
    {
        log_error("SPI slave failed.\n");
    }

    while (1)
    {
    }
}

/**
 * @brief  Configures the different system clocks.
 */
void RCC_Configuration(void)
{
    /* Enable peripheral clocks
     * ------------------------------------------------*/
    /* Enable DMA clock */
    RCC_EnableAHBPeriphClk(SPI_SLAVE_DMA_CLK, ENABLE);

    /* Enable SPI_SLAVE clock and GPIO clock for SPI_SLAVE */
    RCC_EnableAPB2PeriphClk(SPI_SLAVE_GPIO_CLK | SPI_SLAVE_CLK | RCC_APB2_PERIPH_AFIO, ENABLE);
}

/**
 * @brief  Configures the different GPIO ports.
 */
void GPIO_Configuration(void)
{
    GPIO_InitType GPIO_InitStructure;

    GPIO_InitStruct(&GPIO_InitStructure);

    /* Configure SPI_SLAVE pins: SCK��MISO and MOSI */
    GPIO_InitStructure.Pin        = SPI_SLAVE_PIN_SCK | SPI_SLAVE_PIN_MOSI |SPI_SLAVE_PIN_MISO;
    GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_HIGH;  
    GPIO_InitStructure.GPIO_Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF0_SPI1;
    GPIO_InitStructure.GPIO_Pull = GPIO_NO_PULL;
    GPIO_InitPeripheral(SPI_SLAVE_GPIO, &GPIO_InitStructure);
}

/**
 * @brief  Compares two buffers.
 * @param  pBuffer, pBuffer1: buffers to be compared.
 * @param BufferLength buffer's length
 * @return PASSED: pBuffer identical to pBuffer1
 *         FAILED: pBuffer differs from pBuffer1
 */
Status Buffercmp(uint8_t* pBuffer, uint8_t* pBuffer1, uint16_t BufferLength)
{
    while (BufferLength--)
    {
        if (*pBuffer != *pBuffer1)
        {
            return FAILED;
        }

        pBuffer++;
        pBuffer1++;
    }

    return PASSED;
}

/**
 * @}
 */

/**
 * @}
 */
