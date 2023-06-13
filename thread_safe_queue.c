#include "thread_safe_queue.h"

/*---------------------Constructor---------------------*/
void tasksList_init(TasksList *tasks_list)
{
    //  inital fields
    tasks_list->size = 0;
    tasks_list->head = NULL;
    tasks_list->tail = NULL;
}

void pendingQueue_init(PendingQueue *pending_tasks)
{
    //  inital fields
    pending_tasks->size = 0;
    pending_tasks->head = NULL;
    pending_tasks->tail = NULL;
}

/*
    queue_size - the arg given by the cmd line.
    Note: because there are already "number_of_threads" tasks in the tasks list, the amount o remaining tasks that can be in the pending queue is
    "queue_size" - "number_of_threads" (because we splitted the queue into to queue's)
    max_capacity - the arg given by the cmd line for max capacity for the dynamic choice in sched_policy
    Note: The capacity of the pending queue should be "max_capacity" - "number_of_threads", since there are also tasks in the tasks_list
*/
void Queue_init(Queue *q, int _number_of_threads, int queue_size, int max_capacity, Sched_Policy policy)
{
    //  allocate memory
    q->m_pending_queue = (PendingQueue *)malloc(sizeof(PendingQueue));
    q->m_tasks_list = (TasksList *)malloc(sizeof(TasksList));

    //  initial tasks_list
    tasksList_init(q->m_tasks_list);

    //  initial pending_queue
    pendingQueue_init(q->m_pending_queue);

    number_of_threads = _number_of_threads;
}

/*---------------------D'tors---------------------*/

void tasksListDestroy(TasksList *tasks_list)
{
    Node *current = tasks_list->head;
    Node *next;
    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }
}

void pendingQueueDestroy(PendingQueue *pending_queue)
{
    Node *current = pending_queue->head;
    Node *next;
    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }
}

void QueueDestroy(Queue *queue)
{
    pendingQueueDestroy(queue->m_pending_queue);
    tasksListDestroy(queue->m_tasks_list);
    pthread_mutex_destroy(&(q_lock));
    pthread_cond_destroy(&(ready_to_insert));
}

/*---------------------AUX---------------------*/
/*
    The logic is as follows:
    add the job to the pending queue always, when there is a free thread, he would take the first task from the pending queue and transfer it
    to the task_list( which contains only tasks that being handled currently by a thread)
    Note: -if the pending queue was empty, he would singal to the threads that a new tasks has arrived
          -if the pending queue is full, we need to act according to the sched_policy
*/
void addFdToQueue(Queue *q, int fd)
{
    //  get pending queue and lock it
    PendingQueue *pending_tasks = q->m_pending_queue;
    TasksList *tasks_list = q->m_tasks_list;
    pthread_mutex_lock(&q_lock);

    //  check if the pending queue is full
    if (pending_tasks->size + tasks_list->size < q->capacity && !block_flush_on)
    {
        addToPendingQueue(q, fd);
        pthread_mutex_unlock(&q_lock);
    }

    // if the Queue is full, act as the policy indicates
    else
    {
        policyHandler(q, fd);
    }
}

// call only after q_lock is locked
// no locks in this function - to prevent deadlocks!!
void addToPendingQueue(Queue* q, int fd)
{
    //  create new node
    Node *new_fd_request = (Node *)malloc(sizeof(Node));

    //  set data
    new_fd_request->fd = fd;
    new_fd_request->next = NULL;

    PendingQueue* pending_queue = q->m_pending_queue;
    //  check if it's the first Node inserted
    if (pending_queue->size == 1)
    {
        pending_queue->head = new_fd_request;
        pending_queue->tail = new_fd_request;
        pthread_cond_signal(&(pending_queue_not_empty));
    }
    else
    {
        pending_queue->tail->next = new_fd_request;
        pending_queue->tail = pending_queue->tail->next;
    }

    //  increase size
    pending_queue->size++;

    // checks if should turn block_flush on
    if (sched_policy == block_flush)
    {
        if (pending_queue->size + q->m_tasks_list->size == q->capacity)
            block_flush_on = 1;
    }
}

// call only after q_lock is locked
// no locks in this function - to prevent deadlocks!!
int getJobFromPendingQueue(PendingQueue *pending_queue)
{
    //  check if it's the first Node inserted
    if (pending_queue->size <= 0)
    {
        fprintf(stderr, "queue is already empty\n");
        return -1;
    }

    Node *target = pending_queue->head;
    int target_fd = target->fd;
    Node *target_next = target->next;
    pending_queue->head = target_next;

    // If the head becomes NULL, update the tail to NULL as well
    if (!target_next)
    {
        pending_queue->tail = NULL;
    }

    free(target);
    pending_queue->size--;

    //  signal that there is space in the pending queue
    pthread_cond_signal(&(ready_to_insert));

    return target_fd;
}

// call only after q_lock is locked
// no locks in this function - to prevent deadlocks!!
void addToTaskList(TasksList *tasks_list, int fd)
{
    //  create new node
    Node *new_fd_request = (Node *)malloc(sizeof(Node));

    //  set data
    new_fd_request->fd = fd;
    new_fd_request->next = NULL;

    //  check if it's the first Node inserted
    if (tasks_list->size == 0)
    {
        tasks_list->head = new_fd_request;
        tasks_list->tail = new_fd_request;
    }
    else
    {
        tasks_list->tail->next = new_fd_request;
        tasks_list->tail = tasks_list->tail->next;
    }

    //  increase size
    tasks_list->size++;
}

void removeFromTaskList(Queue *q, int target_fd)
{
    TasksList* tasks_list = q->m_tasks_list;
    Node *temp = tasks_list->head;
    Node *prev = NULL;

    // in case we need to delete the head
    if (temp != NULL && temp->fd == target_fd)
    {
        tasks_list->head = temp->next;
        if (!temp->next)
            tasks_list->tail = NULL;
        free(temp);
        return;
    }

    // iterate through the list
    while (temp != NULL && temp->fd != target_fd)
    {
        prev = temp;
        temp = temp->next;
    }

    // If the fd isn't there
    if (temp == NULL)
    {
        printf("Element not found in the list.\n");
        return;
    }

    // updating the previous node's next pointer.
    // check if we need to update the tail
    prev->next = temp->next;
    if (!temp->next)
        tasks_list->tail = NULL; // Note: should it be: tasks_list->tail = prev?

    free(temp);

    tasks_list->size--;

    if (sched_policy == block_flush)
    {
        if (q->m_pending_queue->size + q->m_tasks_list->size ==0)
            block_flush_on = 0;
    }
}

/// wait (without busy-wait) for the condition specified in the policy
void policyHandler(Queue *q, int fd)
{
    Sched_Policy policy = q->m_sched_policy;

    // create cond depending on the policy;
    // no bool type in c, so I used int
    int cond;
    while (!cond)
    {
        pthread_cond_wait(&ready_to_insert, &q_lock);
    }
}

int checkIfCond(Queue* q,int fd){
    Sched_Policy policy = q->m_sched_policy;
    switch (policy) {
        case block:
            handleBlock(q,fd);
            break;
        case drop_tail:
            handleDropTail(q,fd);
            break;
        case drop_head:
            handleDropHead(q,fd);
            break;
        case block_flush:
            handleBlockFlush(q,fd);
            break;
        case Dynamic:
            handleDynamic(q,fd);
            break;
    }
}

void handleBlock(Queue* q, int fd){
    while (q->m_pending_queue->size + q->m_tasks_list->size == q->capacity)
    {
        pthread_cond_wait(&ready_to_insert, &q_lock);
    }
    addToPendingQueue(q, fd);
}

void handleDropTail(Queue* q, int fd){
    Close(fd);
}

void handleDropHead(Queue* q, int fd)
{
    PendingQueue* pending_queue = q->m_pending_queue;
    removeLastNodeInPendingList(pending_queue);
    addToPendingQueue(q, fd);
}

void handleBlockFlush(Queue* q, int fd)
{
    // block_flush is turned on when the queue gets full
    //block flush turned off when the queue gets empty
    while ( (q->m_pending_queue->size ||  q->m_tasks_list->size) && block_flush_on)
    {
        pthread_cond_wait(&ready_to_insert, &q_lock);
    }

    addToPendingQueue(q, fd);
}

void handleDynamic(Queue* q, int fd){
    Close(fd);

    if (q->capacity == q->max_dynamic_capacity){
        fprintf(stderr, "queue capacity is full\n");
        return;
    }

    q->capacity++;
}

void removeLastNodeInPendingList(PendingQueue* pending_queue){
    if (pending_queue->size <= 0)
    {
        fprintf(stderr, "queue is already empty\n");
        return;
    }

    Node *target = pending_queue->head;
    int target_fd = target->fd;
    Node *target_next = target->next;
    pending_queue->head = target_next;

    // edge case- if pending queue capacity is 1- so head = tail = NULL
    if (!target_next)
    {
        pending_queue->tail = NULL;
    }

    free(target);
    pending_queue->size--;
}

