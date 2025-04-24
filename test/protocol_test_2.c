// //
// // Created by wenshen on 25-4-22.
// //
// #include "../common.h"
// #include "../protocol/constant_.h"
// #include "../protocol/slenprotocol.h"
// #include "../server/server_.h"
//
//
// NetEvent *create_test_event(int fd, const char *data, size_t size) {
//     NetEvent *event = calloc(1, sizeof(NetEvent) + size * sizeof(char));
//     event->fd = fd;
//     memcpy(event->data, data, size);
//     event->size = size;
//     event->next = NULL;
//     return event;
// }
//
// void test_basic_functionality() {
//     printf("=== Test: Basic Functionality ===\n");
//
//     init_constants(100);
//     init_net_bags_manager(10);
//
//     // 创建一个简单的消息 "5:hello"
//     NetEvent *event = create_test_event(1, "5:hello", 7);
//     neatenbags(event);
//
//     char *result = NULL;
//     size_t len = paserfdbags(1, &result);
//
//     assert(len == 5);
//     assert(result != NULL);
//     assert(strncmp(result, "hello", 5) == 0);
//
//     printf("Basic test passed! Parsed: '%s' (length: %zu)\n", result, len);
//     free(result);
//
//     destroy_manager();
//     destroy_constants();
// }
//
// void test_fragmented_message() {
//     printf("\n=== Test: Fragmented Message ===\n");
//
//     init_constants(100);
//     init_net_bags_manager(10);
//
//     // 创建一个被分成3个片段的消息
//     NetEvent *event1 = create_test_event(2, "11:he", 5);
//     NetEvent *event2 = create_test_event(2, "llo w", 5);
//     NetEvent *event3 = create_test_event(2, "orld", 4);
//
//     neatenbags(event1);
//
//     char *result = NULL;
//     int len = paserfdbags(2, &result);
//     assert(len == 0); // 消息不完整，应返回0
//     printf("After first fragment: Not enough data (as expected)\n");
//
//     neatenbags(event2);
//     len = paserfdbags(2, &result);
//     assert(len == 0); // 消息仍不完整
//     printf("After second fragment: Not enough data (as expected)\n");
//
//     neatenbags(event3);
//     len = paserfdbags(2, &result);
//     assert(len == 11);
//     assert(result != NULL);
//     assert(strncmp(result, "hello world", 10) == 0);
//
//     printf("Fragmented test passed! Parsed: '%s' (length: %zu)\n", result, len);
//     free(result);
//
//     destroy_manager();
//     destroy_constants();
// }
//
// void test_multiple_messages() {
//     printf("\n=== Test: Multiple Consecutive Messages ===\n");
//
//     init_constants(100);
//     init_net_bags_manager(10);
//
//     // 创建包含两个连续消息的事件: "5:hello7:world!!"
//     NetEvent *event = create_test_event(3, "5:hello7:world!!", strlen("5:hello7:world!!"));
//     neatenbags(event);
//
//     // 解析第一条消息
//     char *result1 = NULL;
//     int len1 = paserfdbags(3, &result1);
//     assert(len1 == 5);
//     assert(result1 != NULL);
//     assert(strncmp(result1, "hello", 5) == 0);
//
//     printf("First message parsed: '%s' (length: %zu)\n", result1, len1);
//     free(result1);
//
//     // 解析第二条消息
//     char *result2 = NULL;
//     int len2 = paserfdbags(3, &result2);
//     assert(len2 == 7);
//     assert(result2 != NULL);
//     assert(strncmp(result2, "world!!", 7) == 0);
//
//     printf("Second message parsed: '%s' (length: %zu)\n", result2, len2);
//     free(result2);
//
//     destroy_manager();
//     destroy_constants();
// }
//
// void test_invalid_format() {
//     printf("\n=== Test: Invalid Format ===\n");
//
//     init_constants(100);
//     init_net_bags_manager(10);
//
//     // 创建一个格式无效的消息 (缺少冒号)
//     NetEvent *event = create_test_event(4, "invalid_no_colon", 16);
//     neatenbags(event);
//
//     char *result = NULL;
//     int len = paserfdbags(4, &result);
//
//     // 应该返回错误
//     assert(len == -1);
//     assert(result == NULL);
//
//     printf("Invalid format test passed! Returned error as expected\n");
//
//     destroy_manager();
//     destroy_constants();
// }
//
// // 测试边界情况 - 零长度消息
// void test_zero_length_message() {
//     printf("\n=== Test: Zero Length Message ===\n");
//
//     init_constants(100);
//     init_net_bags_manager(10);
//
//     // 创建零长度消息 "0:"
//     NetEvent *event = create_test_event(5, "0:", 2);
//     neatenbags(event);
//
//     char *result = NULL;
//     int len = paserfdbags(5, &result);
//
//     assert(len == 0);
//     assert(result != NULL); // 应该分配了一个空字符串
//
//     printf("Zero length message test passed!\n");
//     if (result) free(result);
//
//     destroy_manager();
//     destroy_constants();
// }
//
// // 测试长度前缀异常 - 长度值过大
// void test_excessive_length() {
//     printf("\n=== Test: Excessive Length Value ===\n");
//
//     init_constants(100);
//     init_net_bags_manager(10);
//
//     // 创建一个声明长度非常大但实际数据较少的消息
//     NetEvent *event = create_test_event(6, "9999:short_data", 16);
//     neatenbags(event);
//
//     char *result = NULL;
//     int len = paserfdbags(6, &result);
//
//     // 应该返回0表示数据不足
//     assert(len == 0);
//     assert(result == NULL);
//
//     printf("Excessive length test passed! Correctly detected insufficient data\n");
//
//     destroy_manager();
//     destroy_constants();
// }
//
// void test_colon_at_boundary() {
//     printf("\n=== Test: Colon At Event Boundary ===\n");
//
//     init_constants(100);
//     init_net_bags_manager(10);
//
//     // 创建两个事件，冒号恰好在边界上
//     NetEvent *event1 = create_test_event(7, "5", 1);
//     NetEvent *event2 = create_test_event(7, ":hello", 6);
//
//     neatenbags(event1);
//     neatenbags(event2);
//
//     char *result = NULL;
//     int len = paserfdbags(7, &result);
//
//     // 检查是否正确解析
//     if (len > 0 && result != NULL) {
//         printf("Colon at boundary test: Parsed '%s' (length: %zu)\n", result, len);
//         free(result);
//     } else {
//         printf("Colon at boundary test: Failed to parse message\n");
//     }
//
//     destroy_manager();
//     destroy_constants();
// }
//
// void test_resource_management() {
//     printf("\n=== Test: Resource Management ===\n");
//
//     init_constants(100);
//     init_net_bags_manager(10);
//
//     // 添加多个消息
//     NetEvent *event1 = create_test_event(8, "3:one", 5);
//     NetEvent *event2 = create_test_event(8, "5:two!!", 7);
//     NetEvent *event3 = create_test_event(8, "4:last", 6);
//
//     neatenbags(event1);
//     neatenbags(event2);
//     neatenbags(event3);
//
//     // 解析所有消息
//     char *result;
//     int msg_count = 0;
//
//     while (1) {
//         result = NULL;
//         int len = paserfdbags(8, &result);
//         if (len == 0 || len == -1 || result == NULL) break;
//
//         // printf("Message %d: '%s' (length: %zu)\n", ++msg_count, result, len);
//         free(result);
//     }
//
//     // 关闭并销毁连接
//     close_fd(8);
//     destroy_fd(8);
//
//     printf("Resource management test: Processed %d messages and cleaned up\n", msg_count);
//
//     destroy_manager();
//     destroy_constants();
// }
//
//  // 测试极端情况 - 非常小的分段
// void test_tiny_fragments() {
//     printf("\n=== Test: Tiny Fragments ===\n");
//
//     init_constants(100);
//     init_net_bags_manager(10);
//
//     // 将消息"10:1234567890"分成极小的片段
//     const char *fragments[] = {"1", "0", ":", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
//     int num_fragments = sizeof(fragments) / sizeof(fragments[0]);
//
//     for (int i = 0; i < num_fragments; i++) {
//         NetEvent *event = create_test_event(9, fragments[i], strlen(fragments[i]));
//         neatenbags(event);
//
//         // 每添加一个片段尝试解析
//         char *result = NULL;
//         int len = paserfdbags(9, &result);
//
//         if (len > 0 && result != NULL) {
//             printf("After fragment %d: Successfully parsed '%s'\n", i + 1, result);
//             free(result);
//             break;
//         }
//         if (i == num_fragments - 1) {
//             printf("Failed to parse message even after all fragments\n");
//         }
//     }
//     destroy_manager();
//     destroy_constants();
// }
//
//
// void test_tiny_fragments_merging() {
//     printf("\n=== Test: Tiny Fragments ===\n");
//
//     init_constants(100);
//     init_net_bags_manager(10);
//
//     // 将消息"10:1234567890"分成极小的片段
//     const char *fragments[] = {"1", "0", ":", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
//     int num_fragments = sizeof(fragments) / sizeof(fragments[0]);
//
//     for (int i = 0; i < num_fragments; i++) {
//         NetEvent *event = create_test_event(9, fragments[i], strlen(fragments[i]));
//         neatenbags(event);
//     }
//     char *result = NULL;
//     int len = paserfdbags(9, &result);
//     printf("Successfully parsed %d %s\n", len, result);
//
//     if (len == 10  && strcmp(result,"1234567890") == 0) {
//         printf("test_tiny_fragments_merging Successfully parsed\n");
//     } else {
//         printf("test_tiny_fragments_merging Failed\n");
//     }
//     destroy_manager();
//     destroy_constants();
// }
//
// int main() {
//     printf("Starting comprehensive tests for paserfdbags function\n");
//
//     test_basic_functionality();
//     test_fragmented_message();
//     test_multiple_messages();
//     test_invalid_format();
//     test_zero_length_message();
//     test_excessive_length();
//     test_colon_at_boundary();
//     test_resource_management();
//     test_tiny_fragments();
//     test_tiny_fragments_merging();
//
//     printf("\nAll tests completed!\n");
//     return 0;
// }
//
//
// void test_slenpro() {
//     char *result;
//     const char *test_data = "Hello World";
//     size_t len = slenpro(test_data, strlen(test_data), &result);
//
//     // 应该返回 "11:Hello World" 格式的字符串
//     printf("slenpro result: %s (length: %zu)\n", result, len);
//     free(result);  // 记得释放内存
// }
//
// void test_getstrconstant() {
//     init_constants(100);  // 初始化常量池
//     for (int i = 0; i < 100; i++) {
//         char *str = getstrconstant(i);
//         printf("Constant %d = %s\n", i, str);
//     }
//
//     // 测试边界情况
//     char *beyond = getstrconstant(MAX_STR_NUM_CONSTANT_LEN + 1);
//     printf("Beyond max: %s\n", beyond ? beyond : "NULL as expected");
//
//     destroy_constants();
// }
//
//
// void test_buffer_management() {
//     init_net_bags_manager(10);
//     init_constants(100);
//
//     // 创建模拟事件
//     NetEvent *e1 = create_test_event(1, "5:Hello", 7);
//     NetEvent *e2 = create_test_event(1, " World", 6);
//
//     // 添加到管理器
//     int res1 = neatenbags(e1);
//     int res2 = neatenbags(e2);
//
//     printf("Add event1: %d, Add event2: %d\n", res1, res2);
//
//     // 测试解析
//     char *data;
//     size_t len = paserfdbags(1, &data);
//     if (len > 0) {
//         printf("Parsed data: %s (length: %zu)\n", data, len);
//         free(data);
//     } else {
//         printf("No complete message yet\n");
//     }
//
//     destroy_manager();
//     destroy_constants();
// }
//
