#ifndef _RING_QUEUE_H_
#define _RING_QUEUE_H_

#include <stddef.h>
#include <stdbool.h>

typedef int QueueItem;
typedef struct RingQueue_s RingQueue;

RingQueue* ring_queue_new(size_t size);
void ring_queue_free(RingQueue* q);
bool ring_queue_get(RingQueue* q,QueueItem* i);
bool ring_queue_try_get(RingQueue* q,QueueItem* i);
bool ring_queue_timed_get(RingQueue* q,QueueItem* i,int sec);
bool ring_queue_put(RingQueue* q,QueueItem i);
bool ring_queue_try_put(RingQueue* q,QueueItem i);
bool ring_queue_timed_put(RingQueue* q,QueueItem i,int sec);

#endif /* _RING_QUEUE_H_ */
