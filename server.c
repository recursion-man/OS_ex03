#include "segel.h"
#include "request.h"
#include "threadpool.h"

//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// HW3: Parse the new arguments too
void getargs(int *port, int *num_of_threads, int *queue_size, int *max_size, Sched_Policy *sched_policy, int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);

    // Note: we need to add a validation chcek to see that they are number and positives
    *num_of_threads = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
#include <string.h>

    char *policy = argv[4];

//  set max_size to -1, if the policy is dynamic, we'll update it
    *max_size = -1;

    if (strcmp(policy, "block") == 0) {
        *sched_policy = block;
    } else if (strcmp(policy, "drop_tail") == 0) {
        *sched_policy = drop_tail;
    } else if (strcmp(policy, "drop_head") == 0) {
        *sched_policy = drop_head;
    } else if (strcmp(policy, "block_flush") == 0) {
        *sched_policy = block_flush;
    } else if (strcmp(policy, "dynamic") == 0) {
        *sched_policy = Dynamic;
        *max_size = atoi(argv[5]);
    } else if (strcmp(policy, "drop_random") == 0) {
        *sched_policy = drop_random;
    }

}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, num_of_threads, queue_size, max_size;
    struct sockaddr_in clientaddr;
    Sched_Policy sched_policy;

    getargs(&port, &num_of_threads, &queue_size, &max_size, &sched_policy, argc, argv);

    //
    // HW3: Create some threads...
    //

    // initial args for worker_func
    ThreadPool* thread_pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    workerFuncArgs worker_func_args[number_of_threads];
    for (int i = 0; i < number_of_threads; i++) {
        worker_func_args[i].threadPool = thread_pool;
        worker_func_args[i].index = i;
    }
    threadPool_init(worker_func_args, num_of_threads, queue_size, max_size, sched_policy);

    listenfd = Open_listenfd(port);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);
        struct timeval arrival_time;
        gettimeofday(&arrival_time, NULL);

        // create new request Node
        Node *new_fd_request = (Node *)malloc(sizeof(Node));
        //  set data
        new_fd_request->fd = connfd;
        new_fd_request->next = NULL;
        new_fd_request->stats.arrival=arrival_time;

        addFdToQueue(thread_pool->pool_queue, new_fd_request);
    }
}
