#include "cs537.h"
#include "request.h"
#include "threads.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

const int FIFO = 0;
const int SFNF = 1;
const int SFF = 2;

pthread_t* workers = NULL;

struct data
{
	int priority;
	int connfd;
	//rio_t rio;
	char buff[MAXLINE];
};

struct queue{
	int size;
	int bufSz;
	struct data* connPool;
};

volatile int count = 0;
volatile struct queue q;

pthread_mutex_t cLock = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

/* method for priority queue */
void enqueue(volatile struct queue* q, struct data d);
struct data dequeue(volatile struct queue* q);
void initQueue(volatile struct queue* q, int buffer);
int isEmpty(volatile struct queue* q);
int isFull(volatile struct queue* q);
void printQueue(volatile struct queue* q);

void getargs(int* stats,int argc, char *argv[]);
void* serverWorker(void *arg);
void getRequestFileName(struct data* d, char* filename, char* cgiargs, int connfd);
void requestHandleWorker(struct data* d);
