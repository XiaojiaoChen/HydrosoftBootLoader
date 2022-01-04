///*
// * myUsartFunction.cpp
// *
// *  Created on: Jun 12, 2018
// *      Author: 402072495
// */
//#include "stm32g0b1xx.h"
//#include "myUsartFunction.h"
//#include "stdio.h"
//#include "string.h"
//#include "main.h"
//extern UART_HandleTypeDef huart1;
//
//
///**
// * wait until the flag is false or timeout. The flag is check at 1 us interval
// */
//void myWaitForFlagUntilTimeout(uint8_t flag, uint32_t timeout){
//	uint32_t tt=0;
//	while(flag){
//		delay_us(1);
//		if(tt++>timeout)
//			break;
//	}
//
//}
//
//UART_DEVICE Usart1Device = {
//	.flushLen = 0,
//	.flushStartNum = 0,
//	.bufStartNum = 0,
//	.isFree = 1,
//	.huart = &huart1
//
//};
//
//
//static int writeRingBuf(UART_DEVICE *uDev, char *pSrc, int len);
//static int getVacancy(UART_DEVICE *uDev);
//static int getFlushSegment(UART_DEVICE *uDev);
//static int getBufSegment(UART_DEVICE *uDev);
//static void flushSegment(UART_DEVICE *uDev);
//
///*Redirect printf() by implementing (weak) _write function.
// *Every printf() call would store the output string in TxBuf[], ready for Usart DMA output instead of directly output*/
//int _write(int file, char *pSrc, int len)
//{
//	//HAL_UART_Transmit(Usart1Device.huart,pSrc,len,100);
//	writeRingBuf(&Usart1Device, pSrc, len);
//	if (Usart1Device.isFree)
//	{
//		flushSegment(&Usart1Device);
//	}
//	return len;
//}
//
//
//
//int writeRingBuf(UART_DEVICE *uDev, char *pSrc, int len)
//{
//	if (len > 0)
//	{
//		while(len>getVacancy(uDev));//block here, waiting for enough vacancy TODO: add timeout
//
//		//now we have Enough vacancy
//		int segLen = getBufSegment(uDev);
//
//		memcpy(&uDev->TxBuf[uDev->bufStartNum],pSrc,segLen);
//
//		if(segLen<len)//wrap,basd on the condition that we have enough vacancy.
//			memcpy(&uDev->TxBuf[0],pSrc+segLen,(len-segLen));
//
//		uDev->bufStartNum = (uDev->bufStartNum + len) % UART_TX_BUF_SIZE;
//		uDev->flushLen +=  len;
//		return len;
//	}
//	return 0;
//}
//
//
///**
// * @brief flush out buffered data. If the buffered data is seperated into two parts, then only flush the first part.
// *
// * @param uDev
// */
//static void flushSegment(UART_DEVICE *uDev)
//{
//
//	uDev->transLen = getFlushSegment(uDev);
//
//	if(uDev->transLen > 0){
//		if(HAL_UART_Transmit_DMA(uDev->huart, &(uDev->TxBuf[uDev->flushStartNum]), uDev->transLen)==HAL_OK){
//			uDev->isFree = 0;//only set this if transmision start normal, otherwise may never be free again.
//		}
//	}
//
//
//}
//
//static int getVacancy(UART_DEVICE *uDev){
//	return (int)(UART_TX_BUF_SIZE - uDev->flushLen);
//}
//
///**
// * get the length of the next continuous ready-to-flush block
// */
//static int getFlushSegment(UART_DEVICE *uDev){
//	 if(uDev->flushStartNum < uDev->bufStartNum)
//		 return (uDev->bufStartNum - uDev->flushStartNum);
//	 else if(uDev->flushStartNum > uDev->bufStartNum)
//		 return (UART_TX_BUF_SIZE - uDev->flushStartNum);
//	 else if(uDev->flushLen>0)//uDev->flushStartNum == uDev->bufStartNum, full data
//		 return (UART_TX_BUF_SIZE - uDev->flushStartNum);
//	 else//uDev->flushStartNum == uDev->bufStartNum, no data
//		 return 0;
//}
//
///**
// * get the length of the next continuous free block
// */
//static int getBufSegment(UART_DEVICE *uDev){
//	 if(uDev->flushStartNum < uDev->bufStartNum)
//		 return (UART_TX_BUF_SIZE - uDev->bufStartNum);
//	 else if(uDev->flushStartNum > uDev->bufStartNum)
//		 return (uDev->flushStartNum - uDev->bufStartNum);
//	 else if(uDev->flushLen)//uDev->flushStartNum == uDev->bufStartNum, full data
//		 return 0;
//	 else//uDev->flushStartNum == uDev->bufStartNum, no data
//		 return (UART_TX_BUF_SIZE - uDev->bufStartNum);
//}
//
///*this function would overwrite HAL's weak HAL_UART_TxCpltCallback*/
//void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
//	Usart1Device.flushStartNum = (Usart1Device.flushStartNum + Usart1Device.transLen) % UART_TX_BUF_SIZE;
//	Usart1Device.flushLen -=  Usart1Device.transLen;
//	if(Usart1Device.flushLen>0){//more data to flush
//		flushSegment(&Usart1Device);
//	}
//	else if(Usart1Device.flushLen==0){//normal complete or abnormal
//
////		Usart1Device.flushLen = 0;
////				Usart1Device.flushStartNum = 0;
////		Usart1Device.bufStartNum = 0;
//		Usart1Device.isFree = 1;
//	}
//	else{
//				Usart1Device.flushLen = 0;
//						Usart1Device.flushStartNum = 0;
//				Usart1Device.bufStartNum = 0;
//				Usart1Device.transLen=0;
//		Usart1Device.isFree = 1;
//	}
//
//}
