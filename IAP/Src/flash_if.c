/**
  ******************************************************************************
  * @file    IAP_Main/Src/flash_if.c 
  * @author  MCD Application Team
  * @version 1.0.0
  * @date    8-April-2015
  * @brief   This file provides all the memory related operation functions.
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
#include "flash_if.h"
#include "common.h"
#include "stdio.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static uint32_t GetPage(uint32_t Address);
static uint32_t GetBank(uint32_t Address);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  This function does an erase of all user flash area
  * @param  start: start of user flash area
  * @retval FLASHIF_OK : user flash area successfully erased
  *         FLASHIF_ERASEKO : error occurred
  */
uint32_t FLASH_If_Erase(uint32_t startAdd,uint32_t endAdd)
{
	HAL_StatusTypeDef status;
	uint32_t FirstPage = 0, NbOfPages = 0, BankNumber = 0;
	uint32_t PageError = 0;
	static FLASH_EraseInitTypeDef EraseInitStruct;

	  /* Unlock the Flash to enable the flash control register access *************/
	  HAL_FLASH_Unlock();

   /* Clear OPTVERR bit set on virgin samples */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

    /* Erase the user Flash area
      (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

    /* Get the 1st page to erase */
    FirstPage = GetPage(startAdd);

    /* Get the number of pages to erase from 1st page */
    NbOfPages = GetPage(endAdd) - FirstPage + 1;

    /* Get the bank */
    BankNumber = GetBank(FLASH_USER_START_ADDR);

    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks       = BankNumber;
    EraseInitStruct.Page        = FirstPage;
    EraseInitStruct.NbPages     = NbOfPages;

    /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
       you have to make sure that these data are rewritten before they are accessed during code
       execution. If this cannot be done safely, it is recommended to flush the caches by setting the
       DCRST and ICRST bits in the FLASH_CR register. */
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);


  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
	  HAL_FLASH_Lock();

	  if (status != HAL_OK)
	  {
		/* Error occurred while page erase */
		return FLASHIF_ERASEKO;
	  }

	  return FLASHIF_OK;
}

/* Public functions ---------------------------------------------------------*/
/**
  * @brief  This function writes a data buffer in flash (data are 64-bit aligned).
  * @note   After writing data buffer, the flash content is checked.
  * @param  destination: start address for target location
  * @param  p_source: pointer on buffer with data to write
  * @param  length: length of bytes
  * @retval uint32_t 0: Data successfully written to Flash memory
  *         1: Error occurred while writing data in Flash memory
  *         2: Written Data in flash memory is different from expected one
  */
uint32_t FLASH_If_Write(uint32_t destination, uint32_t *p_source, uint32_t length)
{
  uint32_t i = 0;
  uint64_t *p_source64= (uint64_t*)(p_source);
  uint64_t *p_des64=(uint64_t*)(destination);
  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  /*address is beginning at the row begninning, using fast program*/
  if( destination & 0x000000FF ==0){
	  for (i = 0; (i < length) && (destination <= (FLASH_USER_END_ADDR-8)); i++)
	 	  {
	 		/* Device voltage range supposed to be [2.7V to 3.6V], the operation will
	 		   be done by word */
	 		  if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FAST, destination, *(uint64_t*)(p_source64 + i*256)) == HAL_OK)
	 		{
	 		 /* Check the written value */
	 			uint64_t *p_sourceCur64 = (uint64_t*)(p_source64 + i );
	 			uint64_t *p_desCur64 = (uint64_t*)(p_des64 + i );
	 			for(int j=0;j<32;j++)
				  if ((*p_sourceCur64 != *p_desCur64))
				  {
					/* Flash content doesn't match SRAM content */
					return(FLASHIF_WRITINGCTRL_ERROR);
				  }
	 		  /* Increment FLASH destination address */
	 		  destination += 256;
	 		}
	 		else
	 		{
	 		  /* Error occurred while writing data in Flash memory */
	 		  return (FLASHIF_WRITING_ERROR);
	 		}
	 	  }
  }
  else{
	  for (i = 0; (i < length) && (destination <= (FLASH_USER_END_ADDR-8)); i++)
	  {
		/* Device voltage range supposed to be [2.7V to 3.6V], the operation will
		   be done by word */
		  if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, destination, *(uint64_t*)(p_source64 + i )) == HAL_OK)
		{
		 /* Check the written value */
			uint64_t *p_sourceCur64 = (uint64_t*)(p_source64 + i );
			uint32_t *p_sourceCur32 = (uint32_t *)p_sourceCur64;
			uint64_t *p_desCur64 = (uint64_t*)(p_des64 + i );
			uint32_t *p_desCur32 = (uint32_t *)p_desCur64;
		  if ((*(uint32_t*)p_desCur32 != *(uint32_t*)p_sourceCur32) ||
			  (*(uint32_t*)(p_desCur32+1) != *(uint32_t*)(p_sourceCur32+1)) )
		  {
			/* Flash content doesn't match SRAM content */
			return(FLASHIF_WRITINGCTRL_ERROR);
		  }
		  /* Increment FLASH destination address */
		  destination += 8;
		}
		else
		{
		  /* Error occurred while writing data in Flash memory */
		  return (FLASHIF_WRITING_ERROR);
		}
	  }
  }

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  return (FLASHIF_OK);
}
//
///* Public functions ---------------------------------------------------------*/
///**
//  * @brief  This function writes a data buffer in flash (data are 64-bit aligned).
//  * @note   After writing data buffer, the flash content is checked.
//  * @param  destination: start address for target location
//  * @param  p_source: pointer on buffer with data to write
//  * @param  length: length of data buffer (unit is 64-bit word)
//  * @retval uint32_t 0: Data successfully written to Flash memory
//  *         1: Error occurred while writing data in Flash memory
//  *         2: Written Data in flash memory is different from expected one
//  */
//uint32_t FLASH_If_Write(uint32_t destination, uint32_t *p_source, uint32_t length)
//{
//  uint32_t i = 0;
//  uint64_t *p_source64= (uint64_t*)(p_source);
//  uint64_t *p_des64=(uint64_t*)(destination);
//  /* Unlock the Flash to enable the flash control register access *************/
//  HAL_FLASH_Unlock();
//
//  for (i = 0; (i < length) && (destination <= (FLASH_USER_END_ADDR-8)); i++)
//  {
//    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
//       be done by word */
//	  if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, destination, *(uint64_t*)(p_source64 + i )) == HAL_OK)
//    {
//     /* Check the written value */
//		uint64_t *p_sourceCur64 = (uint64_t*)(p_source64 + i );
//		uint32_t *p_sourceCur32 = (uint32_t *)p_sourceCur64;
//		uint64_t *p_desCur64 = (uint64_t*)(p_des64 + i );
//		uint32_t *p_desCur32 = (uint32_t *)p_desCur64;
//      if ((*(uint32_t*)p_desCur32 != *(uint32_t*)p_sourceCur32) ||
//    	  (*(uint32_t*)(p_desCur32+1) != *(uint32_t*)(p_sourceCur32+1)) )
//      {
//        /* Flash content doesn't match SRAM content */
//        return(FLASHIF_WRITINGCTRL_ERROR);
//      }
//      /* Increment FLASH destination address */
//      uint8_t buf[100];
//      sprintf(buf,"Write Flash at %x \n",p_des64);
//      FDCAN_PutString(buf);
//      destination += 8;
//    }
//    else
//    {
//      /* Error occurred while writing data in Flash memory */
//      return (FLASHIF_WRITING_ERROR);
//    }
//  }
//
//  /* Lock the Flash to disable the flash control register access (recommended
//     to protect the FLASH memory against possible unwanted operation) *********/
//  HAL_FLASH_Lock();
//
//  return (FLASHIF_OK);
//}

/**
  * @brief  Gets the page of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static uint32_t GetPage(uint32_t Addr)
{
  uint32_t page = 0;

  if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
  {
    /* Bank 1 */
    page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;
  }
  else
  {
    /* Bank 2 */
    page = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
  }

  return page;
}

/**
  * @brief  Gets the bank of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The bank of a given address
  */
static uint32_t GetBank(uint32_t Addr)
{
  return FLASH_BANK_1;
}



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
