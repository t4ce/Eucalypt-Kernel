#pragma once

#include <stdint.h>
#include <mem.h>

extern int rdrand64(uint64_t *out);

static inline void get_random_bytes(void *buf, size_t len) {
    uint8_t *p = buf;
    while (len >= 8) {
        uint64_t val;
        while (!rdrand64(&val));
        memcpy(p, &val, 8);
        p   += 8;
        len -= 8;
    }
    if (len) {
        uint64_t val;
        while (!rdrand64(&val));
        memcpy(p, &val, len);
    }
}