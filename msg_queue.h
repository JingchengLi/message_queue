#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t mux;
pthread_cond_t cond_get, cond_put;

struct msg_queue {
    void** buffer; // 缓冲数据, .buffer = msg
    int size; // 队列大小，使用的时候给出稍大的size，可以减少进入内核态的操作
    int lget; // 取队列数据的偏移量
    int lput; // 放队列数据的偏移量
    int nData; // 队列中数据的个数,用来判断队列满/空
    int nFullThread; // 由于队列满而阻塞在put_queue的线程个数
    int nEmptyThread; // 由于队列空而阻塞在get_queue的线程个数
};

void* get_queue(struct msg_queue *q){
    void* data = NULL;
    pthread_mutex_lock(&mux);
    while(q->lget == q->lput && 0 == q->nData){
        // 此处循环判断的原因是：假设2个消费者线程在get_queue阻塞，然后两者都被激活，
        // 而其中一个线程运行比较块，快速消耗了2个数据，另一个线程醒来的时候已
        // 经没有新数据可以消耗了。这种情况是有可能的：比如，其它生产者线程快速
        // 调用put_queue两次，如果有2个线程在get_queue处阻塞，就会被同时激活，
        // 而完全有可能，其中一个被激活的线程获取到了cpu，快速处理了2个消息。

        // 对于循环队列，如果lget与lput相等，那么只有两种情况，
        // 1：nData不为0，队列满
        // 2：nData为0，队列空
        q->nEmptyThread++;
        pthread_cond_wait(&cond_get, &mux);
        q->nEmptyThread--;
    }
#ifdef DEBUG
    printf("get data! lget:%d", q->lget);
#endif
    data = (q->buffer)[q->lget++];
    if(q->lget == q->size){
        // queue用作循环队列
        q->lget = 0;
    }
    q->nData--;
#ifdef DEBUG
    printf(" nData:%d\n", q->nData);
#endif
    if(q->nFullThread){
        // 仅在必要时才调用pthread_cond_signal, 尽量少陷入内核态
        pthread_cond_signal(&cond_put);
    }
    pthread_mutex_unlock(&mux);
    return data;
}

void put_queue(struct msg_queue *q, void* data){
    pthread_mutex_lock(&mux);
    while(q->lget == q->lput && q->nData){
        q->nFullThread++;
        pthread_cond_wait(&cond_put, &mux);
        q->nFullThread--;
    }
#ifdef DEBUG
    printf("put data! lput:%d", q->lput);
#endif
    (q->buffer)[q->lput++] = data;
    if(q->lput == q->size){
        q->lput = 0;
    }
    q->nData++;
#ifdef DEBUG
    printf(" nData:%d\n", q->nData);
#endif
    if(q->nEmptyThread){
        pthread_cond_signal(&cond_get);
    }
    pthread_mutex_unlock(&mux);
}

