/*
 * myQueue.c
 *
 *  Created on: Jan 3, 2022
 *      Author: hydrosoft
 */

#include "myQueue.h"

#include "stdio.h"
#include "string.h"


/**
 * get the length of the next continuous data block
 */
static int getDataSegmentNum(MYQUEUE *q){
	 if(q->front < q->rear)
		 return (q->rear - q->front);
	 else if(q->front > q->rear)
		 return (MY_QUEUE_SIZE - q->front);
	 else if(q->count>0)//q->front == q->rear && count>0, full data
		 return (MY_QUEUE_SIZE - q->front);
	 else//q->front == q->rear && count=0 , no data
		 return 0;
}

/**
 * get the length of the next available continuous free block for buffering
 */
static int getFreeSegmentNum(MYQUEUE *q){
	 if(q->front < q->rear)
		 return (MY_QUEUE_SIZE - q->rear);
	 else if(q->front > q->rear)
		 return (q->front - q->rear);
	 else if(q->count>0)//q->front == q->rear && count>0, full data
		 return 0;
	 else//q->front == q->rear && count=0 , no data
		 return (MY_QUEUE_SIZE - q->rear);
}

int queue_isFull(MYQUEUE *q) {
	return (q->count >= (MY_QUEUE_SIZE));
}
int queue_isEmpty(MYQUEUE *q) {
	return (q->count <= 0);
}
void queue_in(MYQUEUE *q,uint8_t item) {
	if (q->count + 1 <= MY_QUEUE_SIZE) {
		q->count++;
		q->buf[q->rear] = item;
		q->rear = (q->rear + 1) % MY_QUEUE_SIZE;
	}
}



uint8_t queue_out(MYQUEUE *q) {
	uint8_t item=0;
	if (q->count -1 >=0){
		q->count--;
		item = q->buf[q->front];
		q->front = (q->front + 1) % MY_QUEUE_SIZE;
	}
	return item;
}

HAL_StatusTypeDef queue_in_bulk(MYQUEUE *q,uint8_t *pSrc,int len){
	//if enough space, otherwise discard and return err
	if (q->count + len <= MY_QUEUE_SIZE) {

		int segLen = getFreeSegmentNum(q);

		/*if next continous free block is large enough to hold all the data*/
		if(segLen>=len){
			memcpy(&q->buf[q->rear],pSrc,len);
		}
		else{//wrap
			memcpy(&q->buf[q->rear],pSrc,segLen);
			memcpy(&q->buf[0],pSrc+segLen,(len-segLen));
		}

		/*update index*/
		q->count+=len;
		q->rear=(q->rear + len) % MY_QUEUE_SIZE;

		if(q->buf[1]==0){
			int bb=1;
		}

		return HAL_OK;
	}
	else
		return HAL_ERROR;


//	memcpy(&q->buf[q->rear],pSrc,len);
//	q->count+=len;
//	q->rear=(q->rear + len) % MY_QUEUE_SIZE;
//	return HAL_OK;

}

HAL_StatusTypeDef queue_out_bulk(MYQUEUE *q,uint8_t *pDes,int len){
	//enough data
	if (q->count-len>=0){

		int segLen = getDataSegmentNum(q);

		if(segLen>=len){
			memcpy(pDes,&q->buf[q->front],len);
		}
		else{
			memcpy(pDes,&q->buf[q->front],segLen);
			memcpy(pDes+segLen,&q->buf[0],len-segLen);
		}

		/*update index*/
		q->count-=len;
		q->front=(q->front + len) % MY_QUEUE_SIZE;

		return HAL_OK;
	}
	else{
		return HAL_ERROR;
	}
}

