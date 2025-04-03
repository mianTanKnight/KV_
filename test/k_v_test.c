//
// Created by wenshen on 25-3-31.
//
#include <stdio.h>
#include "../core/k_v.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <uuid/uuid.h>
#include <stdarg.h>  // 添加这一行，用于va_list, va_start 和 va_end
#include <unistd.h>
#include <errno.h>
#include <limits.h>

// 生成UUID字符串
void generate_uuid(char *uuid_str) {
    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, uuid_str);
}

void performance_test_uuid() {
    printf("开始UUID性能测试...\n");

    HashTable table;
    init(&table, 1000, free);

    const int TEST_SIZE = 100000;
    char **keys = malloc(sizeof(char *) * TEST_SIZE);
    char **values = malloc(sizeof(char *) * TEST_SIZE);

    // 生成UUID测试数据
    for (int i = 0; i < TEST_SIZE; i++) {
        keys[i] = malloc(37); // UUID字符串长度36 + 终止符
        values[i] = malloc(20);
        generate_uuid(keys[i]);
        sprintf(values[i], "value%d", i);
    }

    // 测试插入性能
    clock_t start = clock();
    for (int i = 0; i < TEST_SIZE; i++) {
        put(&table, keys[i], values[i], -1);
    }
    clock_t end = clock();
    printf("插入 %d 个UUID键值对耗时: %f 秒\n", TEST_SIZE, (double) (end - start) / CLOCKS_PER_SEC);

    // 测试查找性能
    start = clock();
    for (int i = 0; i < TEST_SIZE; i++) {
        get(&table, keys[i]);
    }
    end = clock();
    printf("查找 %d 个UUID键值对耗时: %f 秒\n", TEST_SIZE, (double) (end - start) / CLOCKS_PER_SEC);

    // 测试随机查找性能
    start = clock();
    for (int i = 0; i < TEST_SIZE; i++) {
        int idx = rand() % TEST_SIZE;
        get(&table, keys[idx]);
    }
    end = clock();
    printf("随机查找 %d 次UUID键耗时: %f 秒\n", TEST_SIZE, (double) (end - start) / CLOCKS_PER_SEC);

    // 测试删除性能
    start = clock();
    for (int i = 0; i < TEST_SIZE / 2; i++) {
        remove_(&table, keys[i]);
    }
    end = clock();
    printf("删除 %d 个UUID键值对耗时: %f 秒\n", TEST_SIZE / 2, (double) (end - start) / CLOCKS_PER_SEC);

    // 清理资源
    for (int i = TEST_SIZE / 2; i < TEST_SIZE; i++) {
        remove_(&table, keys[i]);
    }

    for (int i = 0; i < TEST_SIZE; i++) {
        free(keys[i]);
    }
    free(keys);
    free(values);

    clear(&table);
    printf("UUID性能测试完成!\n");
}


void custom_free(void *ptr) {
    if (ptr) free(ptr);
}

// 测试基本的插入和获取功能
void test_basic_put_get() {
    printf("测试基本的插入和获取功能...\n");

    HashTable table;
    init(&table, 5, free);

    char *value1 = strdup("world");
    char *value2 = strdup("value2");

    assert(put(&table, "hello", value1,-1) == true);
    assert(put(&table, "key2", value2,-1) == true);

    assert(strcmp((char*)get(&table, "hello"), "world") == 0);
    assert(strcmp((char*)get(&table, "key2"), "value2") == 0);

    assert(get(&table, "nonexistent") == NULL);
    clear(&table);
    printf("基本插入和获取测试通过！\n");
}

void test_update() {
    printf("测试更新现有键值...\n");

    HashTable table;
    init(&table, 5, free);

    char *value1 = strdup("original");
    put(&table, "key", value1, -1);
    assert(strcmp((char*)get(&table, "key"), "original") == 0);

    char *value2 = strdup("updated");
    put(&table, "key", value2, -1);
    assert(strcmp((char*)get(&table, "key"), "updated") == 0);
    // 旧值应该已经被释放，不需要手动释放value1
    clear(&table);

    printf("更新测试通过！\n");
}

// 测试删除功能
void test_remove() {
    printf("测试删除功能...\n");

    HashTable table;
    init(&table, 5, free);

    char *value1 = strdup("value1");
    char *value2 = strdup("value2");
    char *value3 = strdup("value3");

    put(&table, "key1", value1, -1);
    put(&table, "key2", value2, -1);
    put(&table, "key3", value3, -1);

    assert(table.node_size == 3);

    // 删除存在的键
    remove_(&table, "key2");
    assert(table.node_size == 2);
    assert(get(&table, "key2") == NULL);
    assert(get(&table, "key1") != NULL);
    assert(get(&table, "key3") != NULL);

    // 删除不存在的键
    remove_(&table, "nonexistent");
    assert(table.node_size == 2);

    // 删除头节点
    remove_(&table, "key1");
    assert(table.node_size == 1);
    assert(get(&table, "key1") == NULL);

    // 删除最后一个节点
    remove_(&table, "key3");
    assert(table.node_size == 0);
    assert(get(&table, "key3") == NULL);
    clear(&table);
    printf("删除测试通过！\n");
}

// 测试扩容功能
void test_rehashing() {
    printf("测试扩容功能...\n");

    HashTable table;
    init(&table, 5, free); // 初始容量5，素数7

    size_t initial_capacity = table.capacity;

    // 插入足够多的键触发扩容
    for (int i = 0; i < 20; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        char *value = strdup(key);
        put(&table, key, value, -1);
    }

    // 验证扩容发生了
    assert(table.capacity > initial_capacity);
    printf("容量从 %zu 增加到 %zu\n", initial_capacity, table.capacity);

    // 验证所有数据仍然可以访问
    for (int i = 0; i < 20; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        char *value = (char *) get(&table, key);
        assert(value != NULL);
        assert(strcmp(value, key) == 0);
    }
    clear(&table);
    printf("扩容测试通过！\n");
}

// 测试哈希冲突处理
void test_hash_collision() {
    printf("测试哈希冲突处理...\n");

    // 使用自定义哈希函数，通过修改bucket_i函数来测试
    // 这里我们不修改原代码，而是创造容易冲突的键来测试

    HashTable table;
    init(&table, 3, free); // 使用小容量增加冲突概率

    // 插入多个键值对
    for (int i = 0; i < 10; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        char *value = strdup(key);
        put(&table, key, value, -1);
    }

    // 验证所有数据都能正确访问
    for (int i = 0; i < 10; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        char *value = (char *) get(&table, key);
        assert(value != NULL);
        assert(strcmp(value, key) == 0);
    }
    clear(&table);
    printf("哈希冲突处理测试通过！\n");
}

// 测试边界条件
void test_edge_cases() {
    printf("测试边界条件...\n");

    HashTable table;
    init(&table, 5, free);

    // 测试NULL键和值
    assert(put(&table, NULL, strdup("value"),-1) == false);
    assert(put(&table, "key", NULL,-1) == false);

    // 测试空字符串键
    char *value = strdup("empty key value");
    put(&table, "", value, -1);
    assert(strcmp((char*)get(&table, ""), "empty key value") == 0);

    clear(&table);
    printf("边界条件测试通过！\n");
}

void hashTest() {
    printf("开始哈希表测试...\n\n");

    test_basic_put_get();
    test_update();
    test_remove();
    test_rehashing();
    test_hash_collision();
    test_edge_cases();

    printf("\n所有测试通过！\n");
}

// 性能测试函数
void performance_test() {
    printf("开始性能测试...\n");

    HashTable table;
    init(&table, 1000, free); // 使用较大的初始容量

    char **keys = malloc(sizeof(char *) * 100000);
    char **values = malloc(sizeof(char *) * 100000);

    // 生成测试数据
    for (int i = 0; i < 100000; i++) {
        keys[i] = malloc(20);
        values[i] = malloc(20);
        sprintf(keys[i], "key%d", i);
        sprintf(values[i], "value%d", i);
    }

    // 测试插入性能
    clock_t start = clock();
    for (int i = 0; i < 100000; i++) {
        put(&table, keys[i], values[i], -1);
    }
    clock_t end = clock();
    printf("插入 100,000 个键值对耗时: %f 秒\n", (double) (end - start) / CLOCKS_PER_SEC);

    // 测试查找性能
    start = clock();
    for (int i = 0; i < 100000; i++) {
        get(&table, keys[i]);
    }
    end = clock();
    printf("查找 100,000 个键值对耗时: %f 秒\n", (double) (end - start) / CLOCKS_PER_SEC);

    // 测试随机查找性能
    start = clock();
    for (int i = 0; i < 100000; i++) {
        int idx = rand() % 100000;
        get(&table, keys[idx]);
    }
    end = clock();
    printf("随机查找 100,000 次耗时: %f 秒\n", (double) (end - start) / CLOCKS_PER_SEC);

    // 测试删除性能
    start = clock();
    for (int i = 0; i < 50000; i++) {
        remove_(&table, keys[i]);
    }
    end = clock();
    printf("删除 50,000 个键值对耗时: %f 秒\n", (double) (end - start) / CLOCKS_PER_SEC);

    // 清理测试数据
    for (int i = 50000; i < 100000; i++) {
        remove_(&table, keys[i]);
    }

    for (int i = 0; i < 100000; i++) {
        free(keys[i]);
    }
    free(keys);
    free(values); // values已经由哈希表释放

    printf("性能测试完成!\n");
}


// 测试过期功能
void test_expiry() {
    printf("测试键值过期功能...\n");

    HashTable table;
    init(&table, 5, free);

    // 添加不同过期时间的键值对
    printf("添加过期时间为1000ms的键值对...\n");
    char *value1 = strdup("short-lived value");
    put(&table, "short", value1, 1000); // 1秒过期

    printf("添加过期时间为5000ms的键值对...\n");
    char *value2 = strdup("medium-lived value");
    put(&table, "medium", value2, 5000); // 5秒过期

    printf("添加永不过期的键值对...\n");
    char *value3 = strdup("eternal value");
    put(&table, "eternal", value3, -1); // 永不过期

    // 立即访问所有键
    printf("刚插入后访问所有键...\n");
    assert(get(&table, "short") != NULL);
    assert(get(&table, "medium") != NULL);
    assert(get(&table, "eternal") != NULL);
    printf("所有键都可以访问\n");

    // 等待2秒，短期键应该过期
    printf("等待2秒...\n");
    sleep(2);

    printf("2秒后检查键...\n");
    assert(get(&table, "short") == NULL);
    printf("短期键已过期\n");
    assert(get(&table, "medium") != NULL);
    printf("中期键仍然有效\n");
    assert(get(&table, "eternal") != NULL);
    printf("永久键仍然有效\n");

    // 等待4秒，中期键应该过期
    printf("再等待4秒...\n");
    sleep(4);

    printf("6秒后检查键...\n");
    assert(get(&table, "short") == NULL);
    printf("短期键已过期\n");
    assert(get(&table, "medium") == NULL);
    printf("中期键已过期\n");
    assert(get(&table, "eternal") != NULL);
    printf("永久键仍然有效\n");

    // 测试过期后的内存是否被正确释放
    printf("验证过期键已被删除，节点数应该减少...\n");
    assert(table.node_size == 1);
    printf("节点数正确: %zu\n", table.node_size);
    // 清理
    clear(&table);
    printf("过期功能测试通过！\n");
}

long isNumeric(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0; // 空字符串或 NULL
    }

    char *endptr;
    errno = 0; // 清除之前的错误
    long num = strtol(str, &endptr, 10); // 10 表示十进制

    if (*endptr != '\0') {
        return -1; // 字符串包含非数字字符
    }
    if (errno == ERANGE || num > INT_MAX || num < INT_MIN) {
        return -1; // 数字超出范围
    }

    return num; // 是合法数字
}
