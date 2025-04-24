// //
// // Created by wenshen on 25-4-21.
// //
// #include "../server/server_.h"
// #include "../core/k_v.h"
// #include "../commands/command_.h"
// #define PORT 6379
// #define BACKLOG 10
// #define MAX_CONNECTIONS 100
// // Net-Server
// NetServerContext *net_server_context = NULL;
// HashTable hash_table;
// HashTable *table = &hash_table;
//
// volatile int running = 1;
//
// // 信号处理函数
// void handle_signal(int sig) {
//     printf("Received signal %d, shutting down...\n", sig);
//     running = 0;
// }
//
// int process_events(NetServerContext *context) {
//     if (context->status < 0) {
//         return -1;
//     }
//     NetEvent *events = NULL;
//     size_t count = getEvents(context, &events);
//
//     if (count > 0) {
//         printf("Processing %zu events\n", count);
//         NetEvent *current = events;
//         while (current) {
//             NetEvent *next = current->next;
//             // 显示接收到的数据
//             printf("Received from fd=%d: %.*s\n", current->fd, (int) current->size, current->data);
//             char *argv[10] = {NULL};
//             unsigned argc = tokenize_command(current->data, argv, 10);
//             Command *command = match(argv[0]);
//             CommandResponse *command_response = command->handler(argc, argv, table);
//             printf("Processing command: %s\n", argv[1]);
//
//             if (strcmp("OK", command_response->msg) == 0) {
//                 write(current->fd, command_response->data == NULL ? "empty" : command_response->data,
//                       command_response->data == NULL ? 6 : strlen(command_response->data));
//             } else {
//                 write(current->fd, command_response->msg, strlen(command_response->msg));
//             }
//             free_response(command_response);
//             for (int i = 0; i < argc; i++) {
//                 free(argv[i]);
//             }
//             destroy_event(current);
//             current = next;
//         }
//     } else {
//         pthread_mutex_lock(&context->queue_mutex);
//         pthread_cond_wait(&context->queue_cond, &context->queue_mutex);
//         pthread_mutex_unlock(&context->queue_mutex);
//     }
//     return 0;
// }
//
//
// // int main() {
// //     // 设置信号处理
// //     signal(SIGINT, handle_signal);
// //     signal(SIGTERM, handle_signal);
// //
// //     // 创建服务器上下文
// //     net_server_context = create_net_server_context(PORT, BACKLOG, MAX_CONNECTIONS);
// //     if (!net_server_context) {
// //         fprintf(stderr, "Failed to create server context\n");
// //         return 1;
// //     }
// //     init(&hash_table, 5, free);
// //     printf("Server started on port %d\n", PORT);
// //     while (running) {
// //         process_events(net_server_context);
// //     }
// //
// //     // 清理资源
// //     destroy_net_server_context(net_server_context);
// //     clear(table);
// //
// //     printf("Server shutdown complete\n");
// // }
