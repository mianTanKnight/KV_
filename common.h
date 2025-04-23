//
// Created by wenshen on 25-4-2.
//

#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <stdbool.h>


/**********--inline--************/

static char *get_current_time() {
    static char time_str[64];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    return time_str;
}

/*********--Macro--*************/

#define s_to_zero(s) do { \
            memset(&(s), 0, sizeof(s)); \
            } while(0)

#define p_to_zero(p,s) do { \
            memset(p, 0, s); \
            } while(0)

#define LOG_DEBUG(format, ...) do { \
fprintf(stdout, "[DEBUG][%s:%d][%s][%s] " format "\n", \
__FILE__, __LINE__, __func__, get_current_time(), ##__VA_ARGS__); \
} while(0)

#define LOG_INFO(format, ...) do { \
fprintf(stdout, "[INFO][%s:%d][%s][%s] " format "\n", \
__FILE__, __LINE__, __func__, get_current_time(), ##__VA_ARGS__); \
} while(0)

#define LOG_ERROR(format, ...) do { \
fprintf(stderr, "[ERROR][%s:%d][%s][%s] " format "\n", \
__FILE__, __LINE__, __func__, get_current_time(), ##__VA_ARGS__); \
} while(0)

#define EXIT_SUCCESS_()  do { \
    exit(EXIT_SUCCESS) ; \
} while(0)

#define EXIT_ERROR() do { \
   exit(EXIT_FAILURE) ; \
} while (0)


#endif //COMMON_H
