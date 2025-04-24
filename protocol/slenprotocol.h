//
// Created by wenshen on 25-4-21.
//

#ifndef SLENPROTOCOL_H
#define SLENPROTOCOL_H
#include "../common.h"
#include "../core/k_v.h"
#include "../server/server_.h"
/**
 * 基于长度的简单文本协议
 * <length>:<data>\n
 */

#define MAX_DATA_LEN 50 * 1024 * 1024 // max str len
#define MAX_DATA_LEN_NUMBER_STR_SIZE 10

typedef struct fd_info {
    int fd;
    struct fd_info *next;
    struct fd_info *prev;
} FdInfo;

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

    FdInfo *info;
} FdBuffer;

typedef struct net_bags_manager {
    HashTable *fds_bag_tabel; // k : "fd" , v : *fdBuffer
    FdInfo *info_head;
    FdInfo *info_tail;
    size_t fd_info_size;
} NetBagsManager;


/**
 * Client Api
 */
size_t slenpro(const char *data, size_t data_len, char **pro_str);


/**
 * Server Api
 */
extern NetBagsManager *net_bags_manager; //全局的唯一的 -> Server

int init_net_bags_manager(size_t init_n);

int neatenbags(NetEvent *curr_event);

int paserfdbags(int fd, char **rd); // rd -> result_data

void destroy_fd(int fd);

void close_fd(int fd);

void destroy_manager();


#endif //SLENPROTOCOL_H
