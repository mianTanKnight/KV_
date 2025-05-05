//
// Created by wenshen on 25-4-25.
//

#ifndef BUFFERS__H
#define BUFFERS__H
#include "../common.h"
#include "../server/server_.h"

/**
 *  线性数据的组织
 *  e1 -> e2 -> e3->....
 *  len: 总长度
 *  offset: 总消费游标
 *
 *  cursors_event: 游标事件
 *  cursors_event_offset: 游标事件的有效offset
 *
 *  buffer 会被重复受检,当数据准备就绪(具体的解析) 会同步所有游标
 */
typedef struct fd_buffer {
    NetEvent *head;
    NetEvent *tail;
    size_t event_list_size;
    size_t offset;
    size_t len;

    /**
     * cache support
     */
    NetEvent *cursors_event;
    size_t cursors_event_offset;
    int ccl;   // current_consumer_len  init -> 0
    int cccsl; // current_consumer_constant_str_len init -> 0
    int ccsl;  // current_consumer_str_len  init -> 0

} FdBuffer;


int init_fd_buffers();

int binding_(int fd);

FdBuffer *get_(int fd);

FdBuffer *discharge(int fd);

void destroy_buffers();

#endif //BUFFERS__H
