//
// Created by wenshen on 25-4-30.
//

#include "kvclient.h"

#include <sys/resource.h>

#include "../commands/command_.h"

static int
connect_to_server(KVClient *client);

static int
slenpro_client(KVClient *client,
               size_t command_i,
               char **datas,
               size_t *data_lens,
               size_t data_len);


KVClient
*kv_client_create(const char *host, int port, int timeout, int connect_mode) {
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


void
kv_client_destroy(KVClient **client) {
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

static
int connect_to_server(KVClient *client) {
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

int
kv_client_add_socket_option(KVClient *client, int level, int optname,
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

int
kv_set(KVClient *client, char *key, size_t key_len, char *value, size_t value_len) {
    char *datas[2];
    size_t data_lens[2];
    datas[0] = key;
    datas[1] = value;
    data_lens[0] = key_len;
    data_lens[1] = value_len;
    return slenpro_client(client, 0, datas, data_lens, 2);
}

int
kv_get(KVClient *client, char *key, size_t key_len) {
    char *datas[1];
    size_t data_lens[1];
    datas[0] = key;
    data_lens[0] = key_len;
    return slenpro_client(client, 1, datas, data_lens, 1);
}

int
kv_del(KVClient *client, char *key, size_t key_len) {
    char *datas[1];
    size_t data_lens[1];
    datas[0] = key;
    data_lens[0] = key_len;
    return slenpro_client(client, 2, datas, data_lens, 1);
}

int
kv_expire(KVClient *client, char *key, size_t key_len, char *value, size_t value_len) {
    char *datas[2];
    size_t data_lens[2];
    datas[0] = key;
    datas[1] = value;
    data_lens[0] = key_len;
    data_lens[1] = value_len;
    return slenpro_client(client, 3, datas, data_lens, 2);
}

int
slenpro_client(KVClient *client,
               size_t command_i,
               char **datas,
               size_t *data_lens,
               size_t data_len) {
    if (!client)
        return -1;

    char setv_str_[32]; // max of stack
    int digits = 0;
    if (command_i == 0) {
        size_t vlen = data_lens[1];
        int i = 0;
        if (vlen == 0) {
            setv_str_[0] = '0';
            setv_str_[1] = '\0';
        } else {
            size_t temp = vlen;
            while (temp != 0) {
                temp /= 10;
                digits++;
            }
            temp = vlen;
            for (i = digits - 1; i >= 0; i--) {
                setv_str_[i] = '0' + temp % 10;
                temp /= 10;
            }
            setv_str_[digits++] = ':';
            setv_str_[digits] = '\0';
        }
    }


    Command command = commands[command_i];
    size_t size = command.name_len + 1; // SET'' 1-> ' '
    for (int i = 0; i < data_len; i++) {
        size += data_lens[i];
        if (i != data_len - 1) {
            size++; // add ' '
        }
    }
    if (digits > 0)
        size += digits;

    size_t original_data_len = size;
    int len_str_len = 1;
    while (size /= 10)
        len_str_len++;

    size_t required_size = len_str_len + original_data_len + 1;
    size_t nbuffercap = client->buffer_size;

    while (nbuffercap < required_size)
        nbuffercap *= 2;

    if (nbuffercap != client->buffer_size) {
        char *nbuffer = calloc(1, nbuffercap * sizeof(char));
        if (!nbuffer) {
            LOG_ERROR("calloc fail %s", strerror(errno));
            return -1;
        }
        free(client->buffer);
        client->buffer = nbuffer;
        client->buffer_size = nbuffercap;
    }

    char *bcopyp = client->buffer + len_str_len + 1;
    //copy to buffer
    memcpy(bcopyp, command.name, command.name_len);
    bcopyp += command.name_len;
    memcpy(bcopyp++, " ", 1);

    for (int i = 0; i < data_len; i++) {
        if (digits > 0 && i == 1) {
            memcpy(bcopyp, setv_str_, digits);
            bcopyp += digits;
        }
        memcpy(bcopyp, datas[i], data_lens[i]);
        bcopyp += data_lens[i];
        if (i != original_data_len - 1) {
            *bcopyp++ = ' '; // like -> set k1 v1
        }
    }

    size_t wlen = slenpro(client->buffer, original_data_len, len_str_len);
    if (wlen != len_str_len + 1) {
        LOG_ERROR("slenpro fail %s", strerror(errno));
        return -1;
    }

    client->offset = required_size;
    return (int) required_size;
}


int kv_send(KVClient *client) {
    if (!client)
        return -1;

    if (client->status < 0) {
        LOG_ERROR("Client status exception %d", client->status);
        return -1;
    }

    size_t remain = client->offset;
    size_t roffset = 0;
    while (remain) {
        size_t writerr = write(client->connect_fd, client->buffer + roffset, remain);
        remain -= writerr;
        roffset += writerr;
    }

    client->offset = 0;

    size_t prolen = -1;
    size_t prolen_str_len = 0;
    while (1) {
        ssize_t readt = read(client->connect_fd, client->buffer + client->offset, client->buffer_size - client->offset);
        client->offset += readt;
        if (readt > 0) {
            if (prolen == -1) {
                prolen = 0;
                for (size_t i = 0; i < client->buffer_size; i++) {
                    char c = client->buffer[i];
                    if (c >= '0' && c <= '9') {
                        prolen = prolen * 10 + (c - '0');
                        prolen_str_len++;
                        continue;
                    }
                    if (':' == c) {
                        break;
                    }
                    LOG_ERROR("protocol exception");
                    return -1;
                }
                prolen += prolen_str_len + 1;
            }
            if (client->offset == client->buffer_size) {
                client->buffer_size <<= 1;
                char *nbuffer = calloc(client->buffer_size, sizeof(char));
                if (!nbuffer) {
                    LOG_ERROR("calloc fail %s", strerror(errno));
                    return -1;
                }
                memcpy(nbuffer, client->buffer, client->offset); //copy to new_buffer
                free(client->buffer);
                client->buffer = nbuffer;
            }
            if (client->offset >= prolen) break;
        }  else if (readt < 0) {
            LOG_ERROR("Client read fail %s", strerror(errno));
            return -1;
        } else {
            break;
        }
    }
    client->read_offset = prolen_str_len + 1;
    return (int) client->offset;
}
