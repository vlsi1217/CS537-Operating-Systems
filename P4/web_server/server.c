#include "server.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

void initQueue(volatile struct queue* q, int buffers)
{
    q->size = 1;
    q->bufSz = buffers + 1;
    q->connPool = (struct data*) malloc(sizeof(struct data) * (buffers + 1));
    struct data dummy = {-1, -1};
    q->connPool[0] = dummy;
}

int isEmpty(volatile struct queue* q)
{
    return (q->size == 1); 
}

int isFull(volatile struct queue* q)
{
    return (q->bufSz == q->size);
}

void enqueue(volatile struct queue* q, struct data d)
{
    //printf("before enqueue:\n");
    //printQueue(q);
    q->connPool[q->size] = d;
    int index = q->size++;
    while(index / 2 != 0)
    {
        if(q->connPool[index].priority < q->connPool[index / 2].priority)
        {
            struct data temp = q->connPool[index];
            q->connPool[index] = q->connPool[index / 2];
            q->connPool[index / 2] = temp;
        }
        else
        {
            break;
        }
        index /= 2;
    }
}

struct data dequeue(volatile struct queue* q)
{
    struct data ret = q->connPool[1];
    q->connPool[1] = q->connPool[(--q->size)];
    int index = 1;
    while(index * 2 < q->size)
    {
        if(index * 2 + 1 < q->size && (q->connPool[index].priority > q->connPool[2 * index].priority || q->connPool[index].priority > q->connPool[2 * index + 1].priority))
        {
            int minIndex = -1;
            if(q->connPool[index * 2].priority <= q->connPool[index * 2 + 1].priority)
            {
                minIndex = index * 2;
            }
            else
            {
                minIndex = index * 2 + 1;
            }
            struct data temp = q->connPool[index];
            q->connPool[index] = q->connPool[minIndex];
            q->connPool[minIndex] = temp;
            index = minIndex;
        }
        else if (index * 2 + 1 >= q->size && q->connPool[index].priority > q->connPool[2 * index].priority)
        {
            struct data temp = q->connPool[index];
            q->connPool[index] = q->connPool[2 * index];
            q->connPool[2 * index] = temp;
            index *= 2;
        }
        else
        {
            break;
        }
    }
    return ret;
}

void printQueue(volatile struct queue* q)
{
    int i;
    printf("q size: %d buffer size: %d\n", q->size, q->bufSz);
    for(i = 0; i < q->size; i++)
    {
        printf("priority: %d connfd: %d buffer: %s - ", q->connPool[i].priority, q->connPool[i].connfd, q->connPool[i].buff);
    }
    printf("\n");
}

void* serverWorker(void *arg)
{
    while(1)
    {
        Pthread_mutex_lock(&cLock);
        while(isEmpty(&q))
        {
            Pthread_cond_wait(&empty, &cLock);
        }
        struct data d = dequeue(&q);
        Pthread_cond_signal(&full);
        Pthread_mutex_unlock(&cLock);
        requestHandleWorker(&d);
        Close(d.connfd);
    }
    return NULL;
}

void getRequestFileName(struct data* d, char* filename, char* cgiargs, int connfd)
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t rio;
    Rio_readinitb(&rio, connfd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    requestParseURI(uri, filename, cgiargs);
    strcpy(d->buff, buf);
    requestReadhdrs(&rio);
}

void requestHandleWorker(struct data* d)
{
   int is_static;
   struct stat sbuf;
   char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
   char filename[MAXLINE], cgiargs[MAXLINE];
   int fd = d->connfd;
   sscanf(d->buff, "%s %s %s", method, uri, version);

   printf("%s %s %s\n", method, uri, version);
   if (strcasecmp(method, "GET")) {
      requestError(fd, method, "501", "Not Implemented", "CS537 Server does not implement this method");
      return;
   }
   is_static = requestParseURI(uri, filename, cgiargs);
   if (stat(filename, &sbuf) < 0) {
      requestError(fd, filename, "404", "Not found", "CS537 Server could not find this file");
      return;
   }

   if (is_static) {
      if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
         requestError(fd, filename, "403", "Forbidden", "CS537 Server could not read this file");
         return;
      }
      requestServeStatic(fd, filename, sbuf.st_size);
   } else {
      if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
         requestError(fd, filename, "403", "Forbidden", "CS537 Server could not run this CGI program");
         return;
      }
      requestServeDynamic(fd, filename, cgiargs);
   }
}

void getargs(int* stats,int argc, char *argv[])
{
    if (argc != 5) {
	fprintf(stderr, "Usage: %s [portnum] [threads] [buffers] [schedalg]\n", argv[0]);
	exit(1);
    }
    stats[0] = atoi(argv[1]);		//port number
    stats[1] = atoi(argv[2]);		//number of threads
    stats[2] = atoi(argv[3]);		//number of buffers
    if(strcmp(argv[4], "FIFO") == 0)
    {
    	stats[3] = FIFO;				//0 is FIFO
    }
    else if(strcmp(argv[4], "SFNF") == 0)
    {
    	stats[3] = SFNF;
    }
    else if(strcmp(argv[4], "SFF") == 0)
    {
    	stats[3] = SFF;
    }
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, clientlen;
    int stats[4];
    struct sockaddr_in clientaddr;
    getargs(stats, argc, argv);
    int port = stats[0], threads = stats[1], buffers = stats[2], policy = stats[3];
    //initialize the priority queue
    initQueue(&q, buffers);
    //create a pool of threads
    workers = (pthread_t*) malloc(sizeof(pthread_t) * threads);
    int i;
    for(i = 0; i < threads; i++)
    {
        Pthread_create(&workers[i], NULL, serverWorker, NULL); 
    }
    listenfd = Open_listenfd(port);
    while (1) {
	    clientlen = sizeof(clientaddr);
	    connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        Pthread_mutex_lock(&cLock);
        while(isFull(&q))
        {
            Pthread_cond_wait(&full, &cLock);
        }
        Pthread_mutex_unlock(&cLock); 
        struct data d;
        d.connfd = connfd;
        char filename[MAXLINE], cgiargs[MAXLINE];
        getRequestFileName(&d, filename, cgiargs, connfd);
        if(policy == FIFO)
        {
            d.priority = count++; 
        }
        else if(policy == SFNF)
        {
            d.priority = strlen(filename); 
        }  
        else if(policy == SFF)
        {
            struct stat fileStat;
            if(stat(filename, &fileStat) < 0)
            {
                exit(0);
            }
            d.priority = fileStat.st_size; 
        }
        
        Pthread_mutex_lock(&cLock);
        enqueue(&q, d);
	    Pthread_cond_signal(&empty);
        Pthread_mutex_unlock(&cLock); 
    }

}