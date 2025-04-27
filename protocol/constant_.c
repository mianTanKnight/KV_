//
// Created by wenshen on 25-4-22.
//

#include "constant_.h"
char *constants_ = NULL;
size_t constants_length_ = 0;


int
init_constants(size_t cap_size) {
    if (cap_size == 0 || cap_size > MAX_STR_NUM_CONSTANT_LEN) {
        fprintf(stderr, "Error: Invalid capacity size %zu requested.\n", cap_size);
        return -1;
    }
    constants_length_ = cap_size;

    char *constants = malloc(CHUNK_SIZE * (sizeof(char) * cap_size));
    memset(constants, '\0', CHUNK_SIZE * (sizeof(char) * cap_size));

    char *ch = constants;
    for (size_t i = 0; i < cap_size; i++) {
        // 1. 转换并直接获取长度
        int written = snprintf(ch, CHUNK_SIZE, "%zu", i);
        if (written < 0 || (size_t) written >= CHUNK_SIZE) {
            fprintf(stderr, "snprintf error or truncation for index %zu!\n", i);
            free(constants);
            constants_ = NULL;
            return -1;
        }
        ch += CHUNK_SIZE;
    }
    constants_ = constants;
    return 0;
}


char
*getstrconstant(size_t index) {
    if (!constants_) return NULL;
    if (index > constants_length_) {
        return NULL;
    }
    return constants_ + index * CHUNK_SIZE;
}

void
destroy_str_constants() {
    free(constants_);
    constants_ = NULL;
}
