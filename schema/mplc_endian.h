#ifndef MPLC_ENDIAN_H
#define MPLC_ENDIAN_H

#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define MPLC_BE16(v) __builtin_bswap16(v)
#define MPLC_BE32(v) __builtin_bswap32(v)
#define MPLC_BE64(v) __builtin_bswap64(v)
#elif defined(_MSC_VER)
#include <stdlib.h>
#define MPLC_BE16(v) _byteswap_ushort(v)
#define MPLC_BE32(v) _byteswap_ulong(v)
#define MPLC_BE64(v) _byteswap_uint64(v)
#else
#define MPLC_BE16(v) (v)
#define MPLC_BE32(v) (v)
#define MPLC_BE64(v) (v)
#endif

#if defined(MPLC_BIG_ENDIAN) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define MPLC_LE16(v) MPLC_BE16(v)
#define MPLC_LE32(v) MPLC_BE32(v)
#define MPLC_LE64(v) MPLC_BE64(v)
#else
#define MPLC_LE16(v) (v)
#define MPLC_LE32(v) (v)
#define MPLC_LE64(v) (v)
#endif

static inline uint16_t mplc_read_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static inline uint32_t mplc_read_le32(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static inline uint64_t mplc_read_le64(const uint8_t *p)
{
    uint64_t lo = mplc_read_le32(p);
    uint64_t hi = mplc_read_le32(p + 4);
    return lo | (hi << 32);
}

#endif /* MPLC_ENDIAN_H */
