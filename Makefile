#
# To compile, type "make" or make "all"
# To remove files, type "make clean"
#
OBJS = server.o request.o segel.o client.o thread_safe_queue.o threadpool.o
TARGET = server

CC = gcc
CFLAGS = -g -Wall

LIBS = -lpthread 

.SUFFIXES: .c .o 

all: server client output.cgi
	-mkdir -p public
	-cp output.cgi favicon.ico home.html public

server: server.o request.o segel.o thread_safe_queue.o threadpool.o
	$(CC) $(CFLAGS) -o server server.o request.o segel.o thread_safe_queue.o threadpool.o  $(LIBS)

client: client.o segel.o
	$(CC) $(CFLAGS) -o client client.o segel.o

output.cgi: output.c
	$(CC) $(CFLAGS) -o output.cgi output.c

threadpool.o: threadpool.c
	$(CC) $(CFLAGS) -o $@ -c $<

thread_safe_queue.o: thread_safe_queue.c
	$(CC) $(CFLAGS) -o $@ -c $<

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	-rm -f $(OBJS) server client output.cgi
	-rm -rf public
