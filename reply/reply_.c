//
// Created by wenshen on 25-4-27.
//

#include "reply_.h"


Reply *reply = NULL;

volatile int running_rep_i = 1;

static void reply_data(void *d) {
    Reply *reply = d;
    while (running_rep_i) {
        CommandResponse *head_ = NULL;
        pthread_mutex_lock(&reply->queue_mutex);
        // 使用while循环防止虚假唤醒
        while (reply->count == 0) {
            pthread_cond_wait(&reply->queue_cond, &reply->queue_mutex);
        }
        head_ = reply->head;
        reply->head = reply->tail = NULL;
        reply->count = 0;
        pthread_mutex_unlock(&reply->queue_mutex);

        //reply
        while (head_) {
            CommandResponse *next = head_->next;
            if (strcmp("OK\n", head_->msg) == 0) {
                // 暂时的实现
                write(head_->fd, head_->data == NULL ? "empty" : head_->data,
                      head_->data == NULL ? 6 : strlen(head_->data));
            } else {
                write(head_->fd, head_->msg, strlen(head_->msg));
            }
            free_response(head_);
            head_ = next;
        }
    }
}

void stop_reply() {
    if (reply) {
        running_rep_i = 0;
        pthread_mutex_lock(&reply->queue_mutex);
        pthread_cond_signal(&reply->queue_cond);
        pthread_mutex_unlock(&reply->queue_mutex);

        pthread_join(reply->event_thread, NULL);
        CommandResponse *current = reply->head;
        while (current) {
            CommandResponse *next = current->next;
            free_response(current);
            current = next;
        }

        pthread_mutex_destroy(&reply->queue_mutex);
        pthread_cond_destroy(&reply->queue_cond);
        free(reply);
        reply = NULL;
    }
}

int r_enqueue_(CommandResponse *h, CommandResponse *t, int count) {
    if (!reply || !h || !t || count <= 0)
        return 0;

    pthread_mutex_lock(&reply->queue_mutex);
    if (reply->count == 0) {
        reply->head = h;
        reply->tail = t;
    } else {
        reply->tail->next = h;
        reply->tail = t;
    }
    t->next = NULL;
    reply->count += count;
    pthread_cond_signal(&reply->queue_cond);
    pthread_mutex_unlock(&reply->queue_mutex);
    return 1;
}

int create_reply() {
    Reply *init = calloc(1, sizeof(Reply));
    if (!init) {
        fprintf(stderr, "reply_init: out of memory\n");
        return -1;
    }

    if (pthread_mutex_init(&init->queue_mutex, NULL)) {
        LOG_ERROR("Failed to initialize mutex for NetServerContext %s\n", strerror(errno));
        pthread_mutex_destroy(&init->queue_mutex);
        free(init);
        return -1;
    }
    if (pthread_cond_init(&init->queue_cond, NULL)) {
        LOG_ERROR("Failed to initialize condition variable for NetServerContext %s\n", strerror(errno));
        pthread_cond_destroy(&init->queue_cond);
        free(init);
        return -1;
    }

    if (pthread_create(&init->event_thread, NULL, reply_data, init) != 0) {
        LOG_ERROR("Failed to create event thread: %s", strerror(errno));
        pthread_mutex_destroy(&init->queue_mutex);
        pthread_cond_destroy(&init->queue_cond);
        free(init);
        return -1;
    }

    // init queue
    init->head = init->tail = NULL;
    init->count = 0;
    reply = init;

    return 0;
}
