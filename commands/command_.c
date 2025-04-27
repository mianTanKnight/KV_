//
// Created by wenshen on 25-3-31.
//

#include "command_.h"

Command commands[] = {
    {"SET", cmd_set, 2, 3, "SET key value [expire]", "设置键值对"},
    {"GET", cmd_get, 1, 1, "GET key", "获取键对应的值"},
    {"DEL", cmd_del, 1, 1, "DEL key", "删除键值对"},
    {"EXPIRE", cmd_expire, 2, 2, "EXPIRE key expire", "设置键的过期时间"},
    {"EXIT", cmd_exit, 1, 1, "EXIT", "退出程序"}
};


static long
isNumeric(const char *str);

long
isNumeric(const char *str) {
    if (str == NULL || *str == '\0') {
        return -2;
    }
    char *endptr;
    errno = 0;
    long num = strtol(str, &endptr, 10);
    if (*endptr != '\0') {
        return -3;
    }
    if (errno == ERANGE || num > INT_MAX || num < INT_MIN) {
        return -4;
    }
    return num;
}

unsigned
tokenize_command(char *line, char **argv, int max_args) {
    unsigned size = 0;
    if (!line)
        return size;

    int bounds[max_args][2];
    for (int i = 0; i < max_args; i++) {
        bounds[i][0] = 0;
        bounds[i][1] = 0;
    }

    int offset_ = 0;
    int b_index = -1;
    char prev = -1;
    while (line[offset_] != '\0') {
        if (line[offset_] != ' ') {
            if (prev == ' ' || prev == -1) {
                b_index++;
                if (b_index >= max_args) break; // 防止超出数组范围
                bounds[b_index][0] = offset_;
            }
        } else if (prev != ' ' && b_index >= 0) {
            bounds[b_index][1] = offset_ - 1;
        }
        prev = line[offset_];
        offset_++;
    }

    // 只有当找到至少一个参数时才更新最后一个参数的结束位置
    if (b_index >= 0 && prev != ' ') {
        bounds[b_index][1] = offset_ - 1;
    }

    size = b_index + 1 > max_args ? max_args : b_index + 1;

    for (int i = 0; i < size; i++) {
        int len = bounds[i][1] - bounds[i][0] + 1;
        char *arg = calloc(len + 1, sizeof(char));
        if (arg == NULL) {
            return i;
        }
        for (int j = 0; j < len; j++) {
            arg[j] = line[bounds[i][0] + j];
        }
        arg[len] = '\0';
        argv[i] = arg;
    }

    return size;
}


CommandResponse
*create_response(void *v, ResponseType type, const char *message) {
    CommandResponse *response = malloc(sizeof(CommandResponse));
    if (!response) {
        LOG_ERROR("out of memory : %s", strerror(errno));
        EXIT_ERROR();
    }
    response->type = type;
    response->msg = strdup(message);
    response->data = v;
    return response;
}

// SET key value [expire]
CommandResponse
*cmd_set(int argc, const char **argv, HashTable *table) {
    if (argc < 3 || argc > 4)
        return create_response(NULL, RESP_INVALID_ARGS, "argc num error");

    long expire = 0;
    if (argc == 4) {
        long num = isNumeric(argv[3]);
        if (num < 0) {
            return create_response(NULL, RESP_INVALID_ARGS, "expire num error");
        }
        expire = num;
    }
    if (!put(table, argv[1], (void *) argv[2], expire)) {
        return create_response(NULL, RESP_SERVER_ERROR, "server error");
    }
    return create_response("ok", RESP_OK, "OK\n");
}

// GET key
CommandResponse
*cmd_get(int argc, const char **argv, HashTable *table) {
    if (argc != 2)
        return create_response(NULL, RESP_INVALID_ARGS, "argc num error");
    return create_response(get(table, argv[1]), RESP_OK, "OK\n");
}

// DEL key
CommandResponse
*cmd_del(int argc, char **argv, HashTable *table) {
    if (argc != 2)
        return create_response(NULL, RESP_INVALID_ARGS, "argc num error");
    remove_(table, argv[1]);
    return create_response("ok", RESP_OK, "OK\n");
}

// EXPIRE key
CommandResponse
*cmd_expire(int argc, char **argv, HashTable *table) {
    if (argc != 3)
        return create_response(NULL, RESP_INVALID_ARGS, "argc num error");
    long expire_ = isNumeric(argv[2]);
    if (expire_ < 0) {
        return create_response(NULL, RESP_INVALID_ARGS, "expire num error");
    }
    expire(table, argv[1], expire_);
    return create_response("ok", RESP_OK, "OK\n");
}

// EXIT
CommandResponse
*cmd_exit(int argc, char **argv, HashTable *table) {
    if (argc != 2)
        return create_response(NULL, RESP_INVALID_ARGS, "argc num error");
    clear(table);
    return create_response("ok", RESP_OK, "OK\n");
}


Command *match(const char *name) {
    switch (name[0]) {
        case 'S':
        case 's':
            return &commands[0]; // SET

        case 'G':
        case 'g':
            return &commands[1]; // GET

        case 'D':
        case 'd':
            return &commands[2]; // DEL

        case 'E':
        case 'e': {
            if (name[1] == 'X' || name[1] == 'x')
                return &commands[4]; // EXIT
            return &commands[3]; // EXPIRE
        }
        default:
            return NULL;
    }
}


void
free_response(CommandResponse *response) {
    if (response) {
        // don't free v ,The ownership of V is table
        free(response->msg);
        free(response);
    }
}
