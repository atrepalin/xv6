#ifndef KSERVER_H
#define KSERVER_H

#include "spinlock.h"
#include "http.h"

#define MSG_QUEUE_SIZE 8

struct request_queue {
    struct incoming_http_request requests[MSG_QUEUE_SIZE];
    int head;
    int tail;
    struct spinlock lock;
};

#endif