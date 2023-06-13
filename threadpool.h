//
// Created by tomso on 12/06/2023.
//

#ifndef WEBSERVER_FILES_THREADPOOL_H
#define WEBSERVER_FILES_THREADPOOL_H
#include "segel.h"
#include "thread_safe_queue.h"

typedef struct
{
    pthread_t *threads;
    int num_threads;
    Queue *pool_queue;
} ThreadPool;

void threadPool_init(ThreadPool *threadpool, int number_of_threads, int queue_size, int max_size, Sched_Policy sched_policy);
void threadPoolDestroy(ThreadPool *threadpool);
void *worker_func(void *arg);

#endif // WEBSERVER_FILES_THREADPOOL_H
