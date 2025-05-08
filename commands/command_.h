//
// Created by wenshen on 25-3-31.
//

#ifndef COMMAND__H
#define COMMAND__H
#include "../core/k_v.h"
#include "../common.h"
#include "../protocol/slenprotocol.h"

typedef enum {
    RESP_OK,
    RESP_ERROR,
    RESP_NULL,
    RESP_INVALID_ARGS,
    RESP_SERVER_ERROR
} ResponseType;

typedef struct CommandResponse {
    ResponseType type;
    void *data;
    char *msg;
    int fd;
    struct CommandResponse *next;
} CommandResponse;

typedef CommandResponse * (*CommandHandler)(int argc, char **argv, HashTable *table);

typedef struct Command {
    const char *name;
    CommandHandler handler;
    int min_args, max_args;
    const char *usage;
    const char *description;
    const size_t name_len;
} Command;

unsigned tokenize_command(char *line, char **argv, int max_args);

unsigned tokenize_command_zero_copy_(NetEvent *event, size_t event_offset, size_t len, size_t len_str_l,
                                     char **argv, int max_args, char **buffer_out);

unsigned
tokenize_command_line_(NetEvent *event,
                       size_t event_offset,
                       size_t data_len,
                       size_t len_str_l,
                       char **argv,
                       int max_args,
                       char **buffer_out);

CommandResponse *create_response(void *v, ResponseType type, const char *message);

CommandResponse *cmd_set(int argc, const char **argv, HashTable *table);

CommandResponse *cmd_get(int argc, const char **argv, HashTable *table);

CommandResponse *cmd_del(int argc, char **argv, HashTable *table);

CommandResponse *cmd_expire(int argc, char **argv, HashTable *table);

CommandResponse *cmd_exit(int argc, char **argv, HashTable *table);

void free_response(CommandResponse *response);

extern Command commands[];

Command *match(const char *name);


#endif //COMMAND__H
