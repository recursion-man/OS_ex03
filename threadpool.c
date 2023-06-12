//
// Created by tomso on 12/06/2023.
//
#include "threadpool.h"

// i just copied from gpt.. I'll continue later...
void* worker_func(void* arg, threadpool* _threadpool){
    Queue* task_queue = _threadpool->task_queue;
    Queue* pending_queue = _threadpool->pending_queue;

    while (1) {
        pthread_mutex_lock(&(task_queue->mutex));

        while (task_queue->size == 0) {
            pthread_cond_wait(&(task_queue->condition), &(task_queue->mutex));
        }

        Task task = task_queue->tasks[task_queue->front];
        task_queue->front = (task_queue->front + 1) % task_queue->capacity;
        task_queue->size--;

        pthread_mutex_unlock(&(task_queue->mutex));

        task.task_function(task.fd);

        pthread_mutex_lock(&(pending_queue->mutex));
        Queue* temp = task_queue;
        task_queue = pending_queue;
        pending_queue = temp;
        pthread_mutex_unlock(&(pending_queue->mutex));

        pthread_cond_signal(&(pending_queue->condition));
    }

    pthread_exit(NULL);
}