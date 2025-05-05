//
// Created by wenshen on 25-4-21.
//

#ifndef SLENPROTOCOL_H
#define SLENPROTOCOL_H
#include "../common.h"
#include "../server/server_.h"
#include "buffers_.h"
#include "constant_.h"
#include "slenprotocol.h"


/**
 * 基于长度的简单文本协议
 * <length>:<data>\n
 */

#define MAX_DATA_LEN 50 * 1024 * 1024 // max str len
#define MAX_DATA_LEN_NUMBER_STR_SIZE 10

/**
 * Client Api
 */
size_t slenpro(char* buffer, size_t data_size , size_t data_str_len);

/**
 *  Server Api
 **/

int neatenbags(NetEvent *curr_event, FdBuffer *buffer);

NetEvent *
paserfdbags_zero_copy(FdBuffer *bags, size_t *eventOffset, size_t *len, size_t *len_str_l);

#endif //SLENPROTOCOL_H
