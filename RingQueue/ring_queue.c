#include <stdlib.h>
#include <semaphore.h>
#include "ring_queue.h"

struct RingQueue_s
{
    sem_t item_cnt;
    sem_t free_cnt;
    sem_t put_lock;
    sem_t get_lock;
    size_t size;
    size_t head;
    size_t tail;
    QueueItem rbuff[];
};

RingQueue* ring_queue_new(size_t size)
{
    RingQueue* q=malloc(sizeof(RingQueue)+sizeof(QueueItem)*size);
    if(q==NULL)
    {
        return NULL;
    }
    sem_init(&q->item_cnt,0,0);
    sem_init(&q->free_cnt,0,size);
    sem_init(&q->put_lock,0,1);
    sem_init(&q->get_lock,0,1);
    q->size=size;
    q->head=0;
    q->tail=0;
    return q;
}

void ring_queue_free(RingQueue* q)
{
    free(q);
}

bool ring_queue_get(RingQueue* q,QueueItem* i)
{
    if(q==NULL) return false;
    if(sem_wait(&q->item_cnt)!=0) return false;
    if(sem_wait(&q->get_lock)!=0) return false;
    *i=q->rbuff[q->head];
    q->head=(q->head+1)%q->size;
    sem_post(&q->get_lock);
    sem_post(&q->free_cnt);
    return true;
}

bool ring_queue_try_get(RingQueue* q,QueueItem* i)
{
    if(q==NULL) return false;
    if(sem_trywait(&q->item_cnt)!=0) return false;
    if(sem_wait(&q->get_lock)!=0) return false;
    *i=q->rbuff[q->head];
    q->head=(q->head+1)%q->size;
    sem_post(&q->get_lock);
    sem_post(&q->free_cnt);
    return true;
}

bool ring_queue_put(RingQueue* q,QueueItem i)
{
    if(q==NULL) return false;
    if(sem_wait(&q->free_cnt)!=0) return false;
    if(sem_wait(&q->put_lock)!=0) return false;
    q->rbuff[q->tail]=i;
    q->tail=(q->tail+1)%q->size;
    sem_post(&q->put_lock);
    sem_post(&q->item_cnt);
    return true;
}

bool ring_queue_try_put(RingQueue* q,QueueItem i)
{
    if(q==NULL) return false;
    if(sem_trywait(&q->free_cnt)!=0) return false;
    if(sem_wait(&q->put_lock)!=0) return false;
    q->rbuff[q->tail]=i;
    q->tail=(q->tail+1)%q->size;
    sem_post(&q->put_lock);
    sem_post(&q->item_cnt);
    return true;
}
