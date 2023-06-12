//
// Created by tomso on 12/06/2023.
//

#ifndef WEBSERVER_FILES_THREAD_SAFE_QUEUE_H
#define WEBSERVER_FILES_THREAD_SAFE_QUEUE_H
#include "request.h"
#include "segel.h"

typedef enum
{
    block, drop_tail, drop_head, block_flush, Dynamic, drop_random
} sched_policy;

typedef struct {
    int* fd;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    int capacity;
    int tail;
    int head;
    int size;
} pending_queue;

typedef struct {
    int* fd;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} tasks_list;



typedef struct {
    tasks_list* m_tasks_list;
    pending_queue* m_pending_queue;
    sched_policy m_sched_policy;
} Queue;





#endif //WEBSERVER_FILES_THREAD_SAFE_QUEUE_H
