//
// Created by wenshen on 25-4-27.
//

#ifndef REPLY__H
#define REPLY__H
#include "../commands/command_.h"
#include "../common.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

typedef struct reply {
    CommandResponse *head;
    CommandResponse *tail;
    volatile size_t count;

    pthread_t event_thread;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
} Reply;

int create_reply();

int r_enqueue_(CommandResponse *h, CommandResponse *t, int count);

void stop_reply();


#endif //REPLY__H
