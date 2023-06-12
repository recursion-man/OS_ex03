#include "thread_safe_queue.h"

/*---------------------Constructor---------------------*/
void tasksList_init(TasksList *tasks, int number_of_threads)
{
    //  inital fields
    tasks->size = 0;
    tasks->number_of_threads = number_of_threads;
    tasks->front = 0;
    tasks->rear = 0;

    //  allocate "number_of_threads" cells
    tasks->fd = (int *)malloc(sizeof(int) * number_of_threads);

    //  initial locks and condition variables
    pthread_mutex_init(&(tasks->lock), NULL);
    pthread_cond_init(&(tasks->pending_queue_not_empty), NULL);
}

void pendingQueue_init(PendingQueue *pending_tasks, int size, int capacity)
{
    //  inital fields
    pending_tasks->size = size;
    pending_tasks->capacity = capacity;
    pending_tasks->head = NULL;
    pending_tasks->tail = NULL;

    //  initial locks and condition variables
    pthread_mutex_init(&(tasks->lock), NULL);
    pthread_cond_init(&(tasks->tasks_list_not_full), NULL);
}

/*
    queue_size - the arg given by the cmd line.
    Note: because there are already "number_of_threads" tasks in the tasks list, the amount o remaining tasks that can be in the pending queue is
    "queue_size" - "number_of_threads" (because we splitted the queue into to queue's)
    max_capacity - the arg given by the cmd line for max capacity for the dynamic choise in sched_policy
    Note: The capacity of the pending queue should be "max_capacity" - "number_of_threads", since there are also tasks in the tasks_list
*/
void Queue_init(Queue *q, int number_of_threads, int queue_size, int max_capacity, Sched_Policy policy)
{
    //  allocate memory
    q->m_pending_queue = (PendingQueue *)malloc(sizeof(PendingQueue));
    q->m_tasks_list = (TasksList *)malloc(sizeof(TasksList));

    //  initial tasks_list
    tasksList_init(q->m_tasks_list, number_of_threads);

    //  calculate the pending queue sizes according to the logic in the Note's in the comment above
    int size_of_pending_queue = queue_size - number_of_threads;
    int capacity_for_pending_queue = max_capacity - number_of_threads;

    //  initial pending_queue
    pendingQueue_init(q->m_pending_queue, size_of_pending_queue, capacity_for_pending_queue);
}

/*---------------------D'tors---------------------*/

void tasksListDestroy(TasksList *tasks)
{
    free(tasks->fd);
    pthread_mutex_destroy(&(tasks->lock));
    pthread_cond_destroy(&(tasks->pending_queue_not_empty));
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

    pthread_mutex_destroy(&(pending_queue->lock));
    pthread_cond_destroy(&(pending_queue->tasks_list_not_full));
}

void QueueDestroy(Queue *queue)
{
    pendingQueueDestroy(queue->m_pending_queue);
    tasksListDestroy(queue->m_tasks_list);
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
    pthread_mutex_lock(&(pending_tasks->lock), NULL);

    //  check if the pending queue is full
    if (pending_tasks->size < pending_tasks->capacity)
    {
        //  save current size inorder to know if the queue was empty, to signal the threads
        int prev_size = pending_tasks->size;

        //  add to pending queue
        addToPendingQueue(pending_tasks, fd);

        //  signal the thread if the queue was empty before and now it's not
        if (prev_size == 0)
            pthread_cond_signal(&(q->m_tasks_list->m_pending_queue_not_empty));
    }
    else
    {
        // do some sched shit
    }
    pthread_mutex_unlock(&(pending_tasks->lock));
}

void addToPendingQueue(PendingQueue *pending_queue, int fd)
{
    //  create new node
    Node *new_fd_request = (Node *)malloc(sizeof(Node));

    //  set data
    new_fd_request->fd = fd;
    new_fd_request->next = NULL;

    //  check if it's the first Node inserted
    if (pending_queue->size == 0)
    {
        pending_queue->head = new_fd_request;
        pending_queue->tail = new_fd_request;
    }
    else
    {
        pending_queue->tail->next = new_fd_request;
        pending_queue->tail = pending_queue->tail->next;
    }

    //  increase size
    pending_queue->size++;
}