/**
 ******************************************************************************
 * @file    IAP_Main/Src/menu.c
 * @author  MCD Application Team
 * @version 1.0.0
 * @date    8-April-2015
 * @brief   This file provides the software which contains the main menu routine.
 *          The main menu gives the options of:
 *             - downloading a new binary file,
 *             - uploading internal flash memory,
 *             - executing the binary file already loaded
 *             - configuring the write protection of the Flash sectors where the
 *               user loads his binary file.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/** @addtogroup stm32g0xx_IAP
 * @{
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "common.h"
#include "flash_if.h"
#include "menu.h"
#include "ymodem.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction JumpToApplication;
uint32_t JumpAddress;
uint32_t FlashProtection = 0;
uint8_t aFileName[FILE_NAME_LENGTH];

/* Private function prototypes -----------------------------------------------*/
void SerialDownload(void);
void SerialUpload(void);

/* Private functions ---------------------------------------------------------*/

void JumpToApp() {
	/* execute the new program */
	JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
	/* Jump to user application */
	JumpToApplication = (pFunction) JumpAddress;
	/* Initialize user application's Stack Pointer */
	__set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
	JumpToApplication();
}

/**
 * @brief  Download a file via serial port
 * @param  None
 * @retval None
 */
void SerialDownload(void) {
	uint8_t number[11] = { 0 };
	uint32_t size = 0;
	COM_StatusTypeDef result;

	FDCAN_PutString(
			"Waiting for the file to be sent ... (press 'a' to abort)\n\r");
	result = Ymodem_Receive(&size);
	if (result == COM_OK) {
		FDCAN_PutString(
				"\n\n\r Programming Completed Successfully!\n\r--------------------------------\r\n Name: ");
		FDCAN_PutString((char*) aFileName);
		Int2Str(number, size);
		FDCAN_PutString("\n\r Size: ");
		FDCAN_PutString((char*) number);
		FDCAN_PutString(" Bytes\r\n");
		FDCAN_PutString("-------------------\n");
	} else if (result == COM_LIMIT) {
		FDCAN_PutString(
				"\n\n\rThe image size is higher than the allowed space memory!\n\r");
	} else if (result == COM_DATA) {
		FDCAN_PutString("\n\n\rVerification failed!\n\r");
	} else if (result == COM_ABORT) {
		FDCAN_PutString("\r\n\nAborted by user.\n\r");
	} else {
		FDCAN_PutString("\n\rFailed to receive the file!\n\r");
	}
}

/**
 * @brief  Upload a file via serial port.
 * @param  None
 * @retval None
 */
void SerialUpload(void) {
	uint8_t status = 0;

	FDCAN_PutString("\n\n\rSelect Receive File\n\r");

	FDCAN_Receive(&status, 1, RX_TIMEOUT);
	if (status == CRC16) {
		/* Transmit the flash image through ymodem protocol */
		status = Ymodem_Transmit((uint8_t*) APPLICATION_ADDRESS,
				(const uint8_t*) "UploadedFlashImage.bin", USER_FLASH_SIZE);

		if (status != 0) {
			FDCAN_PutString("\n\rError Occurred while Transmitting File\n\r");
		} else {
			FDCAN_PutString("\n\rFile uploaded successfully \n\r");
		}
	}
}

/**
 * @brief  Display the Main Menu on HyperTerminal
 * @param  None
 * @retval None
 */
void IAP_Menu(uint32_t timeBeforeJumpMs) {
	uint8_t defaultKeyValue = 'a';
	uint8_t key = defaultKeyValue;
	uint8_t holdon = 1;
	FDCAN_PutString(
			"\r\n======================================================================");
	FDCAN_PutString(
			"\r\n=              (C) COPYRIGHT 2022 Hydrosoft                          =");
	FDCAN_PutString(
			"\r\n=                                                                    =");
	FDCAN_PutString(
			"\r\n=  stm32g0xx In-Application Programming Application  (Version 1.0.0) =");
	FDCAN_PutString(
			"\r\n=                                                                    =");
	FDCAN_PutString(
			"\r\n=                                   By Hydrosoft Chen Xiaojiao       =");
	FDCAN_PutString(
			"\r\n======================================================================");
	FDCAN_PutString("\r\n\r\n");

	FDCAN_PutString(
			"\r\n=================== Main Menu ============================\r\n\n");
	FDCAN_PutString(
			"  Wait here--------------------------------------------- 0\r\n\n");
	FDCAN_PutString(
			"  Download image to the internal Flash ----------------- 1\r\n\n");
	FDCAN_PutString(
			"  Upload image from the internal Flash ----------------- 2\r\n\n");
	FDCAN_PutString("  Jump to application ----------------------- 3\r\n\n");

	uint32_t tstart = HAL_GetTick();
	while (1) {

		/* Clean the input path */
//    __HAL_UART_FLUSH_DRREGISTER(&UartHandle);
		/* If time out, jump to application*/
		uint32_t passT = HAL_GetTick() - tstart;

		if (holdon == 0) {

			if (passT == 1000) {
				FDCAN_PutString(
						"Will jump to application after : 9 seconds...\r\n");
			} else if (passT == 2000) {
				FDCAN_PutString(
						"Will jump to application after : 8 seconds...\r\n");
			} else if (passT == 3000) {
				FDCAN_PutString(
						"Will jump to application after : 7 seconds...\r\n");
			} else if (passT == 4000) {
				FDCAN_PutString(
						"Will jump to application after : 6 seconds...\r\n");
			} else if (passT == 5000) {
				FDCAN_PutString(
						"Will jump to application after : 5 seconds...\r\n");
			} else if (passT == 6000) {
				FDCAN_PutString(
						"Will jump to application after : 4 seconds...\r\n");
			} else if (passT == 7000) {
				FDCAN_PutString(
						"Will jump to application after : 3 seconds...\r\n");
			} else if (passT == 8000) {
				FDCAN_PutString(
						"Will jump to application after : 2 seconds...\r\n");
			} else if (passT == 9000) {
				FDCAN_PutString(
						"Will jump to application after : 1 seconds...\r\n");
			} else if (passT == 10000) {
				FDCAN_PutString(
						"Will jump to application after : 0 seconds...\r\n");
				FDCAN_PutString("Auto Start program execution......\r\n\n");
				JumpToApp();
			}
		}

		/* Receive key , ms interval for check input char */
		if (FDCAN_Receive(&key, 1, 1) == HAL_OK) {
			switch (key) {
			case '0':
				/* stop the timeout clock and hold on here*/
				holdon = 1;
				break;
			case '1':
				/* Download user application in the Flash */
				holdon = 1;
				SerialDownload();
				break;
			case '2':
				/* Upload user application from the Flash */
				holdon = 1;
				SerialUpload();
				break;
			case '3':
				FDCAN_PutString("Start program execution......\r\n\n");
				JumpToApp();
				break;
			default:
				FDCAN_PutString(
						"Invalid Number ! ==> The number should be either 0, 1, 2 or 3 \r\n");
				break;
			}

		}
		key = defaultKeyValue;

	}
}

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
