// // //
// // // Created by wenshen on 25-4-28.
// // //
// // // 测试辅助函数
// #include "../protocol/buffers_.h"
// #include "../protocol/slenprotocol.h"
// #include "../server/server_.h"
// //
// // // 创建事件并添加到缓冲区
// // // 创建事件并添加到缓冲区
// // NetEvent *create_event__(const char *data, size_t size) {
// //     // 使用柔性数组，分配整个结构体加数据大小的内存
// //     NetEvent *event = (NetEvent *) malloc(sizeof(NetEvent) + size);
// //     assert(event != NULL);
// //
// //     // 初始化事件属性
// //     event->type = 1; // 假设是读事件
// //     event->fd = 0; // 假设文件描述符
// //     event->next = NULL;
// //     event->size = size;
// //
// //     // 复制数据到柔性数组
// //     memcpy(event->data, data, size);
// //
// //     return event;
// // }
// //
// // // 清理缓冲区
// // void free_buffer__(FdBuffer *buffer) {
// //     NetEvent *current = buffer->head;
// //     while (current) {
// //         NetEvent *next = current->next;
// //         free(current->data);
// //         free(current);
// //         current = next;
// //     }
// //
// //     memset(buffer, 0, sizeof(FdBuffer));
// // }
// //
// // // 初始化缓冲区
// // void init_buffer__(FdBuffer *buffer) {
// //     memset(buffer, 0, sizeof(FdBuffer));
// // }
// //
// // // 验证解析结果
// // void verify_parse_result(NetEvent *result, size_t eventOffset, size_t len, size_t len_str_l,
// //                          NetEvent *expected_event, size_t expected_offset, size_t expected_len,
// //                          size_t expected_len_str_l, const char *test_name) {
// //     if (expected_event == NULL) {
// //         assert(result == NULL);
// //         printf("✅ %s: NULL result verified\n", test_name);
// //         return;
// //     }
// //
// //     assert(result == expected_event);
// //     assert(eventOffset == expected_offset);
// //     assert(len == expected_len);
// //     assert(len_str_l == expected_len_str_l);
// //
// //     printf("✅ %s: Result verified successfully\n", test_name);
// // }
// //
// // // 验证缓冲区状态
// // void verify_buffer_state(FdBuffer *buffer, NetEvent *expected_cursor, size_t expected_offset,
// //                          size_t expected_global_offset, int expected_ccl, int expected_cccsl,
// //                          int expected_ccsl, const char *test_name) {
// //     assert(buffer->cursors_event == expected_cursor);
// //     assert(buffer->cursors_event_offset == expected_offset);
// //     assert(buffer->offset == expected_global_offset);
// //     assert(buffer->ccl == expected_ccl);
// //     assert(buffer->cccsl == expected_cccsl);
// //     assert(buffer->ccsl == expected_ccsl);
// //
// //     printf("✅ %s: Buffer state verified successfully\n", test_name);
// // }
// //
// // // 2. 消息刚好填满一个事件
// // void test_exact_event_size() {
// //     const char *TEST_NAME = "消息刚好填满一个事件";
// //     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
// //
// //     FdBuffer buffer;
// //     init_buffer__(&buffer);
// //
// //     // 创建消息刚好填满事件的场景
// //     NetEvent *e1 = create_event__("3:abc", 5);
// //     neatenbags(e1, &buffer);
// //
// //     // 解析
// //     size_t eventOffset, len, len_str_l;
// //     NetEvent *result = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
// //
// //     // 验证返回结果
// //     verify_parse_result(result, eventOffset, len, len_str_l,
// //                         e1, 0, 3, 1, TEST_NAME);
// //
// //     // 验证缓冲区状态 - cursor应该是NULL，因为下一个事件不存在
// //     verify_buffer_state(&buffer, e1, 5, 5, 0, 0, 0, TEST_NAME);
// //
// //     // 清理
// //     printf("=== 测试完成: %s ===\n", TEST_NAME);
// // }
// //
// // // 3. 消息跨越多个事件
// // void test_cross_events() {
// //     const char *TEST_NAME = "消息跨越多个事件";
// //     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
// //
// //     FdBuffer buffer;
// //     init_buffer__(&buffer);
// //
// //     // 创建跨越多个事件的消息
// //     NetEvent *e1 = create_event__("11:hel", 6);
// //     NetEvent *e2 = create_event__("lo wor", 6);
// //     NetEvent *e3 = create_event__("ld", 2);
// //
// //     neatenbags(e1, &buffer);
// //     neatenbags(e2, &buffer);
// //     neatenbags(e3, &buffer);
// //
// //     // 解析
// //     size_t eventOffset, len, len_str_l;
// //     NetEvent *result = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
// //
// //     // 验证返回结果
// //     verify_parse_result(result, eventOffset, len, len_str_l,
// //                         e1, 0, 11, 2, TEST_NAME);
// //
// //     // 验证缓冲区状态 - cursor应该指向e3，偏移量应该是正确的
// //     verify_buffer_state(&buffer, e3, 2, 14, 0, 0, 0, TEST_NAME);
// //
// //     printf("=== 测试完成: %s ===\n", TEST_NAME);
// // }
// //
// // // 1. 简单消息在单个事件内
// // void test_single_event() {
// //     const char *TEST_NAME = "简单消息在单个事件内";
// //     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
// //
// //     FdBuffer buffer;
// //     init_buffer__(&buffer);
// //
// //     // 创建包含"5:hello"的单个事件
// //     NetEvent *e1 = create_event__("5:hello", 7);
// //     neatenbags(e1, &buffer);
// //
// //     // 解析
// //     size_t eventOffset, len, len_str_l;
// //     NetEvent *result = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
// //
// //     // 验证返回结果
// //     verify_parse_result(result, eventOffset, len, len_str_l,
// //                         e1, 0, 5, 1, TEST_NAME);
// //
// //     // 验证缓冲区状态 - cursor应该指向e1，偏移量应该是7
// //     verify_buffer_state(&buffer, e1, 7, 7, 0, 0, 0, TEST_NAME);
// //
// //     // 再次解析应该返回NULL（没有更多数据）
// //     result = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
// //     assert(result == NULL);
// //
// //     // 清理
// //     printf("=== 测试完成: %s ===\n", TEST_NAME);
// // }
// //
// // // 4. 长度字段跨事件
// // void test_length_cross_events() {
// //     const char *TEST_NAME = "长度字段跨事件";
// //     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
// //
// //     FdBuffer buffer;
// //     init_buffer__(&buffer);
// //
// //     // 创建长度字段跨事件的消息
// //     NetEvent *e1 = create_event__("1", 1);
// //     NetEvent *e2 = create_event__("0:hellohello", 12);
// //
// //     neatenbags(e1, &buffer);
// //     neatenbags(e2, &buffer);
// //
// //     // 解析
// //     size_t eventOffset, len, len_str_l;
// //     NetEvent *result = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
// //
// //     // 验证返回结果
// //     verify_parse_result(result, eventOffset, len, len_str_l,
// //                         e1, 0, 10, 2, TEST_NAME);
// //
// //     // 验证缓冲区状态
// //     verify_buffer_state(&buffer, e2, 12, 13, 0, 0, 0, TEST_NAME);
// //
// //     printf("=== 测试完成: %s ===\n", TEST_NAME);
// // }
// //
// //
// // // 5. 连续解析多个消息
// // void test_multiple_messages() {
// //     const char *TEST_NAME = "连续解析多个消息";
// //     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
// //
// //     FdBuffer buffer;
// //     init_buffer__(&buffer);
// //
// //     // 创建包含多个消息的事件
// //     NetEvent *e1 = create_event__("5:hello3:abc", 12);
// //     neatenbags(e1, &buffer);
// //
// //     // 解析第一个消息
// //     size_t eventOffset, len, len_str_l;
// //     NetEvent *result1 = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
// //
// //     // 验证第一个消息的返回结果
// //     verify_parse_result(result1, eventOffset, len, len_str_l,
// //                         e1, 0, 5, 1, " (Message 1)");
// //
// //     // 验证缓冲区状态
// //     verify_buffer_state(&buffer, e1, 7, 7, 0, 0, 0, " (After Message 1)");
// //
// //     // 解析第二个消息
// //     NetEvent *result2 = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
// //
// //     // 验证第二个消息的返回结果
// //     verify_parse_result(result2, eventOffset, len, len_str_l,
// //                         e1, 7, 3, 1, " (Message 2)");
// //
// //     // 验证缓冲区状态
// //     verify_buffer_state(&buffer, e1, 12, 12, 0, 0, 0, " (After Message 2)");
// //
// //     // 再次解析应该返回NULL（没有更多数据）
// //     NetEvent *result3 = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
// //     assert(result3 == NULL);
// //
// //     // 清理
// //     printf("=== 测试完成: %s ===\n", TEST_NAME);
// // }
// //
// // // 6. 错误格式处理
// // void test_invalid_format() {
// //     const char *TEST_NAME = "错误格式处理";
// //     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
// //
// //     FdBuffer buffer;
// //     init_buffer__(&buffer);
// //
// //     // 创建格式错误的事件 - 没有冒号
// //     NetEvent *e1 = create_event__("5hello", 6);
// //     neatenbags(e1, &buffer);
// //
// //     // 解析
// //     size_t eventOffset, len, len_str_l;
// //     NetEvent *result = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
// //
// //     // 应该返回NULL表示错误
// //     assert(result == NULL);
// //     printf("✅ %s: NULL result for invalid format verified\n", TEST_NAME);
// //
// //     // 清理
// //     printf("=== 测试完成: %s ===\n", TEST_NAME);
// // }
// //
// // void test_empty_buffer() {
// //     const char *TEST_NAME = "空缓冲区";
// //     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
// //
// //     FdBuffer buffer;
// //     init_buffer__(&buffer);
// //
// //     // 解析空缓冲区
// //     size_t eventOffset, len, len_str_l;
// //     NetEvent *result = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
// //
// //     // 应该返回NULL
// //     assert(result == NULL);
// //     printf("✅ %s: NULL result for empty buffer verified\n", TEST_NAME);
// //
// //     // 清理
// //     printf("=== 测试完成: %s ===\n", TEST_NAME);
// // }
// //
// //
// // // 8. 大数据量测试
// // void test_large_data() {
// //     const char *TEST_NAME = "大数据量测试";
// //     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
// //
// //     FdBuffer buffer;
// //     init_buffer__(&buffer);
// //
// //     // 创建长度前缀指示大数据量的消息
// //     char prefix[20];
// //     sprintf(prefix, "1000:");
// //     NetEvent *e1 = create_event__(prefix, strlen(prefix));
// //     neatenbags(e1, &buffer);
// //
// //     // 创建多个事件来容纳大数据
// //     const int chunks = 10;
// //     NetEvent *events[chunks];
// //
// //     for (int i = 0; i < chunks; i++) {
// //         char chunk[101];
// //         memset(chunk, 'A' + i % 26, 100);
// //         chunk[100] = '\0';
// //         events[i] = create_event__(chunk, 100);
// //         neatenbags(events[i], &buffer);
// //     }
// //
// //     // 解析
// //     size_t eventOffset, len, len_str_l;
// //     NetEvent *result = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
// //
// //     // 验证返回结果
// //     verify_parse_result(result, eventOffset, len, len_str_l,
// //                         e1, 0, 1000, 4, TEST_NAME);
// //
// //     // 验证缓冲区状态
// //     verify_buffer_state(&buffer, events[chunks - 1], 100, 1005, 0, 0, 0, TEST_NAME);
// //
// //     // 清理
// //     printf("=== 测试完成: %s ===\n", TEST_NAME);
// // }
// //
// // // 9. 从中间开始解析
// // void test_start_from_middle() {
// //     const char *TEST_NAME = "从中间开始解析";
// //     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
// //
// //     FdBuffer buffer;
// //     init_buffer__(&buffer);
// //
// //     // 创建事件
// //     NetEvent *e1 = create_event__("xxxx5:hello", 11);
// //     neatenbags(e1, &buffer);
// //
// //     // 设置起始位置在中间
// //     buffer.cursors_event = e1;
// //     buffer.cursors_event_offset = 4;
// //
// //     // 解析
// //     size_t eventOffset, len, len_str_l;
// //     NetEvent *result = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
// //
// //     // 验证返回结果
// //     verify_parse_result(result, eventOffset, len, len_str_l,
// //                         e1, 4, 5, 1, TEST_NAME);
// //
// //     // 验证缓冲区状态
// //     verify_buffer_state(&buffer, e1, 11, 7, 0, 0, 0, TEST_NAME);
// //
// //     // 清理
// //     printf("=== 测试完成: %s ===\n", TEST_NAME);
// // }
// //
// // // 10. 解析不完整的消息
// // void test_incomplete_message() {
// //     const char *TEST_NAME = "解析不完整的消息";
// //     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
// //
// //     FdBuffer buffer;
// //     init_buffer__(&buffer);
// //
// //     // 创建不完整的消息 - 长度指示10字节，但只有5字节可用
// //     NetEvent *e1 = create_event__("10:hello", 8);
// //     neatenbags(e1, &buffer);
// //
// //     // 解析
// //     size_t eventOffset, len, len_str_l;
// //     NetEvent *result = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
// //
// //     // 应该返回NULL表示数据不足
// //     assert(result == NULL);
// //     printf("✅ %s: NULL result for incomplete message verified\n", TEST_NAME);
// //
// //     // 验证缓冲区状态 - 应该保持解析状态，等待更多数据
// //     assert(buffer.ccl == 10); // 长度已解析
// //
// //     // 添加更多数据
// //     NetEvent *e2 = create_event__(" world", 6);
// //     neatenbags(e2, &buffer);
// //
// //     // 再次解析
// //     result = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
// //
// //     // 这次应该成功
// //     verify_parse_result(result, eventOffset, len, len_str_l,
// //                         e1, 0, 10, 2, TEST_NAME);
// //
// //     // 验证缓冲区状态
// //     verify_buffer_state(&buffer, e2, 5, 13, 0, 0, 0, TEST_NAME);
// //
// //     printf("=== 测试完成: %s ===\n", TEST_NAME);
// // }
// //
// //
// // // 性能测试
// // void benchmark_performance() {
// //     const char *TEST_NAME = "性能测试";
// //     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
// //
// //     const int ITERATIONS = 100000;
// //
// //     // 准备测试数据 - original实现
// //     FdBuffer buffer_original;
// //     init_buffer__(&buffer_original);
// //
// //     NetEvent *e1_original = create_event__("5:hello", 7);
// //     neatenbags(e1_original, &buffer_original);
// //
// //     // 准备测试数据 - zero_copy实现
// //     FdBuffer buffer_zero_copy;
// //     init_buffer__(&buffer_zero_copy);
// //
// //     NetEvent *e1_zero_copy = create_event__("5:hello", 7);
// //     neatenbags(e1_zero_copy, &buffer_zero_copy);
// //
// //     // 测试原始实现
// //     clock_t start = clock();
// //     for (int i = 0; i < ITERATIONS; i++) {
// //         char *data;
// //         int result = paserfdbags(&buffer_original, &data);
// //         if (result > 0) {
// //             free(data); // 原实现需要释放内存
// //
// //             // 重置测试环境
// //             buffer_original.offset = 0;
// //             buffer_original.ccl = 0;
// //             buffer_original.cccsl = 0;
// //             buffer_original.ccsl = 0;
// //         }
// //     }
// //     clock_t end = clock();
// //     double original_time = (double) (end - start) / CLOCKS_PER_SEC;
// //
// //     // 测试零拷贝实现
// //     start = clock();
// //     for (int i = 0; i < ITERATIONS; i++) {
// //         size_t eventOffset, len, len_str_l;
// //         NetEvent *result = paserfdbags_zero_copy(&buffer_zero_copy, &eventOffset, &len, &len_str_l);
// //         if (result) {
// //             // 重置测试环境
// //             buffer_zero_copy.offset = 0;
// //             buffer_zero_copy.cursors_event = buffer_zero_copy.head;
// //             buffer_zero_copy.cursors_event_offset = 0;
// //             buffer_zero_copy.ccl = 0;
// //             buffer_zero_copy.cccsl = 0;
// //             buffer_zero_copy.ccsl = 0;
// //         }
// //     }
// //     end = clock();
// //     double zero_copy_time = (double) (end - start) / CLOCKS_PER_SEC;
// //
// //     printf("原始实现: %.6f 秒\n", original_time);
// //     printf("零拷贝实现: %.6f 秒\n", zero_copy_time);
// //     printf("性能提升: %.2f%%\n", (original_time - zero_copy_time) / original_time * 100);
// //
// //     // 清理
// //     printf("=== 测试完成: %s ===\n", TEST_NAME);
// // }
// //
// //
// // // 主测试函数
// // void test_paserfdbags_zero_copy() {
// //     printf("===== 开始全面测试 paserfdbags_zero_copy =====\n");
// //
// //     // 测试用例集
// //     test_single_event();
// //     test_exact_event_size();
// //     test_cross_events();
// //     test_length_cross_events();
// //     test_multiple_messages();
// //     test_invalid_format();
// //     test_empty_buffer();
// //     test_large_data();
// //     test_start_from_middle();
// //     test_incomplete_message();
// //
// //     // 性能测试
// //     benchmark_performance();
// //
// //     printf("===== 全面测试完成 =====\n");
// // }
// //
// //
// // int main() {
// //     test_paserfdbags_zero_copy();
// //     return 0;
// // }
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <assert.h>
// #include <ctype.h>
//
// #include "../commands/command_.h"
// #include "../protocol/slenprotocol.h"
//
// // 测试辅助函数
// NetEvent* create_test_event(const char* data, size_t size) {
//     NetEvent *event = (NetEvent*)malloc(sizeof(NetEvent) + size);
//     assert(event != NULL);
//
//     event->type = 1;
//     event->fd = 0;
//     event->next = NULL;
//     event->size = size;
//
//     memcpy(event->data, data, size);
//
//     return event;
// }
//
// // 释放测试事件链
// void free_test_events(NetEvent *head) {
//     while (head) {
//         NetEvent *next = head->next;
//         free(head);
//         head = next;
//     }
// }
//
// // 验证解析结果
// void verify_args(char **actual, char **expected, int count, const char *test_name) {
//     for (int i = 0; i < count; i++) {
//         printf("参数[%d]: 期望='%s', 实际='%s'\n", i, expected[i], actual[i]);
//         assert(strcmp(actual[i], expected[i]) == 0);
//     }
//     printf("✅ %s: 参数验证成功\n", test_name);
// }
//
// // 测试用例1: 基本单参数命令
// void test_basic_command() {
//     const char *TEST_NAME = "基本单参数命令";
//     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
//
//     NetEvent *event = create_test_event("3:GET", 5);
//
//     char *argv[10] = {NULL};
//     char *buffer = NULL;
//     unsigned argc = tokenize_command_zero_copy_(event, 0, 3, 1, argv, 10, &buffer);
//
//     assert(argc == 1);
//     char *expected[] = {"GET"};
//     verify_args(argv, expected, argc, TEST_NAME);
//
//     free(buffer);
//     free_test_events(event);
//     printf("=== 测试完成: %s ===\n", TEST_NAME);
// }
//
// // 测试用例2: 多参数命令
// void test_multi_param_command() {
//     const char *TEST_NAME = "多参数命令";
//     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
//
//     NetEvent *event = create_test_event("11:SET key val", 14);
//
//     char *argv[10] = {NULL};
//     char *buffer = NULL;
//     unsigned argc = tokenize_command_zero_copy_(event, 0, 14, 2, argv, 10, &buffer);
//
//     assert(argc == 3);
//     char *expected[] = {"SET", "key", "val"};
//     verify_args(argv, expected, argc, TEST_NAME);
//
//     free(buffer);
//     free_test_events(event);
//     printf("=== 测试完成: %s ===\n", TEST_NAME);
// }
//
// // 测试用例3: 包含多余空格的命令
// void test_extra_spaces_command() {
//     const char *TEST_NAME = "包含多余空格的命令";
//     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
//
//     NetEvent *event = create_test_event("15:  SET  key  val ", 17);
//
//     char *argv[10] = {NULL};
//     char *buffer = NULL;
//     unsigned argc = tokenize_command_zero_copy_(event, 0, 15, 2, argv, 10, &buffer);
//
//     assert(argc == 3);
//     char *expected[] = {"SET", "key", "val"};
//     verify_args(argv, expected, argc, TEST_NAME);
//
//     free(buffer);
//     free_test_events(event);
//     printf("=== 测试完成: %s ===\n", TEST_NAME);
// }
//
// // 测试用例4: 跨事件命令
// void test_cross_event_command() {
//     const char *TEST_NAME = "跨事件命令";
//     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
//
//     NetEvent *e1 = create_test_event("12:SET ", 6);
//     NetEvent *e2 = create_test_event("key val", 7);
//     e1->next = e2;
//
//     char *argv[10] = {NULL};
//     char *buffer = NULL;
//     unsigned argc = tokenize_command_zero_copy_(e1, 0, 12, 2, argv, 10, &buffer);
//
//     assert(argc == 3);
//     char *expected[] = {"SET", "key", "val"};
//     verify_args(argv, expected, argc, TEST_NAME);
//
//     free(buffer);
//     free_test_events(e1);
//     printf("=== 测试完成: %s ===\n", TEST_NAME);
// }
//
// // 测试用例5: 参数跨事件边界
// void test_param_cross_boundary() {
//     const char *TEST_NAME = "参数跨事件边界";
//     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
//
//     NetEvent *e1 = create_test_event("10:SET k", 7);
//     NetEvent *e2 = create_test_event("ey val", 6);
//     e1->next = e2;
//
//     char *argv[10] = {NULL};
//     char *buffer = NULL;
//     unsigned argc = tokenize_command_zero_copy_(e1, 0, 10, 2, argv, 10, &buffer);
//
//     assert(argc == 3);
//     char *expected[] = {"SET", "key", "val"};
//     verify_args(argv, expected, argc, TEST_NAME);
//
//     free(buffer);
//     free_test_events(e1);
//     printf("=== 测试完成: %s ===\n", TEST_NAME);
// }
//
// // 测试用例6: 最大参数限制
// void test_max_args_limit() {
//     const char *TEST_NAME = "最大参数限制";
//     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
//
//     NetEvent *event = create_test_event("17:a b c d e f g h i", 19);
//
//     char *argv[5] = {NULL};
//     char *buffer = NULL;
//     unsigned argc = tokenize_command_zero_copy_(event, 0, 17, 2, argv, 5, &buffer);
//
//     assert(argc == 5);
//     char *expected[] = {"a", "b", "c", "d", "e"};
//     verify_args(argv, expected, argc, TEST_NAME);
//
//     free(buffer);
//     free_test_events(event);
//     printf("=== 测试完成: %s ===\n", TEST_NAME);
// }
//
// // 测试用例7: 空命令
// void test_empty_command() {
//     const char *TEST_NAME = "空命令";
//     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
//
//     NetEvent *event = create_test_event("5:     ", 7);
//
//     char *argv[10] = {NULL};
//     char *buffer = NULL;
//     unsigned argc = tokenize_command_zero_copy_(event, 0, 5, 1, argv, 10, &buffer);
//
//     assert(argc == 0);
//     printf("✅ %s: 验证成功，没有参数\n", TEST_NAME);
//
//     free(buffer);
//     free_test_events(event);
//     printf("=== 测试完成: %s ===\n", TEST_NAME);
// }
//
// // 测试用例8: 从非零偏移开始
// void test_non_zero_offset() {
//     const char *TEST_NAME = "从非零偏移开始";
//     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
//
//     NetEvent *event = create_test_event("xxxx7:GET key", 13);
//
//     char *argv[10] = {NULL};
//     char *buffer = NULL;
//     unsigned argc = tokenize_command_zero_copy_(event, 4, 7, 1, argv, 10, &buffer);
//
//     assert(argc == 2);
//     char *expected[] = {"GET", "key"};
//     verify_args(argv, expected, argc, TEST_NAME);
//
//     free(buffer);
//     free_test_events(event);
//     printf("=== 测试完成: %s ===\n", TEST_NAME);
// }
//
// // 测试用例9: 内存分配失败
// void test_memory_allocation_failure() {
//     // 这个测试可能需要模拟内存分配失败
//     // 在真实环境中不容易测试，可以通过代码检查确保处理了这种情况
//     printf("\n注意: 内存分配失败测试需要特殊环境，请通过代码审查确保正确处理\n");
// }
//
// // 测试用例10: 长字符串参数
// void test_long_string_args() {
//     const char *TEST_NAME = "长字符串参数";
//     printf("\n=== 开始测试: %s ===\n", TEST_NAME);
//
//     // 创建长参数字符串
//     char *long_cmd = NULL;
//     size_t cmd_len = 0;
//
//     // 准备命令前缀
//     const char *prefix = "500:SET ";
//     cmd_len += strlen(prefix);
//
//     // 创建长键名和值
//     char *long_key = malloc(200);
//     char *long_val = malloc(300);
//     memset(long_key, 'k', 199);
//     memset(long_val, 'v', 299);
//     long_key[199] = '\0';
//     long_val[299] = '\0';
//
//     cmd_len += strlen(long_key) + 1 + strlen(long_val); // +1为空格
//
//     // 分配命令缓冲区
//     long_cmd = malloc(cmd_len + 1);
//     sprintf(long_cmd, "%s%s %s", prefix, long_key, long_val);
//
//     // 分割成多个事件
//     NetEvent *e1 = create_test_event(long_cmd, 200);
//     NetEvent *e2 = create_test_event(long_cmd + 200, 200);
//     NetEvent *e3 = create_test_event(long_cmd + 400, cmd_len - 400);
//     e1->next = e2;
//     e2->next = e3;
//
//     // 解析命令
//     char *argv[10] = {NULL};
//     char *buffer = NULL;
//     unsigned argc = tokenize_command_zero_copy_(e1, 0, 500, strlen(prefix) - 1, argv, 10, &buffer);
//
//     // 验证结果
//     assert(argc == 3);
//     assert(strcmp(argv[0], "SET") == 0);
//     assert(strlen(argv[1]) == 199);
//     assert(strlen(argv[2]) == 299);
//     printf("✅ %s: 长参数验证成功\n", TEST_NAME);
//
//     // 清理
//     free(buffer);
//     free(long_cmd);
//     free(long_key);
//     free(long_val);
//     free_test_events(e1);
//     printf("=== 测试完成: %s ===\n", TEST_NAME);
// }
//
// // 主测试函数
// void test_tokenize_command_zero_copy() {
//     printf("===== 开始全面测试 tokenize_command_zero_copy_ =====\n");
//
//     // test_basic_command();
//     // test_multi_param_command();
//     test_extra_spaces_command();
//     // test_cross_event_command();
//     // test_param_cross_boundary();
//     // test_max_args_limit();
//     // test_empty_command();
//     // test_non_zero_offset();
//     // test_memory_allocation_failure(); // 需要特殊环境
//     // test_long_string_args();
//
//     printf("===== 全面测试完成 =====\n");
// }
//
//
// void test_bug_reproduction() {
//     printf("\n=== 开始测试: 复现cursor在事件末尾的bug ===\n");
//
//     // 创建FdBuffer结构
//     FdBuffer buffer;
//     memset(&buffer, 0, sizeof(FdBuffer));
//
//     // 创建两个事件，模拟你的调试输出中的情况
//     NetEvent *e1 = create_test_event("25:SET bench_key_2_0 value_0", 28);
//     NetEvent *e2 = create_test_event("25:SET bench_key_2_1 value_1", 28);
//     e1->next = e2;
//
//     // 设置buffer的初始状态，关键是让cursor正好在第一个事件的末尾
//     buffer.head = e1;
//     buffer.cursors_event = e1;
//     buffer.cursors_event_offset = 28; // 关键点：设置为事件的大小
//     buffer.len = 56; // 两个事件的总大小
//     buffer.offset = 28; // 已处理的数据量
//     buffer.ccl = 0; // 清空长度相关缓存
//     buffer.cccsl = 0;
//
//     // 打印初始状态
//     printf("初始状态:\n");
//     printf("  head=%p, cursors_event=%p, cursors_event_offset=%zu\n",
//            buffer.head, buffer.cursors_event, buffer.cursors_event_offset);
//     printf("  len=%zu, offset=%zu, ccl=%d, cccsl=%d\n",
//            buffer.len, buffer.offset, buffer.ccl, buffer.cccsl);
//
//     // 调用函数
//     size_t eventOffset, len, len_str_l;
//     NetEvent *result = paserfdbags_zero_copy(&buffer, &eventOffset, &len, &len_str_l);
//
//     // 打印结果
//     printf("解析结果:\n");
//     printf("  result=%p, eventOffset=%zu, len=%zu, len_str_l=%zu\n",
//            result, eventOffset, len, len_str_l);
//     printf("  新状态: cursors_event=%p, cursors_event_offset=%zu\n",
//            buffer.cursors_event, buffer.cursors_event_offset);
//
//     // 检查是否重现了bug
//     if (result && len == 0) {
//         printf("✅ 成功重现bug: 返回了非NULL事件但长度为0\n");
//     } else if (!result) {
//         printf("❓ 函数返回NULL\n");
//     } else {
//         printf("❓ 函数返回正常结果，没有重现bug\n");
//     }
//
//     // 如果成功获取到事件，尝试解析参数
//     if (result) {
//         char *argv[10] = {NULL};
//         char *buffer_ptr = NULL;
//         unsigned argc = tokenize_command_zero_copy_(result, eventOffset, len, len_str_l,
//                                                    argv, 10, &buffer_ptr);
//
//         printf("参数解析结果: argc=%u\n", argc);
//         for (unsigned i = 0; i < argc; i++) {
//             printf("  Arg[%u] = '%s'\n", i, argv[i]);
//         }
//
//         free(buffer_ptr);
//     }
//
//     // 清理
//     free_test_events(e1);
//     printf("=== 测试完成: 复现cursor在事件末尾的bug ===\n");
// }
//
//
// int main111() {
//     test_bug_reproduction();
//     return 0;
// }