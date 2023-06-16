/*
 * client.c: A very, very primitive HTTP client.
 * 
 * To run, try: 
 *      ./client www.cs.technion.ac.il 80 /
 *
 * Sends one HTTP request to the specified HTTP server.
 * Prints out the HTTP response.
 *
 * HW3: For testing your server, you will want to modify this client.  
 * For example:
 * 
 * You may want to make this multi-threaded so that you can 
 * send many requests simultaneously to the server.
 *
 * You may also want to be able to request different URIs; 
 * you may want to get more URIs from the command line 
 * or read the list from a file. 
 *
 * When we test your server, we will be using modifications to this client.
 *
 */

#include "segel.h"

/*
 * Send an HTTP request for the specified file 
 */
void clientSend(int fd, char *filename)
{
    char buf[MAXLINE];
    char hostname[MAXLINE];

    Gethostname(hostname, MAXLINE);

    /* Form and send the HTTP request */
    sprintf(buf, "GET %s HTTP/1.1\n", filename);
    sprintf(buf, "%shost: %s\n\r\n", buf, hostname);
    Rio_writen(fd, buf, strlen(buf));
}

/*
 * Read the HTTP response and print it out
 */
void clientPrint(int fd)
{
    rio_t rio;
    char buf[MAXBUF];
    int length = 0;
    int n;

    Rio_readinitb(&rio, fd);

    /* Read and display the HTTP Header */
    n = Rio_readlineb(&rio, buf, MAXBUF);
    while (strcmp(buf, "\r\n") && (n > 0)) {
        printf("Header: %s", buf);
        n = Rio_readlineb(&rio, buf, MAXBUF);

        /* If you want to look for certain HTTP tags... */
        if (sscanf(buf, "Content-Length: %d ", &length) == 1) {
            printf("Length = %d\n", length);
        }
    }

    /* Read and display the HTTP Body */
    n = Rio_readlineb(&rio, buf, MAXBUF);
    while (n > 0) {
        printf("%s", buf);
        n = Rio_readlineb(&rio, buf, MAXBUF);
    }
}

struct ClientThreadArguments {
    char *host;
    int port;
    char *filename;

};

void *performClientOperations(void *arguments) {
    // Cast the void pointer to the appropriate type
    struct ClientThreadArguments *args = (struct ClientThreadArguments *)arguments;
    fprintf(stderr, "%s", args->filename);
    int clientfd = Open_clientfd(args->host, args->port);

    clientSend(clientfd, args->filename);

    clientPrint(clientfd);


    exit(1);
}



int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <host> <port> <filename1> [filename2] [filename3] ...\n", argv[0]);
        exit(1);
    }

    char *host = argv[1];
    int port = atoi(argv[2]);

    int numFiles = argc - 3;
    char **filenames = malloc(numFiles * sizeof(char *));

    for (int i = 0; i < numFiles; i++) {
        filenames[i] = argv[i + 3];
    }

    pthread_t threads[numFiles];

    for (int i = 0; i < numFiles; i++) {
        // Create the arguments for the thread
        struct ClientThreadArguments *args = malloc(sizeof(struct ClientThreadArguments));
        args->host = host;
        args->port = port;
        args->filename = filenames[i];


        // Create the thread and pass the arguments
        if (pthread_create(&threads[i], NULL, performClientOperations, (void *)args) != 0) {
            fprintf(stderr, "Failed to create thread\n");
            exit(1);
        }
    }

    sleep(2);
    // Wait for all threads to finish
    for (int i = 0; i < numFiles; i++) {
        if (pthread_join(threads[i], NULL) != 0 ) {
            fprintf(stderr, "Failed to join thread\n");
            exit(1);
        }
    }

    //free(filenames);

    exit(0);
}