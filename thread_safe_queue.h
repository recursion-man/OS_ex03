//
// Created by tomso on 12/06/2023.
//

#ifndef WEBSERVER_FILES_THREAD_SAFE_QUEUE_H
#define WEBSERVER_FILES_THREAD_SAFE_QUEUE_H
#include "request.h"
#include "segel.h"


pthread_mutex_t q_lock;
pthread_cond_t ready_to_insert;
static int number_of_threads;

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
    int size;


    // I think we dont need this condition
    pthread_cond_t tasks_list_not_full;

} PendingQueue;

typedef struct
{
    int size;
    Node *head;
    Node *tail;

    // I think we dont need this condition
    pthread_cond_t pending_queue_not_empty;
} TasksList;



typedef struct
{
    TasksList *m_tasks_list;
    PendingQueue *m_pending_queue;
    Sched_Policy m_sched_policy;
    int capacity;
    int max_dynamic_capacity;
} Queue;

void pendingQueueDestroy(PendingQueue *pending_queue);
void pendingQueue_init(PendingQueue *pending_tasks);

void tasksListDestroy(TasksList *tasks);
void tasksList_init(TasksList *tasks);

void QueueDestroy(Queue *queue);
void Queue_init(Queue *q, int number_of_threads, int number_of_request_connection, int max_of_request_connection, Sched_Policy policy);

void addFdToQueue(Queue *q, int fd);
void addToPendingQueue(PendingQueue *pending_queue, int fd);

void policyHandler(Queue* q, int fd);

#endif // WEBSERVER_FILES_THREAD_SAFE_QUEUE_H
