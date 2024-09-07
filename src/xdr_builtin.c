#include <string.h>

static inline uint32_t
xdr_hton32(uint32_t value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap32(value);
#else
    return value;
#endif
}

static inline uint32_t
xdr_ntoh32(uint32_t value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap32(value);
#else
    return value;
#endif
}

static inline uint64_t
xdr_hton64(uint64_t value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap64(value);
#else
    return value;
#endif
}

static inline uint64_t
xdr_ntoh64(uint64_t value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap64(value);
#else
    return value;
#endif
}

struct xdr_cursor {
    const xdr_iovec *cur;
    const xdr_iovec *last;
    unsigned int            offset;
};

static inline void
xdr_cursor_init(
    struct xdr_cursor *cursor,
    const xdr_iovec *iov,
    int niov)
{
    cursor->cur    = iov;
    cursor->last   = iov + (niov - 1);
    cursor->offset = 0;
}

static inline int
xdr_cursor_extract(
    struct xdr_cursor *cursor,
    void *out,
    unsigned int bytes)
{
    unsigned int done, chunk;

    if (cursor->offset + bytes <= xdr_iovec_len(cursor->cur)) {
        memcpy(out, xdr_iovec_data(cursor->cur) + cursor->offset, bytes);
        cursor->offset += bytes;
    } else {

        done = 0;

        while (done < bytes) {

            chunk = xdr_iovec_len(cursor->cur) - cursor->offset;

            if (chunk > bytes - done) {
                chunk = bytes - done;
            }

            if (chunk) {
                memcpy(out + done,
                       xdr_iovec_data(cursor->cur) + cursor->offset,
                       chunk);
            }

            done += chunk;

            if (done < bytes && cursor->cur == cursor->last) {
                return -1;
            }

            cursor->cur++;
            cursor->offset = 0;
        }
    }

    return bytes;
}

static inline int
xdr_cursor_append(
    struct xdr_cursor *cursor,
    const void *in,
    unsigned int bytes)
{
    unsigned int done, chunk;

    if (cursor->offset + bytes <= xdr_iovec_len(cursor->cur)) {
        memcpy(xdr_iovec_data(cursor->cur) + cursor->offset, in, bytes);
        cursor->offset += bytes;
    } else {

        done = 0;

        while (done < bytes) {

            chunk = xdr_iovec_len(cursor->cur) - cursor->offset;

            if (chunk > bytes - done) {
                chunk = bytes - done;
            }

            if (chunk) {
                memcpy(xdr_iovec_data(cursor->cur) + cursor->offset,
                       in + done,
                       chunk);
            }

            done += chunk;

            if (done < bytes && cursor->cur == cursor->last) {
                return -1;
            }

            cursor->cur++;
            cursor->offset = 0;
        }
    }

    return bytes;
}

static inline int
__marshall_uint32_t(
    const uint32_t *v,
    int n,
    struct xdr_cursor *cursor)
{
    uint32_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {
        tmp = xdr_hton32(v[i]);
        rc = xdr_cursor_append(cursor, &tmp, 4);

        if (rc < 0) return rc;
    }        

    return n << 2;
}

static inline int
__unmarshall_uint32_t(
    uint32_t *v,
    int n,
    struct xdr_cursor *cursor)
{
    uint32_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {

        rc = xdr_cursor_extract(cursor, &tmp, 4);

        if (rc < 0) return rc;

        v[i] = xdr_ntoh32(tmp);
    }

    return n << 2;
}

static inline int
__marshall_int32_t(
    const int32_t *v,
    int n,
    struct xdr_cursor *cursor)
{
    int32_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {
        tmp = xdr_hton32(v[i]);
        rc = xdr_cursor_append(cursor, &tmp, 4);

        if (rc < 0) return rc;
    }

    return n << 2;
}

static inline int
__unmarshall_int32_t(
    int32_t *v,
    int n,
    struct xdr_cursor *cursor)
{
    int32_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {

        rc = xdr_cursor_extract(cursor, &tmp, 4);

        if (rc < 0) return rc;

        v[i] = xdr_ntoh32(tmp);
    }

    return n << 2;
}

static inline int
__marshall_uint64_t(
    const uint64_t *v,
    int n,
    struct xdr_cursor *cursor)
{
    uint64_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {
        tmp = xdr_hton64(v[i]);
        rc = xdr_cursor_append(cursor, &tmp, 8);

        if (rc < 0) return rc;
    }

    return n << 3;
}

static inline int
__unmarshall_uint64_t(
    uint64_t *v,
    int n,
    struct xdr_cursor *cursor)
{
    uint64_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {

        rc = xdr_cursor_extract(cursor, &tmp, 8);

        if (rc < 0) return rc;

        v[i] = xdr_ntoh64(tmp);
    }

    return n << 3;
}

static inline int
__marshall_int64_t(
    const int64_t *v,
    int n,
    struct xdr_cursor *cursor)
{
    int64_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {
        tmp = xdr_hton64(v[i]);
        rc = xdr_cursor_append(cursor, &tmp, 8);

        if (rc < 0) return rc;
    }

    return n << 3;
}


static inline int
__unmarshall_int64_t(
    int64_t *v,
    int n,
    struct xdr_cursor *cursor)
{
    int64_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {

        rc = xdr_cursor_extract(cursor, &tmp, 8);

        if (rc < 0) return rc;

        v[i] = xdr_ntoh64(tmp);
    }

    return n << 3;
}
static inline int
__marshall_struct_xdr_iovec(
    xdr_iovec *v,
    int n,
    struct xdr_cursor *cursor)
{
    return 0;
}

static inline int
__unmarshall_struct_xdr_iovec(
    xdr_iovec *v,
    int n,
    struct xdr_cursor *cursor)
{
    return 0;
}
