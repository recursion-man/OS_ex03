//
// Created by tomso on 12/06/2023.
//

#ifndef WEBSERVER_FILES_THREAD_SAFE_QUEUE_H
#define WEBSERVER_FILES_THREAD_SAFE_QUEUE_H
#include "request.h"
#include "segel.h"

pthread_mutex_t q_lock;
pthread_cond_t ready_to_insert;
pthread_cond_t pending_queue_not_empty;
static int number_of_threads;
// block_flush is turned on when the queue gets full
// block flush turned off when the queue gets empty
static int block_flush_on;

typedef enum
{
    block,
    drop_tail,
    drop_head,
    block_flush,
    Dynamic,
    drop_random
} Sched_Policy;

static Sched_Policy sched_policy;

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

void singnalIfCond(Queue* queue);
int checkIfCond(Queue* queue,int);

void tasksListDestroy(TasksList *tasks);
void tasksList_init(TasksList *tasks);

void QueueDestroy(Queue *queue);
void Queue_init(Queue *q, int number_of_threads, int number_of_request_connection, int max_of_request_connection, Sched_Policy policy);

void addFdToQueue(Queue *q, int fd);
void addToPendingQueue(Queue*, int fd);
void removeLastNodeInPendingList(PendingQueue *pending_queue);
int getJobFromPendingQueue(PendingQueue *pending_queue);

void addToTaskList(TasksList *tasks_list, int fd);
void removeFromTaskList(Queue*, int target_fd);

void policyHandler(Queue *q, int fd);

void handleBlock(Queue*,int);
void handleDropTail(Queue* q,int);
void handleDropHead(Queue* q,int);
void handleBlockFlush(Queue* q,int);
void handleDynamic(Queue* q,int);


#endif // WEBSERVER_FILES_THREAD_SAFE_QUEUE_H
