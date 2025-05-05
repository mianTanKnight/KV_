//
// Created by wenshen on 25-4-20.
//
// 基于 epoll 驱动

#ifndef SERVER__H
#define SERVER__H

#include "../common.h"

#define MAX_EVENT_COUNT 512

typedef struct net_event NetEvent;

typedef struct net_event {
    int type; // 连接,读,写(通常不会涉及到写)
    int fd;
    NetEvent *next;
    int size;
    char data[]; //柔性数组
} NetEvent;

typedef struct net_server_context {
    int socket_fd;
    int epoll_fd; // epoll
    // epoll 并不会阻塞main线程 而是阻塞此线程 也就是说整个事件驱动都是单线程作业
    // 但event 并不会处理业务 而是把数据提交给 task队列(这里会使用锁 但会非常小)
    // 真正的业务处理依然会由main 线程完成
    pthread_t event_thread;
    //queue_lock
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;

    NetEvent *head;
    NetEvent *tail;
    volatile size_t event_count;

    volatile int status;
    size_t max_connection_count;
    size_t current_connection_count;

    size_t backlog;
} NetServerContext;


NetServerContext *create_net_server_context(const size_t port, size_t backlog, size_t max_connection_count);

size_t getEvents(NetServerContext *context, NetEvent **result);

void
destroy_event(NetEvent **event);

void destroy_net_server_context(NetServerContext *context);



#endif //SERVER__H
