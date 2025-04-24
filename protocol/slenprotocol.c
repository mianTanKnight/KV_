//
// Created by wenshen on 25-4-21.
//

#include "slenprotocol.h"
#include "constant_.h"

NetBagsManager *net_bags_manager;

HashTable hash_table_bags;
HashTable *table_bags = &hash_table_bags;


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
init_net_bags_manager(size_t init_n) {
    net_bags_manager = calloc(1, sizeof(NetBagsManager));
    if (!net_bags_manager) {
        printf("calloc failed!\n");
        return -1;
    }
    // hash_table_bags 是个没有del函数
    // 它通过外部逐步 remove
    init(&hash_table_bags, init_n, NULL);
    net_bags_manager->fds_bag_tabel = table_bags;
    net_bags_manager->info_head = net_bags_manager->info_tail = NULL;
    net_bags_manager->fd_info_size = 0;
    return 0;
}

int
init_fd_net_bags(const char *fd_str, NetEvent *curr_event) {
    FdInfo *info = calloc(1, sizeof(struct fd_info));
    if (!info) {
        LOG_ERROR("calloc failed!");
        return -1;
    }
    info->fd = curr_event->fd;
    info->next = info->prev = NULL;

    FdBuffer *buffer = calloc(1, sizeof(FdBuffer));
    if (!buffer) {
        free(info);
        LOG_ERROR("calloc failed!");
        return -1;
    }

    buffer->head = buffer->tail = curr_event;
    buffer->event_list_size++;
    buffer->offset = 0;
    buffer->len += curr_event->size;
    buffer->info = info;
    buffer->ccl = 0;
    buffer->ccsl = 0;

    put_pointer(net_bags_manager->fds_bag_tabel, fd_str, buffer, -1);

    if (net_bags_manager->fd_info_size == 0) {
        net_bags_manager->info_head = net_bags_manager->info_tail = info;
    } else {
        net_bags_manager->info_tail->next = info;
        info->prev = net_bags_manager->info_tail;
        net_bags_manager->info_tail = info;
    }
    net_bags_manager->fd_info_size++;
    return 1;
}

int
append_net_bags(FdBuffer *buffer, NetEvent *curr_event) {
    buffer->tail->next = curr_event;
    buffer->tail = curr_event;
    buffer->event_list_size++;
    buffer->len += curr_event->size;
    return 1;
}

int
neatenbags(NetEvent *curr_event) {
    if (!net_bags_manager)
        return -1;
    if (!curr_event)
        return 0;
    char *fd_str_constant = getstrconstant(curr_event->fd);
    if (!fd_str_constant) {
        int fd_len = snprintf(NULL, 0, "%d:", curr_event->fd);
        char fd_str[fd_len + 1];
        snprintf(fd_str, fd_len + 1, "%d", curr_event->fd);
        void *v = get(net_bags_manager->fds_bag_tabel, fd_str);
        if (!v) {
            return init_fd_net_bags(fd_str, curr_event);
        }
        return append_net_bags(v, curr_event);
    }
    void *v = get(net_bags_manager->fds_bag_tabel, fd_str_constant);
    if (!v) {
        return init_fd_net_bags(fd_str_constant, curr_event);
    }
    return append_net_bags(v, curr_event);
}


int
paserfdbags(int fd, char **rd) {
    if (!net_bags_manager)
        return 0;
    char *fd_str_constant = getstrconstant(fd);
    void *v;
    if (!fd_str_constant) {
        int fd_len = snprintf(NULL, 0, "%d:", fd);
        char fd_str[fd_len + 1];
        snprintf(fd_str, fd_len + 1, "%d", fd);
        v = get(net_bags_manager->fds_bag_tabel, fd_str);
    } else {
        v = get(net_bags_manager->fds_bag_tabel, fd_str_constant);
    }
    if (v) {
        //处理具体的拆包和粘包
        FdBuffer *bags = v;
        // buffer 中的 events 是 线性的 这由网络的协议保证
        // len 就是当前 整个线性长度
        // offset 指消费指针 为什么要写offset
        // 如果没有 offset 每消费一批(完整的) 都需要删除或清理
        // 但如果有 offset 就可以跳过已经消费完的(当恰当的时机清理全部 例如close)

        // offset 和 len 都没有0开始的概念 它不是index(C标准 index 从 0 开始) 它是 size
        if (bags->offset == bags->len) {
            //已经消费完了 or 没有新内容
            return 0;
        }
        size_t lt = 0;
        NetEvent *start = bags->head;
        while (bags->offset >= (lt + start->size)) {
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
        int f_count = start->size - event_offset;
        while (start) {
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
                f_count = start->size;
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
            d[len_num + 1] = '\0';
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
        bags->offset += len_num + 1 + len_str_l;
        bags->ccl = 0;
        bags->ccsl = 0;
        bags->cccsl = 0;
        d[len_num + 1] = '\0';
        *rd = d;
        return len_num;
    }
    return -2;
}


int
paserfdbags1(int fd, char **rd) {
    if (!net_bags_manager)
        return 0;
    char *fd_str_constant = getstrconstant(fd);
    void *v;
    if (!fd_str_constant) {
        int fd_len = snprintf(NULL, 0, "%d:", fd);
        char fd_str[fd_len + 1];
        snprintf(fd_str, fd_len + 1, "%d", fd);
        v = get(net_bags_manager->fds_bag_tabel, fd_str);
    } else {
        v = get(net_bags_manager->fds_bag_tabel, fd_str_constant);
    }
    if (v) {
        //处理具体的拆包和粘包
        FdBuffer *bags = v;
        // buffer 中的 events 是 线性的 这由网络的协议保证
        // len 就是当前 整个线性长度
        // offset 指消费指针 为什么要写offset
        // 如果没有 offset 每消费一批(完整的) 都需要删除或清理
        // 但如果有 offset 就可以跳过已经消费完的(当恰当的时机清理全部 例如close)
        if (bags->offset == bags->len) {
            return 0;
        }

        size_t lt = 0;
        NetEvent *start = bags->head;
        while (bags->offset >= (lt + start->size)) {
            lt += start->size;
            start = start->next;
        }
        size_t event_offset = bags->offset - lt;
        char *data_start = start->data + event_offset;
        // 使用 strtol 提取 length
        char *endptr;
        long num = strtol(data_start, &endptr, 10);
        if (endptr == data_start) {
            fprintf(stderr, "Invalid format!\n");
            return -1; // error
        }
        size_t endptr_len = strlen(endptr);
        if (*endptr != ':' && strlen(endptr) > 0) {
            fprintf(stderr, "Invalid format!\n");
            return -1; // error
        }
        if (endptr_len == 0 && !start->next) {
            return 0;
        }

        if (endptr_len == 0) {
            char c;
            start = start->next;
            while (start) {
                for (int i = 0; i < start->size; ++i) {
                    c = start->data[i];
                    if (c >= '0' && c <= '9') {
                        num = num * 10 + (c - '0');
                        continue;
                    }
                    if (c != ':') {
                        fprintf(stderr, "Invalid format!\n");
                        return -1; // error
                    }
                    event_offset = i;
                    goto parser;
                }
                start = start->next;
            }
            return 0; // no find ':'
        }

    parser:

        const char *num_str = getstrconstant(num);
        size_t num_str_len;
        if (num_str) {
            num_str_len = strlen(num_str);
        } else {
            num_str_len = start->size - strlen(endptr);
        }

        if (endptr_len == 0) {
            num_str_len = 0;
        }

        if (bags->offset + num + 1 > bags->len) {
            return 0; // no ready
        }
        char *d = calloc(1, num + 1);
        if (!d) {
            printf("calloc failed!\n");
            return -1;
        }
        int remain = 0;
        if ((remain = start->size - (int) event_offset - (int) num_str_len - num - 1) >= 0) {
            memcpy(d, start->data + num_str_len + event_offset + 1, num);
            *rd = d;
            bags->offset += num + num_str_len + 1;
            d[num + 1] = '\0';
            return num;
        }
        size_t d_offset = 0;
        memcpy(d, start->data + event_offset + num_str_len + 1, start->size - event_offset - num_str_len);
        // Skip char -> :
        d_offset += start->size - event_offset - num_str_len - 1;
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
        bags->offset += num + 1 + num_str_len;
        d[num + 1] = '\0';
        *rd = d;
        return (int) num;
    }

    return 0;
}

void
close_fd(int fd) {
    if (!net_bags_manager)
        return;
    char *fd_str_constant = getstrconstant(fd);
    void *v;
    if (!fd_str_constant) {
        int fd_len = snprintf(NULL, 0, "%d:", fd);
        char fd_str[fd_len + 1];
        snprintf(fd_str, fd_len + 1, "%d", fd);
        v = get(net_bags_manager->fds_bag_tabel, fd_str);
    } else {
        v = get(net_bags_manager->fds_bag_tabel, fd_str_constant);
    }
}

void
destroy_fd(int fd) {
    if (!net_bags_manager)
        return;
    char *fd_str_constant = getstrconstant(fd);
    void *v;
    if (!fd_str_constant) {
        int fd_len = snprintf(NULL, 0, "%d:", fd);
        char fd_str[fd_len + 1];
        snprintf(fd_str, fd_len + 1, "%d", fd);
        v = remove_(net_bags_manager->fds_bag_tabel, fd_str);
    } else {
        v = remove_(net_bags_manager->fds_bag_tabel, fd_str_constant);
    }
    if (v) {
        FdBuffer *buffer = v;
        NetEvent *curr_event = buffer->head;
        while (curr_event) {
            NetEvent *next = curr_event->next;
            destroy_event(curr_event);
            curr_event = next;
        }
        FdInfo *info = buffer->info;
        if (net_bags_manager->info_head == info) {
            net_bags_manager->info_head = info->next;
        }
        if (net_bags_manager->info_tail == info) {
            net_bags_manager->info_tail = info->prev;
        }
        if (info->prev) {
            info->prev->next = info->next;
        }
        if (info->next) {
            info->next->prev = info->prev;
        }
        info->prev = info->next = NULL;
        net_bags_manager->fd_info_size--;
        free(buffer);
        free(info);
    }
}

void
destroy_manager() {
    if (!net_bags_manager)
        return;
    FdInfo *current = net_bags_manager->info_head;
    while (current) {
        FdInfo *next = current->next;
        destroy_fd(current->fd);
        current = next;
    }
    free(net_bags_manager->fds_bag_tabel->table);
    free(net_bags_manager);
    net_bags_manager = NULL;
}
