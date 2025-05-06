//
// Created by wenshen on 25-4-27.
//

#include "reply_.h"


Reply *reply = NULL;


void
*reply_data(void *arg) {
    Reply *reply = arg;
    while (p_running) {
        CommandResponse *head_ = NULL;
        pthread_mutex_lock(&reply->queue_mutex);
        // 使用while循环防止虚假唤醒
        while (reply->count == 0 && p_running) {
            pthread_cond_wait(&reply->queue_cond, &reply->queue_mutex);
        }
        head_ = reply->head;
        reply->head = reply->tail = NULL;
        reply->count = 0;
        pthread_mutex_unlock(&reply->queue_mutex);

        //reply
        while (head_) {
            CommandResponse *next = head_->next;
            const char *response_data;
            size_t response_length;
            if (strcmp("OK\n", head_->msg) == 0) {
                if (head_->data == NULL) {
                    response_data = "null";
                    response_length = 4; // "empty"长度
                } else {
                    response_data = head_->data;
                    response_length = strlen(head_->data);
                }
            } else {
                response_data = head_->msg;
                response_length = strlen(head_->msg);
            }
            uint32_t network_length = htonl((uint32_t)response_length);
            if (write(head_->fd, &network_length, sizeof(network_length)) != sizeof(network_length)) {
                LOG_ERROR("Failed to write length prefix: %s", strerror(errno));
            }
            if (write(head_->fd, response_data, response_length) != response_length) {
                LOG_ERROR("Failed to write response data: %s", strerror(errno));
            }
            free_response(head_);
            head_ = next;
        }
    }
    return NULL;
}

void stop_reply() {
    if (reply) {
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
