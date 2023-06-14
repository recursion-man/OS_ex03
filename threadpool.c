//
// Created by tomso on 12/06/2023.
//
#include "threadpool.h"

// i just copied from gpt.. I'll continue later...
void *worker_func(void *arg)
{
    //  extract args
    workerFuncArgs *args = (workerFuncArgs *)arg;
    ThreadPool *threadpool = args->threadPool;
    int id = args->index;

    // extract queues
    Queue *q = threadpool->pool_queue;
    TasksList *task_list = q->m_tasks_list;
    PendingQueue *pending_queue = q->m_pending_queue;

    // initial counters
    int total_count = 0;
    int dynamic_count = 0;
    int static_count = 0;

    while (1)
    {

        pthread_mutex_lock(&(q_lock));

        //  check when there is a task waiting in the pending queue
        while (pending_queue->size == 0)
        {
            pthread_cond_wait(&(pending_queue_not_empty), &(q_lock));
        }

        //  get oldest task in the pending queue and remove it from the pending queue (will not free memory!)
        Node *request = getJobFromPendingQueue(pending_queue);

        //  insert task to the tasks list for tasks that being handled currently
        addToTaskList(task_list, request);

        pthread_mutex_unlock(&(q_lock));

        // fill stats for current request
        request->stats.thread_id = id;
        request->stats.thread_count = total_count;
        request->stats.thread_static = static_count;
        request->stats.thread_dynamic = dynamic_count;

        // print to the response headers
        printStatsToHeaders(request);

        // handle the fd request

        requestHandle(request);

        // update counters
        total_count++;
        static_count = request->stats.thread_static;
        dynamic_count = request->stats.thread_dynamic;

        // catch a lock and remove the task
        pthread_mutex_lock(&(q_lock));
        removeFromTaskList(q, request);
        pthread_mutex_unlock(&(q_lock));

        // close the connection fd
        Close(request->fd);
    }
}

void threadPool_init(workerFuncArgs *args, int _number_of_threads, int queue_size, int max_size, Sched_Policy _sched_policy)
{
    //  initial fields
    ThreadPool *threadpool = args->threadPool;
    threadpool->num_threads = _number_of_threads;
    threadpool->pool_queue = (Queue *)malloc(sizeof(Queue));

    //  initial queue
    Queue_init(threadpool->pool_queue, _number_of_threads, queue_size, max_size, _sched_policy);

    //  allocate memory for threads array
    threadpool->threads = (pthread_t *)malloc(sizeof(threadpool->threads));

    //  create threads
    for (int i = 0; i < _number_of_threads; i++)
    {
        // the last arg is the arg to be passed to worker_func: meaning : ThreadPool + index
        pthread_create(&(threadpool->threads[i]), NULL, worker_func, &args[i]);
    }
}

void threadPoolDestroy(ThreadPool *threadpool)
{
    QueueDestroy(threadpool->pool_queue);
    free(threadpool->threads);
}

void printStatsToHeaders(Node *request)
{
    char buf[MAXBUF];
    Stats stats = request->stats;

    // fill buffer
    sprintf(buf, "%sStat-Req-Arrival:: %lu.%06lu\r\n", buf, stats.arrival.tv_sec, stats.arrival.tv_usec);
    sprintf(buf, "%sStat-Req-Dispatch:: %lu.%06lu\r\n", buf, stats.arrival.tv_sec, stats.arrival.tv_usec);
    sprintf(buf, "%sStat-Thread-Id:: %d\r\n", buf, stats.thread_id);
    sprintf(buf, "%sStat-Thread-Count:: %d\r\n", buf, stats.thread_count);
    sprintf(buf, "%sStat-Thread-Static:: %d\r\n", buf, stats.thread_static);
    sprintf(buf, "%sStat-Thread-Dynamic:: %d\r\n", buf, stats.thread_dynamic);

    // write the content of the buffer
    Rio_writen(request->fd, buf, strlen(buf));
}
