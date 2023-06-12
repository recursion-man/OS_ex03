//
// Created by tomso on 12/06/2023.
//

#ifndef WEBSERVER_FILES_THREADPOOL_H
#define WEBSERVER_FILES_THREADPOOL_H
#include "segel.h"
#include "thread_safe_queue.h"

typedef struct {
    pthread_t* threads;
    int num_threads;
    Queue* pool_queue;
} threadpool;


void* worker_func(void* arg, threadpool* _threadpool);

#endif //WEBSERVER_FILES_THREADPOOL_H
