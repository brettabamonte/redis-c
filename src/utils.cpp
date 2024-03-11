#include <string>
#include <stdlib.h>

bool str2dbl(const std::string &s, double &out) {
    char *endp = NULL;
    out = strtod(s.c_str(), &endp);
    return endp == s.c_str() + s.size() && !isnan(out);
}

bool str2int(const std::string &s, int64_t &out) {
    char *endp = NULL;
    out = strtoll(s.c_str(), &endp, 10);
    return endp == s.c_str() + s.size();
}

uint64_t str_hash(const uint8_t *data, size_t len) {
    uint32_t h = 0x811C9DC5;
    for(size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x10000193;
    }

    return h;
}

uint32_t min(size_t lhs, size_t rhs) {
    return lhs < rhs ? lhs : rhs;
}