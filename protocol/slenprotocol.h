//
// Created by wenshen on 25-4-21.
//

#ifndef SLENPROTOCOL_H
#define SLENPROTOCOL_H
#include "../common.h"
#include "../server/server_.h"
#include "buffers_.h"
/**
 * 基于长度的简单文本协议
 * <length>:<data>\n
 */

#define MAX_DATA_LEN 50 * 1024 * 1024 // max str len
#define MAX_DATA_LEN_NUMBER_STR_SIZE 10

/**
 * Client Api
 */
size_t slenpro(const char *data, size_t data_len, char **pro_str);


/**
 *  Server Api
 **/

int neatenbags(NetEvent *curr_event, FdBuffer *buffer);

int paserfdbags(FdBuffer *bags, char **rd);


#endif //SLENPROTOCOL_H
