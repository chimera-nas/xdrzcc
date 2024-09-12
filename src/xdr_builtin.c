#include <stdlib.h>
#include <string.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define unlikely(x)    __builtin_expect(!!(x), 0) 
#define FORCE_INLINE __attribute__((always_inline)) inline

xdr_dbuf *
xdr_dbuf_alloc(void)
{
    xdr_dbuf *dbuf;

    dbuf = malloc(sizeof(*dbuf));

    dbuf->buffer = malloc(4096);

    dbuf->used = 0;
    dbuf->size = 4096;

    return dbuf;

}

void
xdr_dbuf_free(xdr_dbuf *dbuf)
{
    free(dbuf->buffer);
    free(dbuf);
}


static FORCE_INLINE uint32_t
xdr_hton32(uint32_t value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap32(value);
#else
    return value;
#endif
}

static FORCE_INLINE uint32_t
xdr_ntoh32(uint32_t value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap32(value);
#else
    return value;
#endif
}

static FORCE_INLINE uint64_t
xdr_hton64(uint64_t value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap64(value);
#else
    return value;
#endif
}

static FORCE_INLINE uint64_t
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

static FORCE_INLINE void
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
xdr_cursor_skip(
    struct xdr_cursor *cursor,
    unsigned int bytes)
{
    unsigned int done, chunk;

    if (cursor->offset + bytes <= xdr_iovec_len(cursor->cur)) {
        cursor->offset += bytes;
    } else {

        done = 0;

        while (done < bytes) {

            chunk = xdr_iovec_len(cursor->cur) - cursor->offset;

            if (chunk > bytes - done) {
                chunk = bytes - done;
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

static FORCE_INLINE int
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

        if (unlikely(rc < 0)) return rc;
    }        

    return n << 2;
}

static FORCE_INLINE int
__unmarshall_uint32_t(
    uint32_t *v,
    int n,
    struct xdr_cursor *cursor,
    xdr_dbuf *dbuf)
{
    uint32_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {

        rc = xdr_cursor_extract(cursor, &tmp, 4);

        if (unlikely(rc < 0)) return rc;

        v[i] = xdr_ntoh32(tmp);
    }

    return n << 2;
}

static FORCE_INLINE int
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

        if (unlikely(rc < 0)) return rc;
    }

    return n << 2;
}

static FORCE_INLINE int
__unmarshall_int32_t(
    int32_t *v,
    int n,
    struct xdr_cursor *cursor,
    xdr_dbuf *dbuf)
{
    int32_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {

        rc = xdr_cursor_extract(cursor, &tmp, 4);

        if (unlikely(rc < 0)) return rc;

        v[i] = xdr_ntoh32(tmp);
    }

    return n << 2;
}

static FORCE_INLINE int
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

        if (unlikely(rc < 0)) return rc;
    }

    return n << 3;
}

static FORCE_INLINE int
__unmarshall_uint64_t(
    uint64_t *v,
    int n,
    struct xdr_cursor *cursor,
    xdr_dbuf *dbuf)
{
    uint64_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {

        rc = xdr_cursor_extract(cursor, &tmp, 8);

        if (unlikely(rc < 0)) return rc;

        v[i] = xdr_ntoh64(tmp);
    }

    return n << 3;
}

static FORCE_INLINE int
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

        if (unlikely(rc < 0)) return rc;
    }

    return n << 3;
}


static FORCE_INLINE int
__unmarshall_int64_t(
    int64_t *v,
    int n,
    struct xdr_cursor *cursor,
    xdr_dbuf *dbuf)
{
    int64_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {

        rc = xdr_cursor_extract(cursor, &tmp, 8);

        if (unlikely(rc < 0)) return rc;

        v[i] = xdr_ntoh64(tmp);
    }

    return n << 3;
}

static FORCE_INLINE int
__marshall_xdr_string(
    const xdr_string *str,
    int n,
    struct xdr_cursor *cursor)
{
    const uint32_t zero = 0;
    int i, rc, pad, len = 0;

    for (i = 0; i < n; ++i) {

        rc = __marshall_uint32_t(&str[i].len, 1, cursor);

        if (unlikely(rc < 0)) return rc;

        len += rc;

        rc = xdr_cursor_append(cursor, str[i].str, str[i].len);

        len += str[i].len;

        if (unlikely(rc < 0)) return rc;

        pad = (4 - (len & 0x3)) & 0x3;

        if (pad) {
            rc = xdr_cursor_append(cursor, &zero, pad);

            if (unlikely(rc < 0)) return rc;

            len += rc;
        }
    }

    return len;
}

static FORCE_INLINE int
__unmarshall_xdr_string(
    xdr_string *str,
    int n,
    struct xdr_cursor *cursor,
    xdr_dbuf *dbuf)
{
    int i, rc, pad, len = 0;

    for (i = 0; i < n; ++i) {

        rc = __unmarshall_uint32_t(&str[i].len, 1, cursor, dbuf);

        if (unlikely(rc < 0)) return rc;

        len += rc;

        str[i].str = dbuf->buffer + dbuf->used;

        dbuf->used += str[i].len + 1;

        rc = xdr_cursor_extract(cursor, str[i].str, str[i].len);

        if (unlikely(rc < 0)) return rc;

        str[i].str[str[i].len] = '\0';

        len += str[i].len;

        pad = (4 - (str[i].len & 0x3)) & 0x3;

        if (pad) {
            rc = xdr_cursor_skip(cursor, pad);

            if (unlikely(rc < 0)) return rc;
    
            len += rc;
        }
    }
 
    return len; 
}

static FORCE_INLINE int
__marshall_opaque_fixed(
    const xdr_iovecr *v,
    uint32_t size,
    struct xdr_cursor *cursor)
{

    xdr_cursor_skip(cursor, size);

    return 0;
}

static FORCE_INLINE int
__unmarshall_opaque_fixed(
    xdr_iovecr *v,
    uint32_t size,
    struct xdr_cursor *cursor,
    xdr_dbuf *dbuf)
{
    v->length = size;
    v->iov = cursor->cur;
    v->niov = (cursor->last - cursor->cur) + 1;
    v->offset = cursor->offset;

    xdr_cursor_skip(cursor, v->length);

    return size;
}

static FORCE_INLINE int
__marshall_opaque_variable(
    const xdr_iovecr *v,
    uint32_t bound,
    struct xdr_cursor *cursor)
{
    int rc;

    rc = __marshall_uint32_t(&v->length, 1, cursor);

    if (unlikely(rc < 0)) return rc;

    xdr_cursor_skip(cursor, v->length);

    return 4 + v->length;
}

static FORCE_INLINE int
__unmarshall_opaque_variable(
    xdr_iovecr *v,
    uint32_t bound,
    struct xdr_cursor *cursor,
    xdr_dbuf *dbuf)
{
    int rc;

    rc = __unmarshall_uint32_t(&v->length, 1, cursor, dbuf);

    if (unlikely(rc < 0)) return rc;

    v->iov = cursor->cur;
    v->niov = (cursor->last - cursor->cur) + 1;
    v->offset = cursor->offset;

    xdr_cursor_skip(cursor, v->length);

    return 4 + v->length;
}
