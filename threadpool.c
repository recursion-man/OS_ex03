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

        pthread_mutex_lock(&(q_lock));

        //  check when there is a task waiting in the pending queue
        while (pending_queue->size == 0)
        {
            pthread_cond_wait(&(pending_queue_not_empty), &(q_lock));
        }

        //  get oldest task in the pending queue
        int task_fd = getJobFromPendingQueue(pending_queue);

        //  insert task to the tasks list for tasks that being handled currently
        addToTaskList(task_list, task_fd);

        pthread_mutex_unlock(&(q_lock));

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

        pthread_mutex_lock(&(q_lock));

        //  remove task from the tasks list because it's finished
        removeFromTaskList(task_list, task_fd);

        pthread_mutex_unlock(&(q_lock));

        // //  thread has finished to handle the requset
        // pthread_cond_signal(&(pending_queue->tasks_list_not_full));
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