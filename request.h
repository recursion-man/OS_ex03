#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <sys/time.h>

typedef struct
{
    struct timeval arrival;
    struct timeval dispatch;
    int thread_id;
    int thread_count;
    int thread_static;
    int thread_dynamic;
} Stats;

typedef struct Node
{
    int fd;
    struct Node *next;
    Stats stats;
} Node;

void requestHandle(Node*);

#endif
