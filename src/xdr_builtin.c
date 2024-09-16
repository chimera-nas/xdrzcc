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

static FORCE_INLINE int 
xdr_iovec_add_offset(xdr_iovec *iov, int offset)
{
    if (unlikely(xdr_iovec_len(iov) <= offset)) {
        return -1;
    }

    xdr_iovec_set_len(iov, xdr_iovec_len(iov) - offset);
    xdr_iovec_set_data(iov, xdr_iovec_data(iov) + offset);

    return 0;
}

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

struct xdr_read_cursor {
    const xdr_iovec *cur;
    const xdr_iovec *last;
    unsigned int            offset;
};

struct xdr_write_cursor {
    xdr_iovec              *cur;
    xdr_iovec              *first;
    xdr_iovec              *last;
    const xdr_iovec        *scratch_iov;
    const xdr_iovec        *scratch_last;
    unsigned int            size;
};

static FORCE_INLINE void
xdr_read_cursor_init(
    struct xdr_read_cursor *cursor,
    const xdr_iovec *iov,
    int niov)
{
    cursor->cur    = iov;
    cursor->last   = iov + (niov - 1);
    cursor->offset = 0;
}

static FORCE_INLINE void
xdr_write_cursor_init(
    struct xdr_write_cursor *cursor,
    const xdr_iovec *scratch_iov,
    int scratch_niov,
    xdr_iovec *out_iov,
    int out_niov)
{
    cursor->scratch_iov = scratch_iov;
    cursor->scratch_last = scratch_iov + (scratch_niov - 1);
    cursor->cur    = out_iov;
    cursor->first  = out_iov;
    cursor->last   = out_iov + (out_niov - 1);

    xdr_iovec_set_data(cursor->cur, xdr_iovec_data(cursor->scratch_iov));
    xdr_iovec_copy_private(cursor->cur, cursor->scratch_iov);
    xdr_iovec_set_len(cursor->cur, 0);

    cursor->size = xdr_iovec_len(cursor->scratch_iov);

    cursor->scratch_iov++;

}

static FORCE_INLINE int
xdr_write_cursor_finish(
    struct xdr_write_cursor *cursor)
{
    if (cursor->cur > cursor->first && xdr_iovec_len(cursor->cur) == 0) {
        --cursor->cur; 
    }

    return (cursor->cur - cursor->first) + 1;
}


static inline int
xdr_read_cursor_extract(
    struct xdr_read_cursor *cursor,
    void *out,
    unsigned int bytes)
{
    unsigned int done, chunk;

    if (cursor->offset + bytes <= xdr_iovec_len(cursor->cur)) {
        memcpy(out, xdr_iovec_data(cursor->cur) + cursor->offset, bytes);
        cursor->offset += bytes;

        if (cursor->offset == xdr_iovec_len(cursor->cur)) {
            cursor->cur++;
            cursor->offset = 0;
        }

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
xdr_write_cursor_append(
    struct xdr_write_cursor *cursor,
    const void *in,
    unsigned int bytes)
{
    unsigned int done, chunk;

    if (bytes <= cursor->size - xdr_iovec_len(cursor->cur)) {
        memcpy(xdr_iovec_data(cursor->cur) + xdr_iovec_len(cursor->cur), in, bytes);
        xdr_iovec_set_len(cursor->cur, xdr_iovec_len(cursor->cur) + bytes);
    } else {

        done = 0;

        while (done < bytes) {

            chunk = cursor->size - xdr_iovec_len(cursor->cur);

            if (chunk > bytes - done) {
                chunk = bytes - done;
            }

            if (chunk) {
                memcpy(xdr_iovec_data(cursor->cur) + xdr_iovec_len(cursor->cur),
                       in + done,
                       chunk);
            }

            done += chunk;

            xdr_iovec_set_len(cursor->cur, xdr_iovec_len(cursor->cur) + chunk);

            if (done < bytes && (cursor->cur == cursor->last || cursor->scratch_iov == cursor->scratch_last)) {
                return -1;
            }

            cursor->cur++;

            xdr_iovec_set_data(cursor->cur, xdr_iovec_data(cursor->scratch_iov));
            xdr_iovec_copy_private(cursor->cur, cursor->scratch_iov);
            xdr_iovec_set_len(cursor->cur, 0);
            cursor->size = xdr_iovec_len(cursor->scratch_iov);
            cursor->scratch_iov++;

        }
    }

    return bytes;
}

static inline int
xdr_read_cursor_skip(
    struct xdr_read_cursor *cursor,
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
    struct xdr_write_cursor *cursor)
{
    uint32_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {
        tmp = xdr_hton32(v[i]);
        rc = xdr_write_cursor_append(cursor, &tmp, 4);

        if (unlikely(rc < 0)) return rc;
    }        

    return n << 2;
}

static FORCE_INLINE int
__unmarshall_uint32_t(
    uint32_t *v,
    int n,
    struct xdr_read_cursor *cursor,
    xdr_dbuf *dbuf)
{
    uint32_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {

        rc = xdr_read_cursor_extract(cursor, &tmp, 4);

        if (unlikely(rc < 0)) return rc;

        v[i] = xdr_ntoh32(tmp);
    }

    return n << 2;
}

static FORCE_INLINE int
__marshall_int32_t(
    const int32_t *v,
    int n,
    struct xdr_write_cursor *cursor)
{
    int32_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {
        tmp = xdr_hton32(v[i]);
        rc = xdr_write_cursor_append(cursor, &tmp, 4);

        if (unlikely(rc < 0)) return rc;
    }

    return n << 2;
}

static FORCE_INLINE int
__unmarshall_int32_t(
    int32_t *v,
    int n,
    struct xdr_read_cursor *cursor,
    xdr_dbuf *dbuf)
{
    int32_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {

        rc = xdr_read_cursor_extract(cursor, &tmp, 4);

        if (unlikely(rc < 0)) return rc;

        v[i] = xdr_ntoh32(tmp);
    }

    return n << 2;
}

static FORCE_INLINE int
__marshall_uint64_t(
    const uint64_t *v,
    int n,
    struct xdr_write_cursor *cursor)
{
    uint64_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {
        tmp = xdr_hton64(v[i]);
        rc = xdr_write_cursor_append(cursor, &tmp, 8);

        if (unlikely(rc < 0)) return rc;
    }

    return n << 3;
}

static FORCE_INLINE int
__unmarshall_uint64_t(
    uint64_t *v,
    int n,
    struct xdr_read_cursor *cursor,
    xdr_dbuf *dbuf)
{
    uint64_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {

        rc = xdr_read_cursor_extract(cursor, &tmp, 8);

        if (unlikely(rc < 0)) return rc;

        v[i] = xdr_ntoh64(tmp);
    }

    return n << 3;
}

static FORCE_INLINE int
__marshall_int64_t(
    const int64_t *v,
    int n,
    struct xdr_write_cursor *cursor)
{
    int64_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {
        tmp = xdr_hton64(v[i]);
        rc = xdr_write_cursor_append(cursor, &tmp, 8);

        if (unlikely(rc < 0)) return rc;
    }

    return n << 3;
}


static FORCE_INLINE int
__unmarshall_int64_t(
    int64_t *v,
    int n,
    struct xdr_read_cursor *cursor,
    xdr_dbuf *dbuf)
{
    int64_t tmp;
    int i, rc;

    for (i = 0; i < n; ++i) {

        rc = xdr_read_cursor_extract(cursor, &tmp, 8);

        if (unlikely(rc < 0)) return rc;

        v[i] = xdr_ntoh64(tmp);
    }

    return n << 3;
}

static FORCE_INLINE int
__marshall_float(
    const float *v,
    int n,
    struct xdr_write_cursor *cursor)
{
    return xdr_write_cursor_append(cursor, v, n << 2);
}

static FORCE_INLINE int
__unmarshall_float(
    float *v,
    int n,
    struct xdr_read_cursor *cursor,
    xdr_dbuf *dbuf)
{
    return xdr_read_cursor_extract(cursor, v, n << 2);
}

static FORCE_INLINE int
__marshall_double(
    const double *v,
    int n,
    struct xdr_write_cursor *cursor)
{
    return xdr_write_cursor_append(cursor, v, n << 3);
}

static FORCE_INLINE int
__unmarshall_double(
    double *v,
    int n,
    struct xdr_read_cursor *cursor,
    xdr_dbuf *dbuf)
{
    return xdr_read_cursor_extract(cursor, v, n << 3);
}

static FORCE_INLINE int
__marshall_xdr_string(
    const xdr_string *str,
    int n,
    struct xdr_write_cursor *cursor)
{
    const uint32_t zero = 0;
    int i, rc, pad, len = 0;

    for (i = 0; i < n; ++i) {

        rc = __marshall_uint32_t(&str[i].len, 1, cursor);

        if (unlikely(rc < 0)) return rc;

        len += rc;

        rc = xdr_write_cursor_append(cursor, str[i].str, str[i].len);

        len += str[i].len;

        if (unlikely(rc < 0)) return rc;

        pad = (4 - (len & 0x3)) & 0x3;

        if (pad) {
            rc = xdr_write_cursor_append(cursor, &zero, pad);

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
    struct xdr_read_cursor *cursor,
    xdr_dbuf *dbuf)
{
    int i, rc, pad, len = 0;

    for (i = 0; i < n; ++i) {

        rc = __unmarshall_uint32_t(&str[i].len, 1, cursor, dbuf);

        if (unlikely(rc < 0)) return rc;

        len += rc;

        str[i].str = dbuf->buffer + dbuf->used;

        dbuf->used += str[i].len + 1;

        rc = xdr_read_cursor_extract(cursor, str[i].str, str[i].len);

        if (unlikely(rc < 0)) return rc;

        str[i].str[str[i].len] = '\0';

        len += str[i].len;

        pad = (4 - (str[i].len & 0x3)) & 0x3;

        if (pad) {
            rc = xdr_read_cursor_skip(cursor, pad);

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
    struct xdr_write_cursor *cursor)
{
    const uint32_t zero = 0;
    xdr_iovec *prev = cursor->cur;
    int i, rc, pad, left = v->length;

    if (size == 0) return 0;

    if (xdr_iovec_len(cursor->cur)) {

        if (unlikely(cursor->cur == cursor->last)) {
            return -1;
        }

        cursor->cur++;
    }

    for (i = 0; i < v->niov && left; ++i) {
        *cursor->cur = v->iov[i];

        if (i == 0 && v->offset) {
            rc = xdr_iovec_add_offset(cursor->cur, v->offset);

            if (unlikely(rc)) return rc;
        }

        if (xdr_iovec_len(cursor->cur) > left) {
            xdr_iovec_set_len(cursor->cur, left);
            left = 0;
        } else {
            left -= xdr_iovec_len(cursor->cur);
        }
    }

    if (unlikely(left)) {
        return -1;
    }

    if (unlikely(cursor->cur == cursor->last)) {
        return -1;
    }

    cursor->cur++;

    xdr_iovec_set_data(cursor->cur, xdr_iovec_data(prev) + xdr_iovec_len(prev));
    xdr_iovec_copy_private(cursor->cur, prev);
    xdr_iovec_set_len(cursor->cur, 0);

    pad = (4 - (size & 0x3)) & 0x3;

    if (pad) {
        rc = xdr_write_cursor_append(cursor, &zero, pad);

        if (unlikely(rc < 0)) return rc;
    }

    return size + pad;
}

static FORCE_INLINE int
__unmarshall_opaque_fixed(
    xdr_iovecr *v,
    uint32_t size,
    struct xdr_read_cursor *cursor,
    xdr_dbuf *dbuf)
{
    int pad, left = size;

    v->length = size;
    v->iov = cursor->cur;
    v->niov = 0;

    while (left > 0) {
        left -= xdr_iovec_len(&v->iov[v->niov]);
        v->niov++;
    }

    v->offset = cursor->offset;

    pad = (4 - (size & 0x3)) & 0x3;

    xdr_read_cursor_skip(cursor, size + pad);

    return size + pad;
}

static FORCE_INLINE int
__marshall_opaque_variable(
    const xdr_iovecr *v,
    uint32_t bound,
    struct xdr_write_cursor *cursor)
{
    int rc;

    rc = __marshall_uint32_t(&v->length, 1, cursor);

    if (unlikely(rc < 0)) return rc;

    rc = __marshall_opaque_fixed(v, v->length, cursor);

    if (unlikely(rc < 0)) return rc;

    return 4 + rc;
}

static FORCE_INLINE int
__unmarshall_opaque_variable(
    xdr_iovecr *v,
    uint32_t bound,
    struct xdr_read_cursor *cursor,
    xdr_dbuf *dbuf)
{
    int rc;
    uint32_t size;

    rc = __unmarshall_uint32_t(&size, 1, cursor, dbuf);

    if (unlikely(rc < 0)) return rc;

    rc = __unmarshall_opaque_fixed(v, size, cursor, dbuf);

    if (unlikely(rc < 0)) return rc;

    return 4 + rc;
}
