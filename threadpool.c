//
// Created by tomso on 12/06/2023.
//
#include "threadpool.h"

// i just copied from gpt.. I'll continue later...
void *worker_func(void *arg, ThreadPool *_threadpool)
{
    //  get the two queues
    TasksList *task_list = _threadpool->pool_queue->m_tasks_list;
    PendingQueue *pending_queue = _threadpool->pending_queue;

    while (1)
    {

        pthread_mutex_lock(&(pending_queue->lock));

        //  check when there is a task waiting in the pending queue
        while (pending_queue->size == 0)
        {
            pthread_cond_wait(&(task_list->pending_queue_not_empty), &(pending_queue->lock));
        }

        //  get oldest task in the queue
        int task_fd = pending_queue->head->fd;

        //  remove the task from pending queue (and free the pointer)
        Node *temp = pending_queue->head;
        pending_queue->head = pending_queue->head->next;
        pending_queue->size--;
        free(temp);

        pthread_mutex_unlock(&(pending_queue->lock));

        //  insert task to the tasks list for tasks that being handled currently
        pthread_mutex_lock(&(task_list->lock));

        task_list->fd[task_list->rear] = task_fd;
        task_list->rear = (task_list->rear + 1) % task_list->size;

        pthread_mutex_unlock(&(task_list->lock));
        //  segel's functions to handle the fd requset
        requestHandle(task_fd);
        Close(connfd);

        //  I have no clue what chatGpt tried to do here
        /*
                pthread_mutex_lock(&(pending_queue->mutex));
                Queue *temp = task_list;
                task_list = pending_queue;
                pending_queue = temp;
                pthread_mutex_unlock(&(pending_queue->mutex));
        */

        //  remove task from the tasks list because it's finished
        task_list->front = (task_list->front + 1) % task_list->size;

        //  thread has finished to handle the requset
        pthread_cond_signal(&(pending_queue->tasks_list_not_full));
    }

    pthread_exit(NULL);
}

void threadPool_init(ThreadPool *threadpool, int number_of_threads, int queue_size, int max_size, Sched_Policy sched_policy)
{
    //  initial fields
    threadpool->num_threads = number_of_threads;
    threadpool->pool_queue = (Queue *)malloc(sizeof(Queue));

    //  initial queue
    Queue_init(threadpool->pool_queue, number_of_threads, queue_size, max_size, sched_policy);

    //  allocate memory for threads array
    threadpool->threads = (pthread_t *)malloc(sizeof(threadpool->threads));

    //  create threads
    for (int i = 0; i < number_of_threads; i++)
    {
        pthread_create(&(threadpool->threads[i]), NULL, worker_func, ) // NOTE: to check what should be pasted as last argument?
    }
}

void threadPoolDestroy(ThreadPool *threadpool)
{
    QueueDestroy(threadpool->pool_queue);
    free(threadpool->threads);
}