//
// Created by wenshen on 25-3-27.
//

#include "k_v.h"

#include <math.h>

static unsigned long
hash_djb2(const char *str);

static int
next_prime(int n);

static int
bucket_i(const char *key, HashTable *table);

static Node
*getNode(HashTable *table, const char *key);

static void
head_push(HashTable *table, unsigned int b_index, Node *node);

static void
rehash(HashTable *table);

static void
free_node(HashTable *table, Node *node);

static Node
*create_node(char *key, void *data, unsigned long expire);

static void
del_node(HashTable *table, Node *current);

static unsigned long
hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}


int
bucket_i(const char *key, HashTable *table) {
    return hash_djb2(key) % table->capacity;
}

bool
is_prime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true; // 2 和 3 是素数
    if (n % 2 == 0 || n % 3 == 0) return false; // 排除偶数及 3 的倍数

    // 只需检查 6k ± 1 的因子（优化关键）
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
    }
    return true;
}

int
next_prime(int n) {
    if (n <= 2) return 2;
    while (!is_prime(n)) {
        n++;
    }
    return n;
}


void
free_node(HashTable *table, Node *node) {
    if (node) {
        free(node->key);
        if (table->value_free_func) table->value_free_func(node->value);
        free(node->mate_data->expire_time);
        free(node->mate_data->create_time);
        free(node->mate_data);
        free(node);
        --table->node_size;
    }
}

Node
*create_node(char *key, void *data, unsigned long expire) {
    Node *n_node = malloc(sizeof(Node));
    if (!n_node) {
        LOG_ERROR("out of memory : %s", strerror(errno));
        EXIT_ERROR();
    }
    n_node->key = key;
    n_node->value = data;
    MateDate *md = malloc(sizeof(MateDate));
    if (!md) {
        LOG_ERROR("out of memory : %s", strerror(errno));
        EXIT_ERROR();
    }
    struct timeval *tv_create = malloc(sizeof(struct timeval));
    if (!tv_create) {
        LOG_ERROR("out of memory : %s", strerror(errno));
        EXIT_ERROR();
    }
    gettimeofday(tv_create,NULL);
    struct timeval *tv_expire = malloc(sizeof(struct timeval));
    if (!tv_expire) {
        LOG_ERROR("out of memory : %s", strerror(errno));
        EXIT_ERROR();
    }
    tv_expire->tv_sec = -1;
    if (expire > 0) {
        tv_expire->tv_sec = tv_create->tv_sec + (expire / 1000);
    }
    md->expire_time = tv_expire;
    md->create_time = tv_create;
    n_node->mate_data = md;
    return n_node;
}

void
init(HashTable *table, size_t capacity, value_free_func free_func) {
    if (!table) {
        LOG_ERROR("hash table is null");
        return;
    }
    size_t cap = next_prime(capacity);
    Node **tb = calloc(cap, sizeof(Node *));
    if (!tb) {
        LOG_ERROR("out of memory : %s", strerror(errno));
        EXIT_ERROR();
    }
    table->table = tb;
    table->capacity = cap;
    table->node_size = 0;
    table->load_factor = 0.75f;
    table->value_free_func = free_func;
}

Node
*getNode(HashTable *table, const char *key) {
    int b_index = bucket_i(key, table);
    Node *bucket = table->table[b_index];
    if (!bucket) {
        return NULL;
    }
    Node *cur = bucket;
    while (cur) {
        Node *next = cur->next;
        if (cur->mate_data->expire_time->tv_sec > 0) {
            struct timeval tv_create; // stack mem
            gettimeofday(&tv_create,NULL);
            __suseconds_t tv_usec = cur->mate_data->expire_time->tv_sec;
            if (tv_create.tv_sec >= tv_usec) {
                Node *prev = cur->prev;
                if (!prev) {
                    table->table[b_index] = next; //prev 如果是NULL 那么就证明是桶头 我们需要更新
                }
                del_node(table, cur);
            } else {
                if (strcmp(cur->key, key) == 0) {
                    return cur;
                }
            }
        } else {
            if (strcmp(cur->key, key) == 0) {
                return cur;
            }
        }
        cur = next;
    }
    return NULL;
}

bool put_pointer(HashTable *table, const char *key, void *data, unsigned int expire) {
    if (!table || !key || !data)
        return false;

    Node *exists = getNode(table, key);
    if (exists) {
        // 如果键已存在，释放旧值并设置新指针
        table->value_free_func(exists->value);
        exists->value = data;
        return true;
    }

    if (table->node_size + 1 >= (int) (table->capacity * table->load_factor))
        rehash(table);

    char *d_key = strdup(key);
    if (!d_key) {
        return false; // 内存分配失败
    }

    int b_index = bucket_i(d_key, table);
    Node *n_node = create_node(d_key, data, expire);
    // 使用头推法
    head_push(table, b_index, n_node);
    ++table->node_size;

    return true;
}


void
head_push(HashTable *table, unsigned int b_index, Node *node) {
    Node *head = table->table[b_index];
    table->table[b_index] = node;
    node->next = head;
    node->prev = NULL;
    if (head) {
        head->prev = node;
    }
}


void
rehash(HashTable *table) {
    Node **old_tb = table->table;
    unsigned int old_capacity = table->capacity;
    unsigned int new_cap = table->capacity * 2;

    Node **new_tb = calloc(new_cap, sizeof(Node *));
    if (!new_tb) {
        LOG_ERROR("out of memory : %s", strerror(errno));
        EXIT_ERROR();
    }
    table->capacity = new_cap;
    table->table = new_tb;

    for (size_t i = 0; i < old_capacity; i++) {
        Node *node_ = old_tb[i];
        if (node_) {
            Node *cur = node_;
            while (cur) {
                Node *next = cur->next;
                int bucket_index = bucket_i(cur->key, table);
                head_push(table, bucket_index, cur);
                cur = next;
            }
        }
    }
    free(old_tb);
}

void
*get(HashTable *table, const char *key) {
    if (!table || !key)
        return NULL;
    Node *exists = getNode(table, key);
    if (!exists) {
        return NULL;
    }
    return exists->value;
}

void
del_node(HashTable *table, Node *current) {
    Node *prev = current->prev;
    Node *next = current->next;
    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;
    free_node(table, current);
}

void *
remove_(HashTable *table, const char *key) {
    if (!table || !key)
        return NULL;

    Node *exists = getNode(table, key);
    if (!exists) {
        return NULL;
    }
    Node *prev = exists->prev;
    Node *next = exists->next;
    if (!prev) {
        int b_index = bucket_i(key, table);
        table->table[b_index] = next; //prev 如果是NULL 那么就证明是桶头 我们需要更新
    } else {
        prev->next = next;  // 更新prev的next指针指向exists的next
    }
    if (next) {
        next->prev = prev;  // 如果next存在，更新它的prev指针
    }
    void *v = exists->value;
    free_node(table, exists);
    return v;
}

bool
put(HashTable *table, const char *key, void *data, unsigned int expire) {
    if (!table || !key || !data)
        return false;

    Node *exists = getNode(table, key);
    if (exists) {
        // 如果键已存在，释放旧值并设置新值的副本
        void *value_copy = strdup(data);
        if (!value_copy) {
            return false; // 内存分配失败
        }
        table->value_free_func(exists->value);
        exists->value = value_copy;
        return true;
    }

    if (table->node_size + 1 >= (int) (table->capacity * table->load_factor))
        rehash(table);

    // 为key创建副本
    char *d_key = strdup(key);
    if (!d_key) {
        return false; // 内存分配失败
    }

    // 为value创建副本
    void *value_copy = strdup(data);
    if (!value_copy) {
        free(d_key);
        return false; // 内存分配失败
    }

    int b_index = bucket_i(d_key, table);
    Node *n_node = create_node(d_key, value_copy, expire);
    // 使用头推法
    head_push(table, b_index, n_node);
    ++table->node_size;

    return true;
}

void
expire(HashTable *table, const char *key, unsigned int expire) {
    Node *exists = getNode(table, key);
    if (exists)
        exists->mate_data->expire_time->tv_sec = exists->mate_data->create_time->tv_sec + (expire / 1000);
}


void
clear(HashTable *table) {
    if (!table || !table->table)
        return;
    for (size_t i = 0; i < table->capacity; i++) {
        Node *node_ = table->table[i];
        if (node_) {
            Node *cur = node_;
            while (cur) {
                Node *next = cur->next;
                free_node(table, cur);
                cur = next;
            }
        }
    }
    free(table->table);
    table->table = NULL;
    table->capacity = 0;
    table->node_size = 0;
}
