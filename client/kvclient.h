//
// Created by wenshen on 25-4-30.
//

#ifndef KVCLIENT_H
#define KVCLIENT_H

#endif //KVCLIENT_H
#include "../common.h"
#include "../protocol/slenprotocol.h"

#include <fcntl.h>

/**
 *  64------1
 *  bit1: 0长链接模式, 1短链接模式
 *  … support more
 */
typedef long KVClientOptions;

typedef struct socket_options {
    int level;
    int optname;
    const void *optval;
    socklen_t optlen;
} SocketOptions;

/**
 * kv_client(阻塞模式)
 * request -> blocking -> response(buffer)
 *
 * 两种链接模式 :
 * 短链接 -> 回复之后直接销毁 client
 * 长链接 -> 回复之后 检查timeout 符合时间销毁(支持 -1)
 *
 * buffer 默认4096(4KB)
 * 如果buffer 容量不够 扩容buffer(但不会收缩)
 *
 */
typedef struct kv_client {
    int connect_fd;
    volatile int status;
    struct sockaddr_in server;
    SocketOptions **socket_options;
    size_t sops_size;

    int timeout;
    int connect_mode;

    int reconnect_attempts;
    int reconnect_interval;
    long last_activity;
    long long requests_sent;
    long long responses_received;

    long connect_time;
    char *buffer;
    size_t offset;
    size_t buffer_size;
} KVClient;


KVClient *kv_client_create(const char *host, int port, int timeout, int connect_mode);

void kv_client_destroy(KVClient **client);


// int set(KVClient *client, const char *key, const char *value);

// void get(KVClient *client, const char *key, size_t *r_len, char ** r);
