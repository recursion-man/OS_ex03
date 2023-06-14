#ifndef __REQUEST_H__


typedef struct
{
    struct timeval arrival;
    struct timeval dispatch;
    int thread_id;
    int thread_count;
    int thread_static;
    int thread_dynamic;
} Stats;

typedef struct
{
    int fd;
    struct Node *next;
    Stats stats;
} Node;

void requestHandle(Node*);

#endif
