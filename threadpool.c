//
// Created by tomso on 12/06/2023.
//
#include "threadpool.h"

// i just copied from gpt.. I'll continue later...
void *worker_func(void *arg)
{
    //  get the two queues
    ThreadPool* threadpool = (ThreadPool*) arg;
    Queue* q = threadpool->pool_queue;
    TasksList *task_list = q->m_tasks_list;
    PendingQueue *pending_queue = q->m_pending_queue;

    while (1) {

        pthread_mutex_lock(&(q_lock));

        //  check when there is a task waiting in the pending queue
        while (pending_queue->size == 0) {
            pthread_cond_wait(&(pending_queue_not_empty), &(q_lock));
        }

        //  get oldest task in the pending queue
        int task_fd = getJobFromPendingQueue(pending_queue);

        //  insert task to the tasks list for tasks that being handled currently
        addToTaskList(task_list, task_fd);

        pthread_mutex_unlock(&(q_lock));

        // handle the fd request
        requestHandle(task_fd);

        // catch a lock and remove the task
        pthread_mutex_lock(&(q_lock));
        removeFromTaskList(q, task_fd);
        pthread_mutex_unlock(&(q_lock));

        // close the connection fd
        Close(task_fd);
    }
}

void threadPool_init(ThreadPool *threadpool, int _number_of_threads, int queue_size, int max_size, Sched_Policy _sched_policy)
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
        pthread_create(&(threadpool->threads[i]), NULL, worker_func, &threadpool); // the last arg is the arg to be passed to worker_func

    }
}

void threadPoolDestroy(ThreadPool *threadpool)
{
    QueueDestroy(threadpool->pool_queue);
    free(threadpool->threads);
}