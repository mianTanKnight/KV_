//
// Created by wenshen on 25-3-27.
//

#ifndef K_V_H
#define K_V_H
#include "../common.h"
#include "sys/time.h"

typedef void (*value_free_func)(void *);

typedef struct MateDate {
    struct timeval *create_time;
    struct timeval *expire_time;
    // ...more
} MateDate;

typedef struct HashTable {
    struct Node **table;
    float load_factor;
    size_t capacity;
    size_t node_size;
    value_free_func value_free_func;
} HashTable;

typedef struct Node {
    char *key;
    void *value;
    MateDate *mate_data;
    struct Node *prev;
    struct Node *next;
} Node;

void init(HashTable *table, size_t capacity, value_free_func value_free_func);

bool put(HashTable *table, const char *key, void *data, unsigned int expire);

void *get(HashTable *table, const char *key);

void *remove_(HashTable *table, const char *key);

void expire(HashTable *table, const char *key, unsigned int expire);

void clear(HashTable *table);

/**
 * 这是个危险的方法 它不受 clear  (因为v 在 value_free_func为NULL 的情况下不受清理)
 * 但它配合 remove_ 外部清理
 *
 */
bool put_pointer(HashTable *table, const char *key, void *data, unsigned int expire);

#endif //K_V_H
