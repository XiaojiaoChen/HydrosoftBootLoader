/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "menu.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
uint8_t CAN_ID = 0;

 typedef struct CANBUS_HANDLE_TAG{
	FDCAN_HandleTypeDef     CanHandle;
	FDCAN_TxHeaderTypeDef   TxHeader;
	FDCAN_RxHeaderTypeDef   RxHeader;
	uint8_t               TxData[8];
	uint8_t               RxData[8];
	uint32_t              TxMailbox;
 }CANBUS_HANDLE;
 CANBUS_HANDLE canbus;


 uint8_t utxbuf[100];
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

FDCAN_HandleTypeDef hfdcan1;

/* USER CODE BEGIN PV */
extern pFunction JumpToApplication;
extern uint32_t JumpAddress;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_FDCAN1_Init(void);
/* USER CODE BEGIN PFP */
void canConfig();
void canSend();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_FDCAN1_Init();
  /* USER CODE BEGIN 2 */

  if (1)
    {
     /* Execute the IAP driver in order to reprogram the Flash */
      canConfig();
      /* Display main menu */
      Main_Menu ();
    }
    /* Keep the user application running */
    else
    {
      /* Test if user code is programmed starting from address "APPLICATION_ADDRESS" */
      if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000)
      {
        /* Jump to user application */
        JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
        JumpToApplication = (pFunction) JumpAddress;
        /* Initialize user application's Stack Pointer */
        __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
        JumpToApplication();
      }
    }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
//	  canSend();
//	  HAL_Delay(10);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2)
  {
  }

  /* HSI configuration and activation */
  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1)
  {
  }

  /* Main PLL configuration and activation */
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_1, 8, LL_RCC_PLLR_DIV_2);
  LL_RCC_PLL_Enable();
  LL_RCC_PLL_EnableDomain_SYS();
  while(LL_RCC_PLL_IsReady() != 1)
  {
  }

  /* Set AHB prescaler*/
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

  /* Sysclk activation on the main PLL */
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  }

  /* Set APB1 prescaler*/
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
  LL_SetSystemCoreClock(64000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief FDCAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_FDCAN1_Init(void)
{

  /* USER CODE BEGIN FDCAN1_Init 0 */

  /* USER CODE END FDCAN1_Init 0 */

  /* USER CODE BEGIN FDCAN1_Init 1 */

  /* USER CODE END FDCAN1_Init 1 */
  hfdcan1.Instance = FDCAN1;
  hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;
  hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
  hfdcan1.Init.AutoRetransmission = DISABLE;
  hfdcan1.Init.TransmitPause = DISABLE;
  hfdcan1.Init.ProtocolException = DISABLE;
  hfdcan1.Init.NominalPrescaler = 4;
  hfdcan1.Init.NominalSyncJumpWidth = 4;
  hfdcan1.Init.NominalTimeSeg1 = 11;
  hfdcan1.Init.NominalTimeSeg2 = 4;
  hfdcan1.Init.DataPrescaler = 4;
  hfdcan1.Init.DataSyncJumpWidth = 4;
  hfdcan1.Init.DataTimeSeg1 = 11;
  hfdcan1.Init.DataTimeSeg2 = 4;
  hfdcan1.Init.StdFiltersNbr = 1;
  hfdcan1.Init.ExtFiltersNbr = 0;
  hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN FDCAN1_Init 2 */

  /* USER CODE END FDCAN1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);

}

/* USER CODE BEGIN 4 */
void canConfig() {

	 canbus.CanHandle = hfdcan1;
	 FDCAN_FilterTypeDef sFilterConfig;

	 /* Configure reception filter to Rx FIFO 0 */
	  sFilterConfig.IdType = FDCAN_STANDARD_ID;
	  sFilterConfig.FilterIndex = 0;
	  sFilterConfig.FilterType = FDCAN_FILTER_MASK;
	  sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	  //only receive IDs with last 8 bits identical to 0 (filtering out normal nodes on can network)
	  sFilterConfig.FilterID1 = (uint16_t) (0x0700); //filter ID
	  sFilterConfig.FilterID2 = (uint16_t) (0x00FF); //high bits indicate a must-match with filter ID corresponding bits
	  if (HAL_FDCAN_ConfigFilter(&canbus.CanHandle, &sFilterConfig) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  /* Configure global filter:
	     Filter all remote frames with STD and EXT ID
	     Reject non matching frames with STD ID and EXT ID */
	  if (HAL_FDCAN_ConfigGlobalFilter(&canbus.CanHandle, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK)
	  {
	    Error_Handler();
	  }

	  /* Activate Rx FIFO 0 new message notification */
	  if (HAL_FDCAN_ActivateNotification(&canbus.CanHandle, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
	  {
	    Error_Handler();
	  }

	  /* Configure and enable Tx Delay Compensation, required for BRS mode.
	     TdcOffset default recommended value: DataTimeSeg1 * DataPrescaler
	     TdcFilter default recommended value: 0 */
	  if (HAL_FDCAN_ConfigTxDelayCompensation(&canbus.CanHandle, 5, 0) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  if (HAL_FDCAN_EnableTxDelayCompensation(&canbus.CanHandle) != HAL_OK)
	  {
	    Error_Handler();
	  }

	  /* Start the FDCAN module */
	  if (HAL_FDCAN_Start(&canbus.CanHandle) != HAL_OK)
	  {
	    Error_Handler();
	  }
}

void canSend() {
	 /* Prepare Tx message Header */
	canbus.TxHeader.Identifier = CAN_ID;
	canbus.TxHeader.IdType = FDCAN_STANDARD_ID;
	canbus.TxHeader.TxFrameType = FDCAN_DATA_FRAME;
	canbus.TxHeader.DataLength = FDCAN_DLC_BYTES_8;
	canbus.TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	canbus.TxHeader.BitRateSwitch = FDCAN_BRS_OFF;    //Need to change in case of CANFD
	canbus.TxHeader.FDFormat = FDCAN_CLASSIC_CAN;//Need to change in case of CANFD
	canbus.TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	canbus.TxHeader.MessageMarker = 0;

	canbus.TxData[0]=1;
	canbus.TxData[1]=2;
	canbus.TxData[2]=3;
	canbus.TxData[3]=4;
	canbus.TxData[4]=5;
	canbus.TxData[5]=6;
	canbus.TxData[6]=7;
	canbus.TxData[7]=8;

	/* Start the Transmission process */
	  if (HAL_FDCAN_AddMessageToTxFifoQ(&canbus.CanHandle, &(canbus.TxHeader), canbus.TxData) != HAL_OK)
		{
		  Error_Handler();
		}
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
	if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0)
	  {
	    /* Retrieve Rx messages from RX FIFO0 */
	    if (HAL_FDCAN_GetRxMessage(&canbus.CanHandle, FDCAN_RX_FIFO0, &canbus.RxHeader, canbus.RxData) != HAL_OK)
	    {
	      Error_Handler();
	    }

		//handle Rx Frame




	  }
}





/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

