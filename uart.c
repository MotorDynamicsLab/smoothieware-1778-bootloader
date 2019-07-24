#include "uart.h"
#include "pins.h"
#include "lpc177x_8x_uart.h"
#include "lpc177x_8x_pinsel.h"

#define _LPC_UART (UART_ID_Type)UART_ID

///Uart initialize
void Uart_Init(uint32_t baud_rate)
{
	// Initialize UART0 pin connect
	PINSEL_ConfigPin(PORT(UART_TX), PIN(UART_TX), UART_TX_FUNC);
	PINSEL_ConfigPin(PORT(UART_RX), PIN(UART_RX), UART_RX_FUNC);
	
	// UART Configuration structure variable
	UART_CFG_Type UARTConfigStruct;
		
	// UART FIFO configuration Struct variable
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;
	
	/* Initialize UART Configuration parameter structure to default state:
	* Baudrate = 115200 bps
	* 8 data bit
	* 1 Stop bit
	* None parity
	*/
	UART_ConfigStructInit(&UARTConfigStruct);
	//Configure baud rate
	UARTConfigStruct.Baud_rate = baud_rate;
	// Initialize UART0 & UART3 peripheral with given to corresponding parameter
	UART_Init(_LPC_UART, &UARTConfigStruct);

	/* Initialize FIFOConfigStruct to default state:
	*              - FIFO_DMAMode = DISABLE
	*              - FIFO_Level = UART_FIFO_TRGLEV0
	*              - FIFO_ResetRxBuf = ENABLE
	*              - FIFO_ResetTxBuf = ENABLE
	*              - FIFO_State = ENABLE
	*/
	UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);

	// Initialize FIFO for UART0 peripheral
	UART_FIFOConfig(_LPC_UART, &UARTFIFOConfigStruct);

	// Enable UART Transmit
	UART_TxCmd(_LPC_UART, ENABLE);
}


///Redefined fputc in order to using printf
int fputc(int ch,FILE *f)
{  
		UART_SendByte(_LPC_UART, ch);

    while (UART_CheckBusy(_LPC_UART) == SET);

    return(ch);  
}
