#include <stdlib.h>
#include "zset.h"
#include <stddef.h>
#include <string>

enum {
    SER_NIL = 0, //'NULL'
    SER_ERR = 1, //Error code and message
    SER_STR = 2, //string
    SER_INT = 3, //int64
    SER_DBL = 4, //double
    SER_ARR = 5, //array
};

#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type, member) );})

uint64_t str_hash(const uint8_t *data, size_t len);
bool str2dbl(const std::string &s, double &out);
bool str2int(const std::string &s, int64_t &out);
uint32_t min(size_t lhs, size_t rhs);