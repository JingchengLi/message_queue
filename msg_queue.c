#include "msg_queue.h"
struct msg_queue queue = {NULL, 10, 0, 0, 0, 0, 0};

void * produce(void * arg)
{
    pthread_detach(pthread_self());
    int i=0;
    while(1){
        put_queue(&queue, (void*)i++);
    }
}

void *consume(void *arg)
{
    int data;
    while(1){
        data = (int)(get_queue(&queue));
    }
}

int main()
{   
    pthread_t pid;
    int i=0;

    pthread_mutex_init(&mux, 0);
    pthread_cond_init(&cond_get, 0);
    pthread_cond_init(&cond_put, 0);

    queue.buffer = malloc(queue.size * sizeof(void*));
    if(queue.buffer == NULL){
        printf("malloc failed!\n");
        exit(-1);
    }

    pthread_create(&pid, 0, produce, 0);
    pthread_create(&pid, 0, produce, 0);
    pthread_create(&pid, 0, produce, 0);
    pthread_create(&pid, 0, consume, 0);
    pthread_create(&pid, 0, consume, 0);
    pthread_create(&pid, 0, consume, 0);

    sleep(60);

    free(queue.buffer);
    pthread_mutex_destroy(&mux);
    pthread_cond_destroy(&cond_get);
    pthread_cond_destroy(&cond_put);
}

