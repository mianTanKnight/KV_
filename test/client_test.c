//
// Created by wenshen on 25-5-6.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../client/kvclient.h"

// 简单测试函数，用于检查响应
void check_response(KVClient *client, const char *operation) {
    printf("Response for %s: %.*s\n", operation, (int) client->offset, client->buffer);
}

// 获取当前时间（毫秒）
long long get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int main111() {
    // 记录开始时间
    long long start_time = get_current_time_ms();

    // 创建客户端连接到服务器
    KVClient *client = kv_client_create("127.0.0.1", 7000, -1, 0);
    if (!client) {
        fprintf(stderr, "Failed to create client: %s\n", strerror(errno));
        return 1;
    }

    printf("Connected to server successfully\n");

    const char *key1 = "testkey1";
    const char *value1 = "testvalue1";

    LOG_INFO("START");
    for (int i = 0; i < 150000; ++i) {
        // 测试 SET 操作
        if (kv_set(client, (char *) key1, strlen(key1), (char *) value1, strlen(value1)) < 0) {
            fprintf(stderr, "Failed to prepare SET command\n");
            kv_client_destroy(&client);
            return 1;
        }

        if (kv_send(client) < 0) {
            fprintf(stderr, "Failed to send SET command\n");
            kv_client_destroy(&client);
            return 1;
        }

        // // 测试 GET 操作
        // if (kv_get(client, (char *) key1, strlen(key1)) < 0) {
        //     fprintf(stderr, "Failed to prepare GET command\n");
        //     kv_client_destroy(&client);
        //     return 1;
        // }
        //
        // if (kv_send(client) < 0) {
        //     fprintf(stderr, "Failed to send GET command\n");
        //     kv_client_destroy(&client);
        //     return 1;
        // }
        //
        // // 测试 DEL 操作
        // if (kv_del(client, (char *) key1, strlen(key1)) < 0) {
        //     fprintf(stderr, "Failed to prepare DEL command\n");
        //     kv_client_destroy(&client);
        //     return 1;
        // }
        //
        // if (kv_send(client) < 0) {
        //     fprintf(stderr, "Failed to send DEL command\n");
        //     kv_client_destroy(&client);
        //     return 1;
        // }
        //
        // // 测试 EXPIRE 操作
        // const char *key2 = "testkey2";
        // const char *expire_time = "10"; // 10秒后过期
        //
        // // 先设置一个值
        // if (kv_set(client, (char *) key2, strlen(key2), (char *) value1, strlen(value1)) < 0) {
        //     fprintf(stderr, "Failed to prepare SET command for EXPIRE test\n");
        //     kv_client_destroy(&client);
        //     return 1;
        // }
        //
        // if (kv_send(client) < 0) {
        //     fprintf(stderr, "Failed to send SET command for EXPIRE test\n");
        //     kv_client_destroy(&client);
        //     return 1;
        // }
        //
        // // 然后设置过期时间
        // if (kv_expire(client, (char *) key2, strlen(key2), (char *) expire_time, strlen(expire_time)) < 0) {
        //     fprintf(stderr, "Failed to prepare EXPIRE command\n");
        //     kv_client_destroy(&client);
        //     return 1;
        // }
        //
        // if (kv_send(client) < 0) {
        //     fprintf(stderr, "Failed to send EXPIRE command\n");
        //     kv_client_destroy(&client);
        //     return 1;
        // }
        //
        // // 获取值（可能未过期）
        // if (kv_get(client, (char *) key2, strlen(key2)) < 0) {
        //     fprintf(stderr, "Failed to prepare GET command for expired key\n");
        //     kv_client_destroy(&client);
        //     return 1;
        // }
        //
        // if (kv_send(client) < 0) {
        //     fprintf(stderr, "Failed to send GET command for expired key\n");
        //     kv_client_destroy(&client);
        //     return 1;
        // }
    }

    LOG_INFO("END");

    // 计算总耗时（毫秒）
    long long end_time = get_current_time_ms();
    long long total_time_ms = end_time - start_time;

    printf("Test completed successfully\n");
    printf("Total execution time: %lld ms\n", total_time_ms);
    printf("Average time per operation: %.3f ms\n", (double)total_time_ms / (10000 * 5));  // 5 ops per iteration (SET, GET, DEL, SET+EXPIRE, GET)

    // 清理
    kv_client_destroy(&client);
    return 0;
}
