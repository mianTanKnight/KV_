//
// Created by wenshen on 25-4-25.
//

#include "buffers_.h"

FdBuffer **bufffers = NULL;
size_t maxfdlimit_;

size_t get_max_fd_limit() {
    struct rlimit rlim;
    if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
        return 1024;
    }
    return rlim.rlim_cur > 100000 ? 100000 : rlim.rlim_cur; // 返回当前限制
}

int binding_(int fd) {
    if (fd >= maxfdlimit_) {
        fprintf(stderr, "maxfdlimit exceeded\n");
        return -1;
    }
    if (bufffers[fd]) {
        fprintf(stderr, "fd %d is already in use\n", fd);
        return -2;
    }
    FdBuffer *buffer = calloc(1, sizeof(FdBuffer));
    if (!buffer) {
        LOG_ERROR("calloc failed!");
        return -3;
    }
    buffer->head = buffer->tail = NULL;
    buffer->event_list_size = 0;
    buffer->offset = 0;
    buffer->len = 0;
    buffer->ccl = 0;
    buffer->ccsl = 0;
    bufffers[fd] = buffer;
    return 0;
}

FdBuffer *get_(int fd) {
    return bufffers[fd];
}

FdBuffer *discharge(int fd) {
    if (fd >= maxfdlimit_) {
        fprintf(stderr, "maxfdlimit exceeded\n");
        return NULL;
    }
    FdBuffer *fb = bufffers[fd];
    bufffers[fd] = NULL;
    return fb;
}

int init_fd_buffers() {
    size_t maxfdlimit = get_max_fd_limit();
    bufffers = calloc(maxfdlimit, sizeof(FdBuffer *));
    if (!bufffers) {
        LOG_ERROR("Buffers allocation failed %s \n", strerror(errno));
        return -1;
    }
    for (int i = 0; i < maxfdlimit; ++i) {
        bufffers[i] = NULL;
    }
    maxfdlimit_ = maxfdlimit;
    return (int) maxfdlimit;
}

