//
// Created by wenshen on 25-4-20.
//

#include "server_.h"
#include "../common.h"
#include "../protocol/slenprotocol.h"

static int
set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd,F_SETFL, flags | O_NONBLOCK); //非阻塞模式
}

static int
add_to_epoll(int epoll_fd, int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    return epoll_ctl(epoll_fd,EPOLL_CTL_ADD, fd, &ev);
}

static void
enqueue(NetServerContext *context, NetEvent *event) {
    if (!context)
        return;
    pthread_mutex_lock(&context->queue_mutex);
    if (context->event_count == 0) {
        context->head = context->tail = event;
    } else {
        context->tail->next = event;
        context->tail = event;
    }
    context->event_count++;
    pthread_cond_signal(&context->queue_cond); // 唤醒可能的被阻塞的线程
    pthread_mutex_unlock(&context->queue_mutex);
}

size_t
getEvents(NetServerContext *context, NetEvent **result) {
    if (context->event_count == 0) // event_count 是由 volatile修饰的 支持无锁 快速检测是否是empty queue
        return 0;
    pthread_mutex_lock(&context->queue_mutex);
    *result = context->head;
    size_t evens = context->event_count;
    context->head = context->tail = NULL;
    context->event_count = 0;
    pthread_mutex_unlock(&context->queue_mutex);
    return evens;
}

void
destroy_event(NetEvent *event) {
    if (event) {
        free(event);
        event = NULL;
    }
}

void
destroy_net_server_context(NetServerContext *context) {
    if (context) {
        close(context->socket_fd);
        close(context->epoll_fd);
        pthread_mutex_destroy(&context->queue_mutex);
        pthread_cond_destroy(&context->queue_cond);
        free(context);
        context = NULL;
    }
}


void
*event_handler(void *args) {
    NetServerContext *net_context = args;
    const int server_fd = net_context->socket_fd;

    struct epoll_event events[MAX_EVENT_COUNT];
    int nfds;

    char buffer[4096]; // stack -> 4kb

    while (1) {
        nfds = epoll_wait(net_context->epoll_fd, events,MAX_EVENT_COUNT, -1);
        if (nfds == -1) {
            if (errno == EINTR) {
                continue;
            }
            LOG_ERROR("EPOLL_WAIT_ERROR %s \n", strerror(errno));
            net_context->status = -1; // 致死
            break;
        }

        for (int i = 0; i < nfds; i++) {
            int current_fd = events[i].data.fd;
            if (server_fd == current_fd) {
                // accept event
                struct sockaddr_in client_;
                socklen_t client_len = sizeof(client_);
                int connect_fd = accept(server_fd, (struct sockaddr *) &client_, &client_len);
                LOG_INFO("Accepted connection from %s", inet_ntoa(client_.sin_addr));
                if (add_to_epoll(net_context->epoll_fd, connect_fd, EPOLLIN) < 0) {
                    LOG_ERROR("Add to epoll failure %s", inet_ntoa(client_.sin_addr));
                    close(connect_fd);
                }
            } else {
                if (events[i].events & EPOLLIN) {
                    ssize_t t = read(current_fd, buffer, 4096);

                    if (t > 0) {
                        NetEvent *event = calloc(1, sizeof(NetEvent) + t * sizeof(char));
                        event->size = (int) t;
                        event->fd = current_fd;
                        event->next = NULL;
                        event->type = 1;
                        memcpy(event->data, buffer, t);
                        enqueue(net_context, event);
                    } else if (t == 0) {
                        LOG_INFO("Connection closed by client: fd=%d", current_fd);
                        epoll_ctl(net_context->epoll_fd, EPOLL_CTL_DEL, current_fd,NULL);
                        NetEvent *event = calloc(1, sizeof(NetEvent));
                        event->size = (int) t;
                        event->fd = current_fd;
                        event->next = NULL;
                        event->type = 2;
                        enqueue(net_context, event);
                        close(current_fd);
                    } else {
                        // 读取错误
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // 暂时没有更多数据，继续等待
                            LOG_INFO("No more data available now, will wait for more");
                        } else {
                            // 其他错误
                            LOG_ERROR("Read error on fd=%d: %s", current_fd, strerror(errno));
                            epoll_ctl(net_context->epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
                            close(current_fd);
                        }
                    }
                }
            }
        }
    }
    return NULL;
}


NetServerContext
*create_net_server_context(const size_t port, size_t backlog, size_t max_connection_count) {
    NetServerContext *init = calloc(1, sizeof(NetServerContext));
    if (!init) {
        LOG_ERROR("Failed to allocate memory for NetServerContext %s\n", strerror(errno));
        return NULL;
    }

    init->status = 0;

    // create lock,cont,event_thread
    if (pthread_mutex_init(&init->queue_mutex, NULL)) {
        LOG_ERROR("Failed to initialize mutex for NetServerContext %s\n", strerror(errno));
        pthread_mutex_destroy(&init->queue_mutex);
        free(init);
        return NULL;
    }
    if (pthread_cond_init(&init->queue_cond, NULL)) {
        LOG_ERROR("Failed to initialize condition variable for NetServerContext %s\n", strerror(errno));
        pthread_cond_destroy(&init->queue_cond);
        free(init);
        return NULL;
    }

    // init queue
    init->head = init->tail = NULL;
    init->event_count = 0;

    // create&init server_socket
    struct sockaddr_in server_;
    int socket_fd;
    if ((socket_fd = socket(AF_INET,SOCK_STREAM, 0)) == -1) {
        LOG_ERROR("Failed to create socket %s", strerror(errno));
        pthread_mutex_destroy(&init->queue_mutex);
        pthread_cond_destroy(&init->queue_cond);
        free(init);
        return NULL;
    }

    int opt = 1;
    if (setsockopt(socket_fd,SOL_SOCKET,SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_ERROR("Failed to set socket options: %s", strerror(errno));
        close(socket_fd);
        pthread_mutex_destroy(&init->queue_mutex);
        pthread_cond_destroy(&init->queue_cond);
        free(init);
        return NULL;
    }

    if (set_nonblocking(socket_fd) < 0) {
        LOG_ERROR("Failed to set socket non-blocking: %s", strerror(errno));
        close(socket_fd);
        pthread_mutex_destroy(&init->queue_mutex);
        pthread_cond_destroy(&init->queue_cond);
        free(init);
        return NULL;
    }

    memset(&server_, 0, sizeof(struct sockaddr_in)); // to zero
    server_.sin_family = AF_INET;
    server_.sin_addr.s_addr = htonl(INADDR_ANY);
    server_.sin_port = htons(port);

    if (bind(socket_fd, (struct sockaddr *) &server_, sizeof(server_)) < 0) {
        LOG_ERROR("Failed to bind socket %s", strerror(errno));
        close(socket_fd);
        pthread_mutex_destroy(&init->queue_mutex);
        pthread_cond_destroy(&init->queue_cond);
        free(init);
        return NULL;
    }

    if (listen(socket_fd, (int) backlog) < 0) {
        LOG_ERROR("Failed to listen socket %s", strerror(errno));
        close(socket_fd);
        pthread_mutex_destroy(&init->queue_mutex);
        pthread_cond_destroy(&init->queue_cond);
        free(init);
        return NULL;
    }

    init->socket_fd = socket_fd;

    // create&init epoll
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        LOG_ERROR("Failed to create epoll instance: %s", strerror(errno));
        close(socket_fd);
        pthread_mutex_destroy(&init->queue_mutex);
        pthread_cond_destroy(&init->queue_cond);
        free(init);
        return NULL;
    }

    init->epoll_fd = epoll_fd;

    // 将监听套接字添加到epoll
    if (add_to_epoll(epoll_fd, socket_fd, EPOLLIN) < 0) {
        LOG_ERROR("Failed to add socket to epoll: %s", strerror(errno));
        close(epoll_fd);
        close(socket_fd);
        pthread_mutex_destroy(&init->queue_mutex);
        pthread_cond_destroy(&init->queue_cond);
        free(init);
        return NULL;
    }

    // 创建事件处理线程
    if (pthread_create(&init->event_thread, NULL, event_handler, init) != 0) {
        LOG_ERROR("Failed to create event thread: %s", strerror(errno));
        close(epoll_fd);
        close(socket_fd);
        pthread_mutex_destroy(&init->queue_mutex);
        pthread_cond_destroy(&init->queue_cond);
        free(init);
        return NULL;
    }

    init->backlog = backlog;
    init->max_connection_count = max_connection_count;
    init->current_connection_count = 0;
    init->status = 1; // have prepared

    return init;
}
