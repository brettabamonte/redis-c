#pragma once

#include <stdlib.h>
#include "zset.h"
#include <stddef.h>
#include <string>

uint64_t str_hash(const uint8_t *data, size_t len) {
    uint32_t h = 0x811C9DC5;
    for(size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x10000193;
    }

    return h;
}

enum {
    SER_NIL = 0, //'NULL'
    SER_ERR = 1, //Error code and message
    SER_STR = 2, //string
    SER_INT = 3, //int64
    SER_ARR = 4, //array
};

#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type, member) );})


static bool str2dbl(const std::string &s, double &out);