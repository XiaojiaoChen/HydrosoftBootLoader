/*
 * myQueue.h
 *
 *  Created on: Jan 3, 2022
 *      Author: hydrosoft
 */

#ifndef INC_MYQUEUE_H_
#define INC_MYQUEUE_H_

#include "main.h"
#define MY_QUEUE_SIZE 1280

typedef struct MYQUEUETAG{
	uint8_t buf[MY_QUEUE_SIZE];
	int count;//number of stored data
	int front;//beginning index of stored data
	int rear;//beginning index of free buffer

}MYQUEUE;

int queue_isFull(MYQUEUE *q);
int queue_isEmpty(MYQUEUE *q);

void queue_in(MYQUEUE *q,uint8_t item);
uint8_t queue_out(MYQUEUE *q);

HAL_StatusTypeDef queue_in_bulk(MYQUEUE *q,uint8_t *pSrc,int len);
HAL_StatusTypeDef queue_out_bulk(MYQUEUE *q,uint8_t *pdes,int len);

#endif /* INC_MYQUEUE_H_ */
