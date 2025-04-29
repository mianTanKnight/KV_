#include "common.h"
#include "core/k_v.h"
#include "server/server_.h"
#include "protocol/slenprotocol.h"
#include "commands/command_.h"
#include "protocol/buffers_.h"
#include "reply_/reply_.h"

// Net-Server
NetServerContext *net_server_context = NULL;
HashTable hash_table;
HashTable *k_v_table = &hash_table;

volatile int running = 1;

void clear__() {
    clear(k_v_table);
    stop_reply();
    destroy_net_server_context(net_server_context);
    free(net_server_context);
    free(k_v_table);
}

void handle_signal(int sig) {
    clear__();
    printf("Received signal %d, shutting down...\n", sig);
    running = 0;
}

#define DEFAULT_PORT 7000
#define DEFAULT_BACKLOG 1024
#define DEFAULT_MAX_CONNECTIONS 10000

void parser_start_args(int argc, char **argv, int *port, int *backlog, int *max_connections) {
}


void dis_all_event(NetEvent *head) {
    while (head) {
        NetEvent *next = head->next;
        destroy_event(&head);
        head = next;
    }
}

int process_events(NetServerContext *context) {
    if (context->status < 0) {
        fprintf(stderr, "Net context status -1 \n");
        return -1;
    }
    NetEvent *current = NULL;
    getEvents(context, &current);
    char *argv[10] = {NULL};

    CommandResponse *head = NULL;
    CommandResponse *tail = NULL;
    int count = 0;

    while (current) {
        NetEvent *next = current->next;
        if (current->type == 2) {
            FdBuffer *fdnetbuffer = discharge(current->fd);
            if (fdnetbuffer) {
                dis_all_event(fdnetbuffer->head);
                free(fdnetbuffer);
            }
            destroy_event(&current);
            current = next;
        } else {
            FdBuffer *bags = get_(current->fd);
            if (bags) {
                if (neatenbags(current, bags) < 0) {
                    fprintf(stderr, "Error while processing events %s\n ", strerror(errno));
                    goto next_;
                }
                size_t eventOffset, len, str_len;
                NetEvent *event = paserfdbags_zero_copy(bags, &eventOffset, &len, &str_len);
                if (!event) {
                    goto next_;
                }
                char *buffer = NULL;
                unsigned argc = tokenize_command_zero_copy_(event, eventOffset, len, str_len, argv, 10, &buffer);
                if (!argv[0]) {
                    fprintf(stderr, "Error while processing events of parser tokenize_command_zero_copy_  %s\n ",
                            strerror(errno));
                    goto next_;
                }
                Command *command = match(argv[0]);
                if (!command) {
                    fprintf(stderr, "Error while processing events of parser command  %s\n ", strerror(errno));
                    goto next_;
                }
                CommandResponse *command_response = command->handler(argc, argv, k_v_table);
                command_response->fd = current->fd;
                if (tail) {
                    tail->next = command_response;
                    tail = command_response;
                } else {
                    head = tail = command_response;
                }
                count++;
                command_response->next = NULL;
            next_:
                if (buffer) free(buffer);
                current = next;
            }
        }
    }
    return r_enqueue_(head, tail, count);
}


int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    int backlog = DEFAULT_BACKLOG;
    int max_connections = DEFAULT_MAX_CONNECTIONS;

    parser_start_args(argc, argv, &port, &backlog, &max_connections);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    net_server_context = create_net_server_context(port, backlog, max_connections);
    if (!net_server_context) {
        fprintf(stderr, "Failed to create server context\n");
        exit(EXIT_FAILURE);
    }
    if (init_fd_buffers() < 0) {
        fprintf(stderr, "Failed to create buffers \n");
        exit(EXIT_FAILURE);
    }
    init(&hash_table, 5, free);
    fprintf(stdout, "Server started port: %d\n", port);

    create_reply();

    while (running) {
        if (process_events(net_server_context) < 0) {
            fprintf(stderr, "Failed to process events %s \n", strerror(errno));
            fprintf(stderr, "Exiting...\n");
            break;
        }
    }
    clear__();
}
