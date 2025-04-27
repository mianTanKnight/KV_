// //
// // Created by wenshen on 25-4-21.
// //
// #include <assert.h>
//
// #include "../protocol/slenprotocol.h"
// #include "../common.h"
// #include "../math.h"
//
// bool parse_command(FdBuffer *buffer, char **cmd_out, size_t *cmd_len) {
//     if (!buffer || buffer->len == 0) {
//         return false;
//     }
//
//     // 处理数据
//     char *data = malloc(buffer->len + 1);
//     if (!data) {
//         return false;
//     }
//
//     // 从事件链表中收集连续数据
//     size_t total_copied = 0;
//     NetEvent *curr = buffer->head;
//     while (curr && total_copied < buffer->len) {
//         size_t to_copy = curr->size - buffer->offset > buffer->len - total_copied
//                              ? buffer->len - total_copied
//                              : curr->size - buffer->offset;
//         memcpy(data + total_copied, curr->data + buffer->offset, to_copy);
//         total_copied += to_copy;
//
//         if (to_copy + buffer->offset == curr->size) {
//             // 当前事件已完全处理
//             buffer->offset = 0;
//             curr = curr->next;
//         } else {
//             // 当前事件部分处理
//             buffer->offset += to_copy;
//         }
//     }
//
//     // 寻找长度字段和分隔符
//     char *colon = memchr(data, ':', total_copied);
//     if (!colon) {
//         free(data);
//         return false; // 数据不完整
//     }
//
//     // 解析长度
//     *colon = '\0';
//     size_t cmd_length = atoi(data);
//     *colon = ':';
//
//     size_t header_len = (colon - data) + 1;
//
//     // 检查是否有足够的数据
//     if (total_copied < header_len + cmd_length) {
//         free(data);
//         return false; // 数据不完整
//     }
//
//     // 提取命令
//     *cmd_out = malloc(cmd_length + 1);
//     if (!*cmd_out) {
//         free(data);
//         return false;
//     }
//
//     memcpy(*cmd_out, data + header_len, cmd_length);
//     (*cmd_out)[cmd_length] = '\0';
//     *cmd_len = cmd_length;
//
//     // 更新缓冲区状态
//     buffer->len -= (header_len + cmd_length);
//
//     // 更新事件链和偏移量
//     // 这部分需要更复杂的逻辑，类似于之前的收集数据过程
//
//     free(data);
//     return true;
// }
//
// NetEvent *create_mock_event(int fd, const char *data, size_t len) {
//     // 分配内存（包括事件结构和数据部分）
//     NetEvent *event = calloc(1, sizeof(NetEvent) + len);
//     if (!event) {
//         return NULL;
//     }
//
//     // 初始化事件
//     event->fd = fd;
//     event->size = len;
//     event->next = NULL;
//     event->type = 1; // 假设1表示数据事件
//
//     // 复制数据
//     memcpy(event->data, data, len);
//
//     return event;
// }
//
// void test_protocol_parsing() {
//     // 创建管理器init_net_bags_manager(10);
//
//     // 准备测试数据 - 两个完整命令
//     char *cmd1 = "SET key1 value1";
//     char *cmd2 = "GET key2";
//
//     // 编码命令
//     char *encoded1, *encoded2;
//     size_t len1 = slenpro(cmd1, strlen(cmd1), &encoded1);
//     size_t len2 = slenpro(cmd2, strlen(cmd2), &encoded2);
//
//     // 模拟拆包情况 - 将两个命令分成三段发送
//     // 1. cmd1的一部分
//     // 2. cmd1的剩余部分 + cmd2的一部分
//     // 3. cmd2的剩余部分
//
//     size_t split1 = 5; // cmd1的前5个字节
//     size_t split2 = len1 - 5 + 4; // cmd1的剩余部分 + cmd2的前4个字节
//     size_t split3 = len2 - 4; // cmd2的剩余部分
//
//     // 创建三个网络事件
//     int mock_fd = 100; // 模拟文件描述符
//
//     NetEvent *event1 = create_mock_event(mock_fd, encoded1, split1);
//     NetEvent *event2 = create_mock_event(mock_fd, encoded1 + split1, split2);
//     NetEvent *event3 = create_mock_event(mock_fd, encoded2 + 4, split3);
//
//     // 将事件添加到管理器
//     neatenbags(event1);
//     neatenbags(event2);
//     neatenbags(event3);
//
//     // 获取这个fd的缓冲区
//     char fd_str[20];
//     sprintf(fd_str, "%d", mock_fd);
//     // FdBuffer *buffer = get(manager->fds_bag_tabel, fd_str);
//
//     // 现在解析缓冲区中的数据，应该能提取出两个完整命令
//     // char *parsed_cmd;
//     // size_t parsed_len;
//
//     // 应该能解析出第一个命令
//     // bool result1 = parse_command(buffer, &parsed_cmd, &parsed_len);
//     // assert(result1 == true);
//     // assert(parsed_len == strlen(cmd1));
//     // assert(memcmp(parsed_cmd, cmd1, parsed_len) == 0);
//     //
//     // // 应该能解析出第二个命令
//     // bool result2 = parse_command(buffer, &parsed_cmd, &parsed_len);
//     // assert(result2 == true);
//     // assert(parsed_len == strlen(cmd2));
//     // assert(memcmp(parsed_cmd, cmd2, parsed_len) == 0);
//     //
//     // // 缓冲区应该已空
//     // bool result3 = parse_command(buffer, &parsed_cmd, &parsed_len);
//     // assert(result3 == false);
//
//     // 清理资源
//     free(encoded1);
//     free(encoded2);
//     destroy_manager();
//
//     printf("Protocol parsing test passed!\n");
// }
//
// void test_performance() {
//     clock_t start, end;
//     double cpu_time_used;
//
//     // 创建一个较大的管理器
//     start = clock();
//     init_net_bags_manager(1000);
//     end = clock();
//     cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
//     printf("创建管理器用时: %f 秒\n", cpu_time_used);
//
//     // 准备测试数据
//     char *cmd = "SET key1 value1";
//     char *encoded;
//     size_t len = slenpro(cmd, strlen(cmd), &encoded);
//
//     // 测试大量连接的性能
//     start = clock();
//     const int NUM_CONNECTIONS = 1000;
//     for (int fd = 1; fd <= NUM_CONNECTIONS; fd++) {
//         NetEvent *event = create_mock_event(fd, encoded, len);
//         neatenbags(event);
//     }
//     end = clock();
//     cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
//     printf("创建 %d 个连接用时: %f 秒\n", NUM_CONNECTIONS, cpu_time_used);
//
//     // 测试单个连接大量事件的性能
//     start = clock();
//     const int NUM_EVENTS = 10000;
//     for (int i = 0; i < NUM_EVENTS; i++) {
//         NetEvent *event = create_mock_event(1, encoded, len);
//         neatenbags(event);
//     }
//     end = clock();
//     cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
//     printf("对单个连接添加 %d 个事件用时: %f 秒\n", NUM_EVENTS, cpu_time_used);
//
//     // 测试资源清理性能
//     start = clock();
//     destroy_manager();
//     end = clock();
//     cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
//     printf("销毁管理器用时: %f 秒\n", cpu_time_used);
//
//     free(encoded);
//     printf("性能测试完成\n");
// }
//
//
// // int main() {
// //     test_performance();
// // }
