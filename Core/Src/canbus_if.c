/*
 * canbus_if.c
 *
 *  Created on: Dec 31, 2021
 *      Author: softarm
 */

#include "canbus_if.h"

const uint16_t CAN_ID=0x0000;


#define MASTER_BROADCAST  0x07FF   //Master BroadCast ID
#define MASTER_P2P_MASK   0x0400   //Master p2p Mask is 0x0400


CANBUS_HANDLE canbus;
extern FDCAN_HandleTypeDef hfdcan1;

void canConfig() {

	 canbus.CanHandle = hfdcan1;
	 FDCAN_FilterTypeDef sFilterConfig;

	 /* Configure reception filter to Rx FIFO 0 */
	  sFilterConfig.IdType = FDCAN_STANDARD_ID;
	  sFilterConfig.FilterIndex = 0;
	  sFilterConfig.FilterType = FDCAN_FILTER_DUAL;
	  sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	  //only receive two ID, Master general call and P2P call
	  sFilterConfig.FilterID1 = (uint16_t)(MASTER_P2P_MASK | CAN_ID); // Master P2P call
	  sFilterConfig.FilterID2 = (uint16_t)MASTER_BROADCAST; //Master general call
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
uint32_t FDCAN_DLC_BYTES_ARR[9]={
		FDCAN_DLC_BYTES_0,
		FDCAN_DLC_BYTES_1,
		FDCAN_DLC_BYTES_2,
		FDCAN_DLC_BYTES_3,
		FDCAN_DLC_BYTES_4,
		FDCAN_DLC_BYTES_5,
		FDCAN_DLC_BYTES_6,
		FDCAN_DLC_BYTES_7,
		FDCAN_DLC_BYTES_8
};

/*Only suit DLC <=8 bytes*/
HAL_StatusTypeDef FDCAN_Transmit_One_Frame(uint8_t *p_string,int16_t num,uint32_t timeout){
	HAL_StatusTypeDef status=HAL_OK;
	canbus.TxHeader.Identifier = CAN_ID;
	canbus.TxHeader.IdType = FDCAN_STANDARD_ID;
	canbus.TxHeader.TxFrameType = FDCAN_DATA_FRAME;
	canbus.TxHeader.DataLength = FDCAN_DLC_BYTES_ARR[num];  // num bytes(0~8)
	canbus.TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	canbus.TxHeader.BitRateSwitch = FDCAN_BRS_OFF;    //Need to change in case of CANFD
	canbus.TxHeader.FDFormat = FDCAN_CLASSIC_CAN;//Need to change in case of CANFD
	canbus.TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	canbus.TxHeader.MessageMarker = 0;

	for(int i=0;i<num;i++){
		canbus.TxData[i]=p_string[i];
	}

	/* Start the Transmission process */
	if((status = HAL_FDCAN_AddMessageToTxFifoQ(&canbus.CanHandle, &(canbus.TxHeader), canbus.TxData))==HAL_OK){

		uint32_t tstart=HAL_GetTick();
		while(HAL_FDCAN_GetTxFifoFreeLevel(&canbus.CanHandle)<3){
					if(HAL_GetTick()-tstart>timeout){
						status = HAL_TIMEOUT;
						break;
					}
		}

	}

	return status;
}

HAL_StatusTypeDef FDCAN_Transmit(uint8_t *p_string,int16_t num,uint32_t timeout){
	HAL_StatusTypeDef status=HAL_OK;
	uint32_t frameNB=num/8;
	uint32_t remBytesNB=num%8;
	uint8_t timeoutInd=timeout/8;
	for(int i=0;i<frameNB && (status==HAL_OK);i++){
		status = FDCAN_Transmit_One_Frame(p_string,8,timeoutInd);
		p_string+=8;
	}
	if(status==HAL_OK && remBytesNB!=0){
		status = FDCAN_Transmit_One_Frame(p_string,remBytesNB,timeoutInd);
	}

	return status;
}

/*CAN Rx Interrupt Routine, into rx queue*/
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
  if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0)
  {
    /* Retrieve Rx messages from RX FIFO0 */
    if(HAL_FDCAN_GetRxMessage(&canbus.CanHandle, FDCAN_RX_FIFO0, &canbus.RxHeader, canbus.RxData) != HAL_OK)
    {
      Error_Handler();
    }

    /* Copy to buff */
     uint32_t bytesNB = (canbus.RxHeader.DataLength)>>16;
     if(queue_in_bulk(&canbus.RxQueue,&canbus.RxData[0],bytesNB)==HAL_ERROR){
    	 int aa=1;
    	 aa++;
     }
  }
}

/*get Rx data from rx queue*/
HAL_StatusTypeDef FDCAN_Receive(uint8_t *p_string,int16_t num,uint32_t timeout){

	uint32_t tstart = HAL_GetTick();
	while(queue_out_bulk(&canbus.RxQueue,p_string,num)!=HAL_OK){
		if(HAL_GetTick()-tstart>timeout){
			return HAL_TIMEOUT;
		}
	}
	return HAL_OK;
}

void FDCAN_ClearRxBuffer(){
	canbus.RxQueue.count=0;
	canbus.RxQueue.front=0;
	canbus.RxQueue.rear=0;
}
