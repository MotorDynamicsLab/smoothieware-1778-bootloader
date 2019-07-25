#include "spi.h"
#include "pins.h"
#include "lpc177x_8x_ssp.h"
#include "lpc177x_8x_pinsel.h"

//Define sd spi
#define SD_SSP	LPC_SSP1


///SD card SPI initialize
void SD_SPI_Init(void)
{
	//Configure pins
	PINSEL_ConfigPin(PORT(SD_SPI_SCLK), PIN(SD_SPI_SCLK), SD_SPI_SCLK_FUNC);
	PINSEL_ConfigPin(PORT(SD_SPI_MISO), PIN(SD_SPI_MISO), SD_SPI_MISO_FUNC);
	PINSEL_ConfigPin(PORT(SD_SPI_MOSI), PIN(SD_SPI_MOSI), SD_SPI_MOSI_FUNC);

	// SSP Configuration structure variable
	SSP_CFG_Type SSP_ConfigStruct;
	// initialize SSP configuration structure to default
	SSP_ConfigStructInit(&SSP_ConfigStruct);
	SSP_ConfigStruct.ClockRate = 4000000;
	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(SD_SSP, &SSP_ConfigStruct);
	// Enable SD_SSP peripheral
	SSP_Cmd(SD_SSP, ENABLE);
}


///SD card SPI write and read one byte data
uint8_t SD_SPI_WriteAndRead(uint8_t data)
{
	uint8_t i = 0;
	while(RESET == SSP_GetStatus(SD_SSP, SSP_STAT_TXFIFO_EMPTY))
	{
		i++; if(i == 250) break;
	}
	
	SSP_SendData(SD_SSP, data);
	
	i = 0;
	while(RESET == SSP_GetStatus(SD_SSP, SSP_STAT_RXFIFO_NOTEMPTY))
	{
		i ++; if(i == 250) break;
	}

	return SSP_ReceiveData(SD_SSP);
}
