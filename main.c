#include "common.h"
#include "core/k_v.h"
#include "server/server_.h"
#include "protocol/slenprotocol.h"
#include "commands/command_.h"

// Net-Server
NetServerContext *net_server_context = NULL;
HashTable hash_table;
HashTable *k_v_table = &hash_table;

volatile int running = 1;

void handle_signal(int sig) {
    printf("Received signal %d, shutting down...\n", sig);
    running = 0;
}

#define DEFAULT_PORT 7000
#define DEFAULT_BACKLOG 1024
#define DEFAULT_MAX_CONNECTIONS 10000

void parser_start_args(int argc, char **argv, int *port, int *backlog, int *max_connections) {
}

int process_events(NetServerContext *context) {
    if (context->status < 0) {
        return -1;
    }
    NetEvent *events = NULL;
    size_t count = getEvents(context, &events);
    char *argv[10] = {NULL};

    if (count > 0) {
        NetEvent *current = events;
        while (current) {
            NetEvent *next = current->next;
            if (current->type == 2) {
                //fd close event
                destroy_fd(current->fd);
                destroy_event(current);
                goto next;
            }
            if (neatenbags(current) < 0) {
                fprintf(stderr, "Error while processing events %s\n ", strerror(errno));
                continue;
            }
            //优先处理当下current 最有可能就绪
            char *data = NULL;
            int size = paserfdbags(current->fd, &data);
            if (size == -1) {
                fprintf(stderr, "Error paserfdbags processing events %s\n ", strerror(errno));
                goto next;
            }
            if (size <= 0) {
                if (data) free(data);
                goto next;
            }
            unsigned argc = tokenize_command(data, argv, 10);
            Command *command = match(argv[0]);

            if (!command) {
                fprintf(stderr, "The command is illegal. %s\n ", argv[0]);
                continue;
            }
            CommandResponse *command_response = command->handler(argc, argv, k_v_table);
            if (strcmp("OK\n", command_response->msg) == 0) {
                // 暂时的实现
                write(current->fd, command_response->data == NULL ? "empty" : command_response->data,
                      command_response->data == NULL ? 6 : strlen(command_response->data));
            } else {
                write(current->fd, command_response->msg, strlen(command_response->msg));
            }
            free_response(command_response);
            for (int i = 0; i < argc; i++) {
                free(argv[i]);
            }
            memset(argv, 0, sizeof(char) * 10);
        next:
            current = next;
        }
    } else {
        pthread_mutex_lock(&context->queue_mutex);
        pthread_cond_wait(&context->queue_cond, &context->queue_mutex);
        pthread_mutex_unlock(&context->queue_mutex);
    }
    return 0;
}

void eixt_(void) {
}

/**
 *
 *
 */
int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    int backlog = DEFAULT_BACKLOG;
    int max_connections = DEFAULT_MAX_CONNECTIONS;

    parser_start_args(argc, argv, &port, &backlog, &max_connections);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    net_server_context = create_net_server_context(port, backlog, max_connections);
    init_net_bags_manager(backlog);
    if (!net_server_context) {
        fprintf(stderr, "Failed to create server context\n");
        return 1;
    }
    init(&hash_table, 5, free);
    fprintf(stdout, "Server started port: %d\n", port);

    while (running) {
        if (process_events(net_server_context)) {
            fprintf(stderr, "Failed to process events %s \n", strerror(errno));
            fprintf(stderr, "Exiting...\n");
            break;
        }
    }
    eixt_();
}
