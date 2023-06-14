#include "segel.h"
#include "request.h"
#include "threadpool.h"
#include "thread_safe_queue.h"
#include <string.h>


//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is daddone within routines written in request.c
//

// HW3: Parse the new arguments too

int isNumber(char* str){
	if(str==NULL||*str=='\0'){
		return 0;
	}
	while(*str!='\0'){
		if(*str>'9'||*str<'0'){
			return 0;
		}
		str++;
	}
	return 1;
}
void getargs(int *port, int *num_of_threads, int *queue_size, int *max_size, Sched_Policy *sched_policy, int argc, char *argv[])
{
    if (argc < 5)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    if(!isNumber(argv[1])||!isNumber(argv[2])||!isNumber(argv[3]))
		exit(1);
    *port = atoi(argv[1]);

    // Note: we need to add a validation chcek to see that they are number and positives
    
    *num_of_threads = atoi(argv[2]);
    *queue_size = atoi(argv[3]);

    char *policy = argv[4];

    //  set max_size to -1, if the policy is dynamic, we'll update it
    *max_size = -1;

    if (strcmp(policy, "block") == 0)
    {
        *sched_policy = block;
    }
    else if (strcmp(policy, "dt") == 0)
    {
        *sched_policy = drop_tail;
    }
    else if (strcmp(policy, "dh") == 0)
    {
        *sched_policy = drop_head;
    }
    else if (strcmp(policy, "bf") == 0)
    {
        *sched_policy = block_flush;
    }
    else if (strcmp(policy, "dynamic") == 0)
    {
        *sched_policy = Dynamic;
        if(argc!=6||!isNumber(argv[5]))
			exit(1);
        *max_size = atoi(argv[5]);
    }
    else if (strcmp(policy, "random") == 0)
    {
        *sched_policy = drop_random;
    }
    else{
		exit(1);
	}
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, num_of_threads=-1, queue_size=-1, max_size=-1;
    struct sockaddr_in clientaddr;
    Sched_Policy sched_policy=-1;


    getargs(&port, &num_of_threads, &queue_size, &max_size, &sched_policy, argc, argv);

    //
    // HW3: Create some threads...
    //

    // initial args for worker_func
    ThreadPool *thread_pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    workerFuncArgs *worker_func_args = (workerFuncArgs *)malloc(sizeof(workerFuncArgs) * num_of_threads);

    for (int i = 0; i < num_of_threads; i++)
    {
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
        new_fd_request->stats.arrival = arrival_time;

        addFdToQueue(thread_pool->pool_queue, new_fd_request);
    }

    //  free memory
    free(worker_func_args);
    threadPoolDestroy(thread_pool);
}


		// for debugging use this code:
		//FILE* file = fopen("error.txt", "a");
		//fprintf(file, "capacity = %d, p-size = %d, t-size = %d\n", q->capacity,pending_tasks->size, tasks_list->size);
        //fclose(file);
		//exit(1);
