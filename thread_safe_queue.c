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

   // number_of_threads = _number_of_threads;
    q->m_sched_policy = policy;
    q->capacity = queue_size;
    q->max_dynamic_capacity = max_capacity;
    
   // block_flush_on = 0;
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
void addFdToQueue(Queue *q, Node *request)
{
    //  get pending queue and lock it
    PendingQueue *pending_tasks = q->m_pending_queue;
    TasksList *tasks_list = q->m_tasks_list;
    
    pthread_mutex_lock(&q_lock);
	
	
    //  check if the pending queue is full
    if (pending_tasks->size + tasks_list->size < q->capacity)
    {
    
        addToPendingQueue(q, request);
        pthread_mutex_unlock(&q_lock);
        
    }

    // if the Queue is full, act as the policy indicates
    else
    {
    
        checkIfCond(q, request);
        pthread_mutex_unlock(&q_lock);
    }
}

// call only after q_lock is locked
// no locks in this function - to prevent deadlocks!!
void addToPendingQueue(Queue *q, Node *request)
{

    PendingQueue *pending_queue = q->m_pending_queue;
    //  check if it's the first Node inserted
    if (pending_queue->size == 0)
    {
        pending_queue->head = request;
        pending_queue->tail = request;
    }
    else
    {
        pending_queue->tail->next = request;
        pending_queue->tail = pending_queue->tail->next;
    }

    //  increase size
    pending_queue->size++;
}

// call only after q_lock is locked
// no locks in this function - to prevent deadlocks!!
Node *getJobFromPendingQueue(PendingQueue *pending_queue)
{
    //  check if it's the first Node to be inserted
    if (pending_queue->size <= 0)
    {
     //   fprintf(stderr, "queue is already empty\n");
        return NULL;
    }

    // update dispatch interval in request Node
    Node *target = pending_queue->head;
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    target->stats.dispatch.tv_sec = current_time.tv_sec - target->stats.arrival.tv_sec;
    target->stats.dispatch.tv_usec = current_time.tv_usec - target->stats.arrival.tv_usec;

    // update head to point to the next Node (casting is required because C sucks)
    pending_queue->head = (Node *)target->next;

    // If the head becomes NULL, update the tail to NULL as well
    if (!pending_queue->head)
    {
        pending_queue->tail = NULL;
    }

    pending_queue->size--;

    return target;
}

// call only after q_lock is locked
// no locks in this function - to prevent deadlocks!!
void addToTaskList(TasksList *tasks_list, Node *request)
{

    //  check if it's the first Node inserted
    if (tasks_list->size == 0)
    {
        tasks_list->head = request;
        tasks_list->tail = request;
    }
    else
    {
        tasks_list->tail->next = request;
        tasks_list->tail = tasks_list->tail->next;
    }

    //  increase size
    tasks_list->size++;
}

void removeFromTaskList(Queue *q, Node *request)
{
    TasksList *tasks_list = q->m_tasks_list;
    Node *temp = tasks_list->head;
    Node *prev = NULL;

    // in case we need to delete the head
    if (temp != NULL && temp->fd == request->fd)
    {
        tasks_list->head = (Node *)temp->next;
        if (!temp->next)
            tasks_list->tail = NULL;
        free(temp);
        return;
    }

    // iterate through the list
    while (temp != NULL && temp->fd != request->fd)
    {
        prev = temp;
        temp = (Node *)temp->next;
    }

    // If the fd isn't there
    if (temp == NULL)
    {
    //    printf("Element not found in the list.\n");
        return;
    }

    // updating the previous node's next pointer.
    // check if we need to update the tail
    prev->next = temp->next;
    if (!temp->next)
        tasks_list->tail = prev; // Note: should it be: tasks_list->tail = prev?

    free(temp);

    tasks_list->size--;

    //  signal that there is space in the pending queue
    if (q->m_sched_policy == block_flush)
    {
        if (q->m_pending_queue->size + q->m_tasks_list->size == 0)
            pthread_cond_signal(&ready_to_insert);
    }
    else
        pthread_cond_signal(&(ready_to_insert));
}

int checkIfCond(Queue *q, Node *request)
{
    Sched_Policy policy = q->m_sched_policy;
    switch (policy)
    {
    case block:
        handleBlock(q, request);
        break;
    case drop_tail:
        handleDropTail(request);
        break;
    case drop_head:
        handleDropHead(q, request);
        break;
    case block_flush:
        handleBlockFlush(q, request);
        break;
    case Dynamic:
        handleDynamic(q, request);
        break;
    case drop_random:
        handleDropRandom(q, request);
        break;
    }
    return 1;
}

void handleBlock(Queue *q, Node *request)
{
    while (q->m_pending_queue->size + q->m_tasks_list->size == q->capacity)
    {
        pthread_cond_wait(&ready_to_insert, &q_lock);
    }
    addToPendingQueue(q, request);
}

void handleDropTail(Node *request)
{
    Close(request->fd);
    free(request);
}

void handleDropHead(Queue *q, Node *request)
{
    PendingQueue *pending_queue = q->m_pending_queue;
    removeFirstNodeInPendingList(pending_queue);
    addToPendingQueue(q, request);
}

void handleBlockFlush(Queue *q, Node *request)
{

    // stays in loop while queue isn't empty
    while (q->m_pending_queue->size || q->m_tasks_list->size)
    {
        pthread_cond_wait(&ready_to_insert, &q_lock);
    }

    // drop the request
    Close(request->fd);
    free(request);
}

void handleDynamic(Queue *q, Node *request)
{
    Close(request->fd);
    free(request);

    if (q->capacity == q->max_dynamic_capacity)
    {
     //   fprintf(stderr, "queue capacity is already max\n");
        return;
    }

    q->capacity++;
}

void removeFirstNodeInPendingList(PendingQueue *pending_queue)
{
    if (pending_queue->size <= 0)
    {
       // fprintf(stderr, "queue is already empty\n");
        return;
    }

    Node *target = pending_queue->head;
    Node *target_next = (Node *)target->next;
    pending_queue->head = target_next;

    // edge case: if pending queue capacity is 1 - so head = tail = NULL
    if (!target_next)
    {
        pending_queue->tail = NULL;
    }

    // drop oldest request
    Close(target->fd);
    free(target);

    pending_queue->size--;
}

void handleDropRandom(Queue *q, Node *request)
{
    //  get amount to drop
    int size_to_drop = (q->m_pending_queue->size) / 2;

    int current_size;
    while (size_to_drop > 0)
    {
        current_size = q->m_pending_queue->size;

        srand(time(NULL));

        // get a random number between 0 and (pending_queue current size - 1 indluded) - [0,pending_queue size -1]
        int node_index_to_drop = rand() % current_size;

        //  drop node according to index picked
        dropNodeByindex(q->m_pending_queue, node_index_to_drop);

        //  decrease amount to drop
        size_to_drop--;
    }

    //  add the new requst to the pending queue
    addToPendingQueue(q, request);
}

void dropNodeByindex(PendingQueue *pending_queue, int node_index_to_drop)
{
    //  validity check
    if (node_index_to_drop >= pending_queue->size)
    {
        return;
    }
    //  if first node was picked
    if (node_index_to_drop == 0)
    {
        Node *node_to_free = pending_queue->head;
        pending_queue->head = node_to_free->next;
        if (!pending_queue->head)
            pending_queue->tail = NULL;
        Close(node_to_free->fd);
        free(node_to_free);
        return;
    }

    //  start from second node
    int index = 1;
    Node *prev = pending_queue->head;
    Node *node_to_drop = pending_queue->head->next;

    //  find the node in the queue with the index required
    while (index < node_index_to_drop)
    {
        prev = node_to_drop;
        node_to_drop = node_to_drop->next;
        index++;
    }

    // link prev and next
    prev->next = node_to_drop->next;

    //  check if prev is now the new tail
    if (!prev->next)
        pending_queue->tail = prev;

    //  delete node
    Close(node_to_drop->fd);
    free(node_to_drop);
}
