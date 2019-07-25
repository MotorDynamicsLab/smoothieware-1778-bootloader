/*****************************************************************************
 *                                                                            *
 * DFU/SD/SDHC Bootloader for LPC17xx                                         *
 *                                                                            *
 * by Triffid Hunter                                                          *
 *                                                                            *
 *                                                                            *
 * This firmware is Copyright (C) 2009-2010 Michael Moon aka Triffid_Hunter   *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation; either version 2 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software                *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA *
 *                                                                            *
 *****************************************************************************/

#include "usbhw.h"
#include "usbcore.h"

#include "uart.h"

#include "SDCard.h"

#include "gpio.h"

#include "sbl_iap.h"
#include "sbl_config.h"

#include "ff.h"

#include "dfu.h"

#include "lpc177x_8x_wwdt.h"

#ifndef DEBUG_MESSAGES
#define printf(...) do {} while (0)
#endif

	
/***************************************/
/*               PINs                  */
/***************************************/
void config_pins(void)
{
	//Config DFU_BTN
	GPIO_init(DFU_BTN); GPIO_input(DFU_BTN);
	
	//Config LEDs
	GPIO_init(LED1); GPIO_output(LED1);
	GPIO_init(LED2); GPIO_output(LED2);
	GPIO_init(LED3); GPIO_output(LED3);
	GPIO_init(LED4); GPIO_output(LED4);

	//Turn off heater outputs
	GPIO_init(P2_4); GPIO_output(P2_4); GPIO_write(P2_4, 0);
	GPIO_init(P2_5); GPIO_output(P2_5); GPIO_write(P2_5, 0);
	GPIO_init(P2_6); GPIO_output(P2_6); GPIO_write(P2_6, 0);
	GPIO_init(P2_7); GPIO_output(P2_7); GPIO_write(P2_7, 0);
}

	
/***************************************/
/*               Leds                  */
/***************************************/
void setleds(int leds)
{
	GPIO_write(LED1, leds &  1);
	GPIO_write(LED2, leds &  2);
	GPIO_write(LED3, leds &  4);
	GPIO_write(LED4, leds &  8);
}

/***************************************/
/*           DFU firmware              */
/***************************************/
int dfu_btn_pressed(void)
{
	return GPIO_get(DFU_BTN);
}

void start_dfu(void)
{
	DFU_init();
	usb_init();
	usb_connect();
	while (DFU_complete() == 0)
		usb_task();
	usb_disconnect();
}

//Initialize dfu and check is enter dfu mode
void check_dfu_firmware(void)
{
	int dfu = 0;
	if (dfu_btn_pressed() == 0)
	{
		printf("ISP button pressed, entering DFU mode\r\n");
		dfu = 1;
	}
	else if (WWDT_GetStatus(WWDT_TIMEOUT_FLAG))
	{
		WWDT_ClrTimeOutFlag();
		printf("WATCHDOG reset, entering DFU mode\r\n");
		dfu = 1;
	}
	else if (*(uint32_t *)USER_FLASH_START == 0xFFFFFFFF)
	{
		printf("User flash empty, enabling DFU\r\n");
		dfu = 1;
	}
	
	if (dfu) start_dfu();
}


/***************************************/
/*         SD card firmware            */
/***************************************/
void check_sd_firmware(void)
{
	const char *firmware_file = "firmware.bin";
	const char *firmware_old  = "firmware.cur";
	static FATFS	fat;
	static FIL		file;
	
	printf("Initialize SD card\r\n");
	SDCard_Init();
	if (SDCard_disk_initialize()) return;
	
	printf("Check SD card\r\n");
	f_mount(0, &fat);

	int res = f_open(&file, firmware_file, FA_READ);
	if (res == FR_OK)
	{
		printf("Flashing firmware: Start!\r\n");
		
		static uint8_t readbuf[512];
		unsigned int readsize = sizeof(readbuf);
		uint32_t address = USER_FLASH_START;
		
		while (readsize == sizeof(readbuf))
		{
			if (f_read(&file, readbuf, sizeof(readbuf), &readsize) != FR_OK)
			{
				f_close(&file);
				return;
			}

			printf("address: 0x%x, readsize: %d\r\n", address, readsize);
			
			setleds((address - USER_FLASH_START) >> 15);
			write_flash((void *)address, (char *)readbuf, sizeof(readbuf));
			address += readsize;
		}
		
		f_close(&file);
		if (address > USER_FLASH_START)
		{
			printf("Flashing firmware: Complete!\r\n");
			res = f_unlink(firmware_old);
			res = f_rename(firmware_file, firmware_old);
		}
	}
}

// this seems to fix an issue with handoff after poweroff
// found here http://knowledgebase.nxp.trimm.net/showthread.php?t=2869
typedef void __attribute__((noreturn))(*exec)();

static void boot(uint32_t a)
{
    uint32_t *start;

    __set_MSP(*(uint32_t *)USER_FLASH_START);
    start = (uint32_t *)(USER_FLASH_START + 4);
    ((exec)(*start))();
}

static uint32_t delay_loop(uint32_t count)
{
	volatile uint32_t j, del;
	for(j=0; j<count; ++j){
		del=j; // volatiles, so the compiler will not optimize the loop
	}
	return del;
}

static void new_execute_user_code(void)
{
	uint32_t addr=(uint32_t)USER_FLASH_START;
	
	// relocate vector table
	SCB->VTOR = (addr & 0x1FFFFF80);
	
	delay_loop(1000);
	
	// reset pipeline, sync bus and memory access
	__asm (
		   "dmb\n"
		   "dsb\n"
		   "isb\n"
		  );
	boot(addr);
}

int main(void)
{
	WWDT_Feed();
	
	//Configure pins
	config_pins();

	//Turn on all leds
	setleds(0xf);
	
	//Initialize uart
	Uart_Init(APPBAUD);
	printf("Bootloader Start\r\n");

	//Initialize sd card and check sd firmware
	check_sd_firmware();
	
	//Initialize dfu and check dfu firmware
	check_dfu_firmware();

	//Initialize watchdog
#ifdef WATCHDOG
	WWDT_Init(WDT_CLKSRC_IRC, WDT_MODE_RESET);
	WWDT_Start(1<<22);
#endif

	//Jump to application
	printf("Jump!\r\n");
	new_execute_user_code();

  Uart_Init(APPBAUD);
	printf("This should never happen\r\n");

	printf("Reset system\r\n");
	NVIC_SystemReset();
}

//fatfs get_fattime
DWORD get_fattime(void)
{
#define	YEAR	2012
#define MONTH	11
#define DAY		13
#define HOUR	20
#define MINUTE	13
#define SECOND	1
return	(((DWORD)YEAR  & 127) << 25) |
		(((DWORD)MONTH &  15) << 21) |
		(((DWORD)DAY   &  31) << 16) |
		(((DWORD)HOUR  &  31) << 11) |
		(((DWORD)MINUTE & 63) <<  5) |
		(((DWORD)SECOND & 63) <<  0);
}


void NMI_Handler() {
 	printf("NMI\n");
	for (;;);
}
void HardFault_Handler() {
 	printf("HardFault\n");
	for (;;);
}
void MemManage_Handler() {
 	printf("MemManage\n");
	for (;;);
}
void BusFault_Handler() {
 	printf("BusFault\n");
	for (;;);
}
void UsageFault_Handler() {
 	printf("UsageFault\n");
	for (;;);
}
