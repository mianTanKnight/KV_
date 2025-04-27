//
// Created by wenshen on 25-4-25.
//

#ifndef BUFFERS__H
#define BUFFERS__H
#include "../common.h"
#include "../server/server_.h"
#include <sys/resource.h>
/**
 * fd_buffer_: 以文件描述符(fd)为索引的直接访问缓冲区数组
 *
 * 核心优势:
 * - O(1)访问: fd直接作为数组索引，无哈希计算开销
 * - 明确的生命周期: buffers[fd]随fd创建和销毁，避免重复释放
 * - 简化内存管理: 释放后置NULL (buffers[fd] = NULL)防止悬挂指针
 *
 * 设计限制:
 * - 数组大小受系统fd限制，需预分配
 * - 权衡取舍: 空间换时间，获得更高性能和更简单的内存管理
 */

typedef struct fd_buffer {
    // buffer 是线性的 虽然是以event段为单位,但逻辑上我们需要线性处理
    // offset,len 并不会受除main线程之外的线程修改
    NetEvent *head;
    NetEvent *tail;
    size_t event_list_size;
    size_t offset;
    size_t len;

    // cache -> 一种缓存机制 当前的消费信息 被解析出来之后会被缓存下来 len:
    int ccl; // current_consumer_len  init -> 0
    int cccsl; // current_consumer_constant_str_len init -> 0
    int ccsl; // current_consumer_str_len  init -> 0
} FdBuffer;


int init_fd_buffers();
int binding_(int fd);
FdBuffer *get_(int fd);
FdBuffer *discharge(int fd);

#endif //BUFFERS__H
