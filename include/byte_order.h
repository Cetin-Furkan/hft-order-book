#ifndef BYTE_ORDER_H
#define BYTE_ORDER_H

#include <stdint.h>
#include <arpa/inet.h> // For htonl/htons

// Most modern Linux systems (glibc) have htobe64/be64toh in <endian.h>
// but for portability, it's safer to provide our own fallback.
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

static inline uint64_t htonll(uint64_t x) {
    return ((uint64_t)htonl((uint32_t)(x & 0xFFFFFFFF)) << 32) | htonl((uint32_t)(x >> 32));
}

static inline uint64_t ntohll(uint64_t x) {
    return ((uint64_t)ntohl((uint32_t)(x & 0xFFFFFFFF)) << 32) | ntohl((uint32_t)(x >> 32));
}

#else // System is already big-endian, functions do nothing.

static inline uint64_t htonll(uint64_t x) {
    return x;
}

static inline uint64_t ntohll(uint64_t x) {
    return x;
}

#endif

#endif // BYTE_ORDER_H
