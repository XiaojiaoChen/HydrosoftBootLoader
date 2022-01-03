/*
 * myQueue.c
 *
 *  Created on: Jan 3, 2022
 *      Author: hydrosoft
 */

#include "myQueue.h"



void queue_init(MYQUEUE *q) {
	q->count = 0;
	q->front = 0;
	q->rear = -1;
}
int queue_isFull(MYQUEUE *q) {
	return (q->count >= (MY_QUEUE_SIZE - 1));
}
int queue_isEmpty(MYQUEUE *q) {
	return (q->count <= 0);
}
void queue_in(MYQUEUE *q,uint8_t item) {
	if (!queue_isFull(q)) {
		q->count++;
		q->rear = (q->rear + 1) % MY_QUEUE_SIZE;
		q->buf[q->rear] = item;
	}
}
uint8_t queue_out(MYQUEUE *q) {
	uint8_t item=0;
	if (!queue_isEmpty(q)){
		q->count--;
		item = q->buf[q->front];
		q->front = (q->front + 1) % MY_QUEUE_SIZE;
	}
	return item;
}

