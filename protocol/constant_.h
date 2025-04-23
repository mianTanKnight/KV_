//
// Created by wenshen on 25-4-22.
//

#ifndef CONSTANT__H
#define CONSTANT__H
#include "../common.h"

#define MAX_STR_NUM_CONSTANT_LEN 99999
#define CHUNK_SIZE 6

/**
 * 一种常量的支持 或者说 O1 效率的快速转换 使用(空间换性能)
 * 它的生命周期和整个程序同步
 * 本质是一个字符串数组
 * ["0","1","2","3","4","5","6".... cap_size]
 * exp :
 *  fd -> 5
 *  getstrconstant(5) -> "5"
 *
 * 每个数字字符串都会以 6 char 的长度(最大-> MAX_STR_NUM_CONSTANT_LEN)
 * 会带以下好处
 * 更容易的分配连续空间
 * 更简单的offset 计算
 *
 * 会带来空间浪费 但它是值得的
 *
 */

int init_constants(size_t cap_size);

char *getstrconstant(size_t i);

void destroy_constants();

#endif //CONSTANT__H
