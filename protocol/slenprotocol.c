//
// Created by wenshen on 25-4-21.
//
#include "slenprotocol.h"

#include <wctype.h>

#include "constant_.h"

size_t
slenpro(const char *data, size_t data_len, char **pro_str) {
    if (data_len > MAX_DATA_LEN) {
        LOG_ERROR("too much data!");
        return -1;
    }
    int written;
    char *spro;
    char *num_str = getstrconstant(data_len);
    if (!num_str) {
        int prefix_len = snprintf(NULL, 0, "%zu:", data_len);
        if (prefix_len < 0) {
            LOG_ERROR("snprintf len failed!");
            return -1;
        }
        spro = calloc(1, prefix_len + data_len + 2);
        if (!spro) {
            LOG_ERROR("calloc failed!");
            return -1;
        }
        written = snprintf(spro, prefix_len + data_len + 2, "%zu:%s", data_len, data);
    } else {
        spro = calloc(1, CHUNK_SIZE + data_len + 2);
        written = snprintf(spro, CHUNK_SIZE + data_len + 2, "%s:%s", num_str, data);
    }
    if (written < 0) {
        LOG_ERROR("snprintf failed!");
        free(spro);
        return -1;
    }
    *pro_str = spro;
    return written;
}


int
neatenbags(NetEvent *curr_event, FdBuffer *buffer) {
    if (!curr_event) {
        fprintf(stderr, "event must not NULL");
        return -1;
    }
    if (!buffer) {
        fprintf(stderr, "buffer must not NULL");
        return -1;
    }
    if (!buffer->head) {
        buffer->head = buffer->tail = curr_event;
    } else {
        buffer->tail->next = curr_event;
        buffer->tail = curr_event;
    }
    curr_event->next = NULL;
    buffer->event_list_size++;
    buffer->len += curr_event->size;
    return 0;
}


/**
 *
 * 处理具体的拆包和粘包
 * buffer 中的 events 是 线性的 这由网络的协议保证
 * len 就是当前 整个线性长度
 * offset 指消费指针 为什么要写offset
 * 如果没有 offset 每消费一批(完整的) 都需要删除或清理
 * 但如果有 offset 就可以跳过已经消费完的(当恰当的时机清理全部 例如close)
 *
 * paserfdbags 是通信中极高频 fn
 * 它的性能决定 吞吐的上下限
 *
 * event1 -> event2 -> event3 -> event4.....
 * 1: 线性
 * 2: 一次完整的请求可能分散多个event
 *
 * zero_copy
 * 去掉所有的 memcpy
 * 返回 startEvent, eventOffset, len
 * 因为 event 是一个链表所以 startEvent是头节点
 * 而 eventOffset是 从event的有效开始点
 *
 */
NetEvent *
paserfdbags_zero_copy(FdBuffer *bags, size_t *eventOffset, size_t *len, size_t *len_str_l) {
    // offset 和 len 都没有0开始的概念 它不是index(C标准 index 从 0 开始) 它是 size
    if (bags->len == 0 || bags->offset == bags->len) {
        return NULL;
    }
    NetEvent *start_event = bags->cursors_event;
    if (!start_event) {
        start_event = bags->head;
    }
    size_t event_offset = bags->cursors_event_offset;
    int len_num = bags->ccl;
    int len_constant_str_len = bags->cccsl;
    char *start_data = start_event->data + event_offset;

    if (!len_num) {
        NetEvent *start_c = start_event;
        size_t event_len_c = event_offset;
        char c = 0;
        while (start_c) {
            int f_count = start_c->size - event_len_c;
            for (int i = 0; i < f_count; ++i) {
                c = start_data[i];
                if (c >= '0' && c <= '9') {
                    len_num = len_num * 10 + (c - '0');
                    len_constant_str_len++;
                    continue;
                }
                if (c != ':') {
                    fprintf(stderr, "Invalid format!\n");
                    return NULL; // error
                }
                goto next_;
            }
            start_c = start_c->next;
            if (start_c) {
                start_data = start_c->data;
                event_len_c = 0;
            }
        }
    next_:
        bags->ccl = len_num;
        bags->cccsl = len_constant_str_len;
    }

    if (len_num && len_num + len_constant_str_len + 1 <= bags->len) {
        *eventOffset = event_offset;
        *len = len_num;
        *len_str_l = len_constant_str_len;
        NetEvent *r_event = start_event;

        int read_len = len_num + len_constant_str_len + 1;

        size_t remaining = start_event->size - event_offset;
        if (remaining >= read_len) {
            bags->cursors_event_offset += read_len;
        } else {
            start_event = start_event->next;
            while (start_event && read_len > remaining + start_event->size) {
                remaining += start_event->size;
                start_event = start_event->next;
            }
            bags->cursors_event_offset = read_len - remaining;
        }
        bags->cursors_event = start_event;
        bags->offset += read_len;
        bags->ccl = 0;
        bags->ccsl = 0;
        bags->cccsl = 0;
        return r_event;
    }

    return NULL;
}


int
paserfdbags(FdBuffer *bags, char **rd) {
    // 处理具体的拆包和粘包
    // buffer 中的 events 是 线性的 这由网络的协议保证
    // len 就是当前 整个线性长度
    // offset 指消费指针 为什么要写offset
    // 如果没有 offset 每消费一批(完整的) 都需要删除或清理
    // 但如果有 offset 就可以跳过已经消费完的(当恰当的时机清理全部 例如close)

    // offset 和 len 都没有0开始的概念 它不是index(C标准 index 从 0 开始) 它是 size
    if (bags->len == 0 || bags->offset == bags->len) {
        //已经消费完了 or 没有新内容
        return 0;
    }
    size_t lt = 0;
    NetEvent *start = bags->head;
    while (bags->offset >= lt + start->size) {
        lt += start->size;
        start = start->next;
    }
    size_t event_offset = bags->offset - lt;
    char *data_start = start->data + event_offset;

    int len_num = bags->ccl;
    int len_constant_str_len = bags->cccsl;
    int len_str_l = bags->ccsl;

    if (len_num && len_num + len_constant_str_len + 1 <= bags->len) // It's ready
        goto parser;

    char c = 0;
    while (start) {
        int f_count = start->size - event_offset;
        for (int i = 0; i < f_count; ++i) {
            c = data_start[i];
            if (c >= '0' && c <= '9') {
                len_num = len_num * 10 + (c - '0');
                len_constant_str_len++;
                len_str_l++;
                continue;
            }
            if (c != ':') {
                fprintf(stderr, "Invalid format!\n");
                return -1; // error
            }
            if (len_num + len_constant_str_len + 1 <= bags->len) {
                bags->ccl = len_num;
                bags->ccsl = len_str_l;
                bags->cccsl = len_constant_str_len;
                goto parser;
            }
            return 0; //no ready
        }
        start = start->next;
        if (start) {
            len_str_l -= f_count;
            data_start = start->data;
        }
    }
    return 0; // no find ':'

parser:
    if (bags->offset + len_num + 1 > bags->len) {
        return 0; // no ready
    }
    char *d = calloc(1, len_num + 1);
    if (!d) {
        printf("calloc failed!\n");
        return -1;
    }
    int remain = 0;
    if ((remain = start->size - (int) event_offset - len_str_l - len_num - 1) >= 0) {
        memcpy(d, start->data + len_str_l + event_offset + 1, len_num);
        *rd = d;
        bags->offset += len_num + len_str_l + 1;
        //消费成功之后需要重置cache_l
        bags->ccl = 0;
        bags->ccsl = 0;
        bags->cccsl = 0;
        return len_num;
    }
    size_t d_offset = 0;
    memcpy(d, start->data + event_offset + len_str_l + 1, start->size - event_offset - len_str_l);
    // Skip char -> :
    d_offset += start->size - event_offset - len_str_l - 1;
    start = start->next;

    while (start && remain + start->size <= 0) {
        memcpy(d + d_offset, start->data, start->size);
        d_offset += start->size;
        remain += start->size;
        start = start->next;
    }
    if (!start && remain != 0) {
        return 0; //no ready
    }
    if (remain != 0) {
        memcpy(d + d_offset, start->data, -remain);
    }
    bags->offset += len_num + len_str_l + 0;
    bags->ccl = 0;
    bags->ccsl = 0;
    bags->cccsl = 0;
    *rd = d;
    return len_num;
}
