///*
// * myUsartFunction.h
// *
// *  Created on: Jun 12, 2018
// *      Author: 402072495
// */
//
//#ifndef FRAMEINC_MYUSARTFUNCTION_H_
//#define FRAMEINC_MYUSARTFUNCTION_H_
//
//#ifdef __cplusplus
// extern "C" {
//#endif
//
//#include "main.h"
//#include "stdio.h"
//
//
//#define UART_TX_BUF_SIZE 2048
//#define UART_RX_BUF_SIZE 30
//
////---------------------------RING BUFFER--------------------------
////
////| 0 | 1 | 2 | 3 | 4 | 5 | ...                           |UART_TX_BUF_SIZE-1|
////|---flushLen----|------------------------------|--------flushLen-----------|
////                *                               *
////                bufStartNum    				  flushStartNum
//
//
// //---------------------------RING BUFFER--------------------------
// //
// //| 0 | 1 | 2 | 3 | 4 | 5 | ...                  |UART_TX_BUF_SIZE-1|
// //|-------------------------------------------|----flushLen--|---------------|
// //                            				   *               *
// //                                            flushStartNum   bufStartNum
//
//
////possible to overlap sending data in exchange for better utilization
//
//
// typedef struct UART_DEVICE_STRUCT{
//	uint8_t TxBuf[UART_TX_BUF_SIZE];  /*Tx ring + dynamic Buffer */
//
//	int16_t flushStartNum;
//	int16_t flushLen;
//
//	int16_t bufStartNum;
//	int16_t transLen;
//	uint8_t isFree;
//
//	UART_HandleTypeDef *huart;
//
//}UART_DEVICE;
//
//extern UART_DEVICE Usart1Device;
//
//#ifdef __cplusplus
//}
//#endif
//
//
//#endif /* FRAMEINC_MYUSARTFUNCTION_H_ */
