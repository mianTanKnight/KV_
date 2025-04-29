//
// Created by wenshen on 25-4-22.
//

#ifndef CONSTANT__H
#define CONSTANT__H
#include "../common.h"
#include "slenprotocol.h"

#define MAX_STR_NUM_CONSTANT_LEN 99999
#define CHUNK_SIZE 6

/**
 * o1
 * 0 -> "0", 1 -> "1"
 */

int init_constants(size_t cap_size);
char *getstrconstant(size_t i);
void destroy_str_constants();

#endif //CONSTANT__H
