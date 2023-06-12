//
// Created by tomso on 12/06/2023.
//

#ifndef WEBSERVER_FILES_THREAD_SAFE_QUEUE_H
#define WEBSERVER_FILES_THREAD_SAFE_QUEUE_H
#include "request.h"
#include "segel.h"

typedef enum
{
    block,
    drop_tail,
    drop_head,
    block_flush,
    Dynamic,
    drop_random
} Sched_Policy;

typedef struct Node
{
    int fd;
    struct Node *next;
} Node;

typedef struct
{
    Node *head;
    Node *tail;
    pthread_mutex_t mutex;

    // I think we dont need this condition
    pthread_cond_t tasks_list_not_full;
    int capacity;
    int size;
} PendingQueue;

typedef struct
{
    int *fd;
    int size;
    int number_of_threads;
    int front;
    int rear;
    pthread_mutex_t lock;
    pthread_cond_t pending_queue_not_empty;
} TasksList;

typedef struct
{
    TasksList *m_tasks_list;
    PendingQueue *m_pending_queue;
    Sched_Policy m_sched_policy;
} Queue;

void pendingQueueDestroy(PendingQueue *pending_queue);
void pendingQueue_init(PendingQueue *pending_tasks, int size, int capacity);

void tasksListDestroy(TasksList *tasks);
void tasksList_init(TasksList *tasks, int number_of_threads);

void QueueDestroy(Queue *queue);
void Queue_init(Queue *q, int number_of_threads, int number_of_request_connection, int max_of_request_connection, Sched_Policy policy);

void addFdToQueue(Queue *q, int fd);
void addToPendingQueue(PendingQueue *pending_queue, int fd);

#endif // WEBSERVER_FILES_THREAD_SAFE_QUEUE_H
