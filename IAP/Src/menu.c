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
#include "stdio.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction JumpToApplication;
uint32_t JumpAddress;
uint32_t FlashProtection = 0;
uint8_t aFileName[FILE_NAME_LENGTH];
uint8_t canIDStr[20]={0};
/* Private function prototypes -----------------------------------------------*/
HAL_StatusTypeDef SerialDownload(void);
HAL_StatusTypeDef SerialUpload(void);

/* Private functions ---------------------------------------------------------*/

void JumpToApp() {

	//before jump, deinitialize all the possible things
	deInitAll();

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
HAL_StatusTypeDef SerialDownload(void) {
	uint8_t number[11] = { 0 };
	uint32_t size = 0;
	COM_StatusTypeDef result;

	result = Ymodem_Receive(&size);
	if (result == COM_OK) {
		FDCAN_PutString("\n\n\r CAN Node ");
		FDCAN_PutString((char *)canIDStr);
		FDCAN_PutString(" Programming Completed !\n\r--------------------------------\r\n Name: ");
		FDCAN_PutString((char*) aFileName);
		Int2Str(number, size);
		FDCAN_PutString("\n\r Size: ");
		FDCAN_PutString((char*) number);
		FDCAN_PutString(" Bytes\r\n");
		FDCAN_PutString("-------------------\n");

		return HAL_OK;
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
	return HAL_ERROR;
}

/**
 * @brief  Display the Main Menu on HyperTerminal
 * @param  None
 * @retval None
 */
void IAP_Menu() {
	extern const uint64_t CAN_ID_64;
	uint8_t defaultKeyValue = 'a';
	uint8_t key = defaultKeyValue;
	uint8_t holdon = 0;
	uint32_t tstart = HAL_GetTick();
	int32_t lastT=0;
	uint8_t tick[20]={0};

	Int2Str(canIDStr,(CAN_ID_64));
	const uint64_t *canIDAddress=&CAN_ID_64;//0x08003fb2
	uint8_t canAddStr[50]={0};
	Int2Str(canAddStr,(uint32_t)(canIDAddress));

	FDCAN_PutString("\r\n=================== CAN_ID = ");
	FDCAN_PutString((char *)canIDStr);
	FDCAN_PutString(" at Address ");
	FDCAN_PutString((char *)canAddStr);
	FDCAN_PutString("   as uint64_t ====\r\n\n");
	FDCAN_PutString("  Jump to application                  ----------------- 0\r\n\n");
	FDCAN_PutString("  Download image to the internal Flash ----------------- 1\r\n\n");
	FDCAN_PutString("  hold                                 ----------------- 2\r\n\n");

	while (1) {

		/* Clean the input path */
		FDCAN_ClearRxBuffer();

		/* If time out, jump to application*/
		uint32_t passT = HAL_GetTick() - tstart;
		if (holdon == 0) {
			if(passT/1000-lastT==1){
				for(int i=0;i<10;i++)
					tick[i]=0;
				FDCAN_PutString("Will jump to application after :");
				Int2Str(tick,(10-lastT));
			    FDCAN_PutString((char *)tick);
			    FDCAN_PutString(" seconds...\r\n");
				if(lastT==10){
					FDCAN_PutString("Auto Start program execution......\r\n\n");
					JumpToApp();
				}
				lastT++;
			}
		}

		/* Receive key , ms interval for check input char */
		if (FDCAN_Receive(&key, 1, 10) == HAL_OK) {
			switch (key) {
			case '0':
				FDCAN_PutString("Start program execution......\r\n\n");
				JumpToApp();
				break;
			case '1':
				/* Download user application in the Flash */
				holdon = 1;
				if(SerialDownload()==HAL_OK){
//					/*Dont jump directly, may contaminate the CAN bus for other nodes' update*/
					FDCAN_PutString("Holding on......\r\n\n");

				}
				else{
					FDCAN_ClearRxBuffer();
				}
				break;
			case '2':
				/* Just hold, waiting for further instruction */
				holdon = 1;
				FDCAN_PutString((char *)canIDStr);
				FDCAN_PutString("  is holding on, waiting for furthur instructions..\r\n");
				break;
			default:

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
