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
    return rlim.rlim_cur > 100000 ? 100000 : rlim.rlim_cur; // MAX
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
    buffer->cursors_event = NULL;
    buffer->cursors_event_offset = 0;
    buffer->len = 0;
    buffer->ccl = 0;
    buffer->ccsl = 0;
    bufffers[fd] = buffer;
    return 0;
}

void destroy_buffers() {
    if (bufffers) {
        free(bufffers);
    }
}

FdBuffer *get_(int fd) {
    return bufffers[fd];
}

/**
 * discharge 不会管理 buffer(free)
 * 它只负责接触绑定 返回 buffer*
 */
FdBuffer *discharge(int fd) {
    if (fd >= maxfdlimit_) {
        fprintf(stderr, "maxfdlimit exceeded\n");
        return NULL;
    }
    FdBuffer *fb = bufffers[fd];
    bufffers[fd] = NULL;
    return fb;
}

/**
 * fd 是不重复的 int
 * 每个有效的fd 都会有绑定一个有效的 buffer
 * 因为 fd 的特性
 * 使用 array + index(fd) 使用完全的 O1 和简洁 高效的buffer生命周期管理
 *
 * 缺点是 预分配
 */
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
