//
// Created by wenshen on 25-4-21.
//
#include "slenprotocol.h"


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
 * 基于 len:data 协议
 * paserfdbags_zero_copy 没有然后的内存申请操作(本地栈除外)
 * 使用游标支持 cache (buffer 会频繁受检) @see typedef struct fd_buffer
 *
 * step1 : 获取当前有效游标
 * step2 : 是否分析出len (如果没有实现分析len(copy *) 并同步游标)
 * step3 : 分析 events
 *
 */
NetEvent *
paserfdbags_zero_copy(FdBuffer *bags, size_t *eventOffset, size_t *len, size_t *len_str_l) {
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

// discard
int
paserfdbags(FdBuffer *bags, char **rd) {
    if (bags->len == 0 || bags->offset == bags->len) {
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
    return 0;

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
        bags->ccl = 0;
        bags->ccsl = 0;
        bags->cccsl = 0;
        return len_num;
    }
    size_t d_offset = 0;
    memcpy(d, start->data + event_offset + len_str_l + 1, start->size - event_offset - len_str_l);
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
