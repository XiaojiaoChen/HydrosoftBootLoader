/*
 * canbus_if.h
 *
 *  Created on: Dec 31, 2021
 *      Author: softarm
 */

#ifndef INC_CANBUS_IF_H_
#define INC_CANBUS_IF_H_

#include "main.h"



 typedef struct CANBUS_HANDLE_TAG{
	FDCAN_HandleTypeDef     CanHandle;
	FDCAN_TxHeaderTypeDef   TxHeader;
	FDCAN_RxHeaderTypeDef   RxHeader;
	uint8_t               TxData[8];
	uint8_t               RxData[8];
	uint32_t              TxMailbox;
 }CANBUS_HANDLE;
 extern CANBUS_HANDLE canbus;

 HAL_StatusTypeDef FDCAN_Transmit(uint8_t *p_string,int16_t num,uint32_t timeout);
 HAL_StatusTypeDef FDCAN_Receive(uint8_t *p_string,int16_t num,uint32_t timeout);


#endif /* INC_CANBUS_IF_H_ */