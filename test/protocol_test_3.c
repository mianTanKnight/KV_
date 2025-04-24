//
// Created by wenshen on 25-4-22.
//
#include "../common.h"
#include "../protocol/constant_.h"
#include "../protocol/slenprotocol.h"
#include "../server/server_.h"

#include <time.h>
#include <sys/time.h>

// 计时函数
double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

NetEvent *create_test_event(int fd, const char *data, size_t size) {
     NetEvent *event = calloc(1, sizeof(NetEvent) + size * sizeof(char));
     event->fd = fd;
     memcpy(event->data, data, size);
     event->size = size;
     event->next = NULL;
     return event;
}


// 测试多个小型消息的性能
void benchmark_small_messages(int msg_size, int msg_count) {
    printf("\n=== Benchmark: Small Messages (%d bytes x %d messages) ===\n", msg_size, msg_count);
    init_net_bags_manager(10);

    // 创建多个小型消息
    char **messages = malloc(msg_count * sizeof(char *));
    size_t *message_lengths = malloc(msg_count * sizeof(size_t));

    for (int i = 0; i < msg_count; i++) {
        // 计算前缀 (长度前缀)
        char prefix[32];
        int prefix_len = snprintf(prefix, sizeof(prefix), "%d:", msg_size);

        // 分配内存并构建消息
        messages[i] = malloc(prefix_len + msg_size);
        if (!messages[i]) {
            fprintf(stderr, "Memory allocation failed!\n");
            exit(EXIT_FAILURE);
        }

        // 复制前缀
        memcpy(messages[i], prefix, prefix_len);

        // 填充数据部分 (使用循环字母 A-Z)
        memset(messages[i] + prefix_len, 'A' + (i % 26), msg_size);

        // 保存实际消息长度
        message_lengths[i] = prefix_len + msg_size;
    }

    // 开始计时
    double start_time = get_time_ms();

    // 添加所有消息
    for (int i = 0; i < msg_count; i++) {
        NetEvent *event = create_test_event(1, messages[i], message_lengths[i]);
        neatenbags(event);
    }

    // 解析所有消息
    int parsed_count = 0;
    while (parsed_count < msg_count) {
        char *result = NULL;
        int len = paserfdbags(1, &result);
        if (len > 0) {
            free(result);
            parsed_count++;
        } else {
            // 如果没有更多消息可解析，就退出循环
            break;
        }
    }

    // 结束计时
    double end_time = get_time_ms();
    double elapsed = end_time - start_time;

    // 计算统计数据
    double total_mb = ((double)msg_size * parsed_count) / 1048576.0;
    double throughput = total_mb / (elapsed / 1000.0);

    // 输出结果
    printf("Total time: %.2f ms\n", elapsed);
    printf("Average time per message: %.4f ms\n", elapsed / parsed_count);
    printf("Throughput: %.2f MB/s\n", throughput);
    printf("Messages parsed: %d of %d (%.1f%%)\n",
           parsed_count, msg_count,
           (double)parsed_count / msg_count * 100);

    // 清理资源
    for (int i = 0; i < msg_count; i++) {
        free(messages[i]);
    }
    free(messages);
    free(message_lengths);

    destroy_manager();
}
// 测试分段消息的性能
void benchmark_fragmented_messages(int msg_size, int fragment_count, int iterations) {
    printf("\n=== Benchmark: Fragmented Messages (%d bytes split into %d fragments, %d iterations) ===\n",
           msg_size, fragment_count, iterations);

    // 创建消息和分段
    char prefix[32];
    int prefix_len = snprintf(prefix, sizeof(prefix), "%d:", msg_size);
    size_t total_msg_len = prefix_len + msg_size;

    // 分配完整消息内存
    char *message = malloc(total_msg_len);
    if (!message) {
        fprintf(stderr, "Memory allocation failed for message!\n");
        return;
    }

    // 构建完整消息
    memcpy(message, prefix, prefix_len);
    memset(message + prefix_len, 'B', msg_size);

    // 计算每个分段的大小和创建分段
    int fragment_size = total_msg_len / fragment_count;
    char **fragments = malloc(fragment_count * sizeof(char *));
    size_t *fragment_sizes = malloc(fragment_count * sizeof(size_t));

    if (!fragments || !fragment_sizes) {
        fprintf(stderr, "Memory allocation failed for fragments!\n");
        free(message);
        free(fragments);
        free(fragment_sizes);
        return;
    }

    for (int i = 0; i < fragment_count; i++) {
        int start = i * fragment_size;
        // 最后一个分段可能大小不同
        int size = (i == fragment_count - 1) ? (total_msg_len - start) : fragment_size;

        fragments[i] = malloc(size);
        if (!fragments[i]) {
            fprintf(stderr, "Memory allocation failed for fragment %d!\n", i);
            // 清理已分配的资源
            for (int j = 0; j < i; j++) {
                free(fragments[j]);
            }
            free(fragments);
            free(fragment_sizes);
            free(message);
            return;
        }

        memcpy(fragments[i], message + start, size);
        fragment_sizes[i] = size;
    }

    double start_time = get_time_ms();

    for (int iter = 0; iter < iterations; iter++) {
        // 每次迭代重置状态
        destroy_manager();
        init_net_bags_manager(10);

        // 添加所有分段
        for (int i = 0; i < fragment_count; i++) {
            NetEvent *event = create_test_event(1, fragments[i], fragment_sizes[i]);
            neatenbags(event);
        }

        // 解析消息
        char *result = NULL;
        int len = paserfdbags(1, &result);
        if (len > 0) {
            free(result);
        }
    }

    double end_time = get_time_ms();
    double elapsed = end_time - start_time;

    double throughput_mbps = (double)(msg_size * iterations) / (elapsed / 1000.0) / 1048576.0;

    printf("Total time: %.2f ms\n", elapsed);
    printf("Average time per message: %.4f ms\n", elapsed / iterations);
    printf("Throughput: %.2f MB/s\n", throughput_mbps);

    // 清理资源
    free(message);
    for (int i = 0; i < fragment_count; i++) {
        free(fragments[i]);
    }
    free(fragments);
    free(fragment_sizes);

    destroy_manager();
}


void test_memory_leaks() {
    printf("\n=== Test: Memory Leaks ===\n");

    init_net_bags_manager(10);

    // 1. 创建并销毁大量连接
    for (int i = 0; i < 1000; i++) {
        int fd = i + 100;
        char buffer[32];
        sprintf(buffer, "%d:test%d", 5, i);

        NetEvent *event = create_test_event(fd, buffer, strlen(buffer));
        neatenbags(event);

        // 随机选择一些连接进行解析
        if (i % 3 == 0) {
            char *data;
            paserfdbags(fd, &data);
            if (data) free(data);
        }

        // 随机选择一些连接进行关闭
        if (i % 2 == 0) {
            destroy_fd(fd);
        }
    }

    // 2. 测试错误条件下的内存处理
    NetEvent *bad_event = create_test_event(2000, "bad", 3);
    neatenbags(bad_event);
    char *data;
    paserfdbags(2000, &data);

    // 3. 清理所有剩余资源
    destroy_manager();

    printf("Memory leak test completed\n");
}



// 测试单个大型消息的性能
void benchmark_large_message(int size, int iterations) {
    printf("\n=== Benchmark: Large Message (%d bytes, %d iterations) ===\n", size, iterations);

    // 创建前缀
    char prefix[32];
    int prefix_len = snprintf(prefix, sizeof(prefix), "%d:", size);
    size_t total_len = prefix_len + size;

    // 分配并构建消息
    char *message = malloc(total_len);
    if (!message) {
        fprintf(stderr, "Memory allocation failed for large message!\n");
        return;
    }

    memcpy(message, prefix, prefix_len);
    memset(message + prefix_len, 'A', size);

    double start_time = get_time_ms();

    for (int i = 0; i < iterations; i++) {
        // 每次迭代重置状态
        destroy_manager();
        init_net_bags_manager(10);

        // 添加消息 - 使用确切的长度
        NetEvent *event = create_test_event(1, message, total_len);
        neatenbags(event);

        // 解析消息
        char *result = NULL;
        int len = paserfdbags(1, &result);
        if (len > 0) {
            free(result);
        }
    }

    double end_time = get_time_ms();
    double elapsed = end_time - start_time;

    double throughput_mbps = (double)(size * iterations) / (elapsed / 1000.0) / 1048576.0;

    printf("Total time: %.2f ms\n", elapsed);
    printf("Average time per message: %.4f ms\n", elapsed / iterations);
    printf("Throughput: %.2f MB/s\n", throughput_mbps);

    free(message);
    destroy_manager();
}

//
// // 主函数
// int main(int argc, char *argv[]) {
//
//     init_constants(99999);
//     // // 测试大型单一消息
//     benchmark_large_message(1024 * 1024, 10); // 1MB消息，10次迭代
//
//     // // 测试多个小型消息
//     benchmark_small_messages(256, 10000); // 256字节消息，10000条
//     //
//     // // 测试分段消息
//     benchmark_fragmented_messages(1024 * 10, 5, 100); // 10KB消息，分5段，100次迭代
//
//     printf("Starting performance benchmarks\n\n");
//     test_memory_leaks();
//
//     destroy_constants();
//     printf("\nAll benchmarks completed!\n");
//
//     return 0;
// }
