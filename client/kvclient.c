//
// Created by wenshen on 25-4-30.
//

#include "kvclient.h"

#include <sys/resource.h>

static int
connect_to_server(KVClient *client);

KVClient *kv_client_create(const char *host, int port, int timeout, int connect_mode) {
    if (!host || port <= 0) {
        errno = EINVAL;
        return NULL;
    }
    KVClient *client = calloc(1, sizeof(KVClient));
    if (!client) {
        return NULL;
    }
    client->server.sin_family = AF_INET;
    client->server.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &client->server.sin_addr) <= 0) {
        free(client);
        return NULL;
    }
    client->connect_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->connect_fd < 0) {
        free(client);
        return NULL;
    }
    client->timeout = timeout;
    client->connect_mode = connect_mode;
    client->status = 0; // 初始状态
    client->reconnect_attempts = 3; // 默认重试3次
    client->reconnect_interval = 1000; // 默认1秒间隔
    client->requests_sent = 0;
    client->responses_received = 0;
    client->connect_time = 0;
    client->last_activity = 0;
    client->buffer_size = 4096; // 默认4KB
    client->offset = 0;
    client->buffer = (char *) malloc(client->buffer_size);
    if (!client->buffer) {
        close(client->connect_fd);
        free(client);
        return NULL;
    }
    if (connect_to_server(client) != 0) {
        kv_client_destroy(&client);
        return NULL;
    }
    client->connect_time = time(NULL);
    client->last_activity = client->connect_time;
    return client;
}


void kv_client_destroy(KVClient **client) {
    if (!client)
        return;
    if (!*client)
        return;
    KVClient *client_ = *client;
    if (client_->connect_fd >= 0) {
        close(client_->connect_fd);
        client_->connect_fd = -1;
    }
    if (client_->buffer) {
        free(client_->buffer);
        client_->buffer = NULL;
    }
    for (size_t i = 0; i < client_->sops_size; i++) {
        if (client_->socket_options[i]) {
            if (client_->socket_options[i]->optval) {
                free((void *) client_->socket_options[i]->optval);
            }
            free(client_->socket_options[i]);
        }
    }
    free(*client);
    *client = NULL;
}

static int connect_to_server(KVClient *client) {
    if (client->connect_fd < 0) {
        return -1;
    }

    if (client->timeout > 0) {
        struct timeval tv;
        tv.tv_sec = client->timeout / 1000;
        tv.tv_usec = (client->timeout % 1000) * 1000;

        if (setsockopt(client->connect_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ||
            setsockopt(client->connect_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
            return -1;
        }
    }

    for (size_t i = 0; i < client->sops_size; i++) {
        SocketOptions *opt = client->socket_options[i];
        if (opt && setsockopt(client->connect_fd, opt->level, opt->optname,
                              opt->optval, opt->optlen) < 0) {
            return -1;
        }
    }

    if (connect(client->connect_fd, (struct sockaddr *) &client->server,
                sizeof(client->server)) < 0) {
        for (int i = 0; i < client->reconnect_attempts; i++) {
            usleep(client->reconnect_interval * 1000);
            if (connect(client->connect_fd, (struct sockaddr *) &client->server,
                        sizeof(client->server)) == 0) {
                client->status = 1;
                return 0;
            }
        }
        return -1;
    }

    client->status = 1;
    return 0;
}

int kv_client_add_socket_option(KVClient *client, int level, int optname,
                                const void *optval, socklen_t optlen) {
    if (!client || !optval || optlen <= 0) {
        return -1;
    }

    SocketOptions *option = malloc(sizeof(SocketOptions));
    if (!option) {
        return -1;
    }

    void *value_copy = malloc(optlen);
    if (!value_copy) {
        free(option);
        return -1;
    }
    memcpy(value_copy, optval, optlen);

    option->level = level;
    option->optname = optname;
    option->optval = value_copy;
    option->optlen = optlen;

    size_t new_size = client->sops_size + 1;
    SocketOptions **new_options = realloc(
        client->socket_options, new_size * sizeof(SocketOptions *));

    if (!new_options) {
        free(value_copy);
        free(option);
        return -1;
    }

    client->socket_options = new_options;
    client->socket_options[client->sops_size] = option;
    client->sops_size = new_size;

    if (client->status == 1 && client->connect_fd >= 0) {
        if (setsockopt(client->connect_fd, level, optname, value_copy, optlen) < 0) {
            return -1;
        }
    }
    return 0;
}


int set(KVClient *client, const char *key, const size_t key_len, const char *value, const size_t value_len) {
}


