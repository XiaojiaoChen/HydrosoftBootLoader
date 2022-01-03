/*
 * myQueue.h
 *
 *  Created on: Jan 3, 2022
 *      Author: hydrosoft
 */

#ifndef INC_MYQUEUE_H_
#define INC_MYQUEUE_H_

#include "main.h"
#define MY_QUEUE_SIZE (2048)

typedef struct MYQUEUETAG{
	uint8_t buf[MY_QUEUE_SIZE];
	int count;
	int front;
	int rear;

}MYQUEUE;

void queue_init(MYQUEUE *q);
void queue_in(MYQUEUE *q,uint8_t item);
int queue_isEmpty(MYQUEUE *q);
uint8_t queue_out(MYQUEUE *q);


#endif /* INC_MYQUEUE_H_ */
