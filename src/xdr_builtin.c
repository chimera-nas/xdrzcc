#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#ifndef XDRZCC_XDR_BUILTIN_H
/* Just for in-tree builds to suppress warnings*/
#include "xdr_builtin.h"
#endif /* ifndef XDRZCC_XDR_BUILTIN_H */

#ifndef TRUE
#define TRUE         1
#endif /* ifndef TRUE */

#ifndef FALSE
#define FALSE        0
#endif /* ifndef FALSE */

#define unlikely(x) __builtin_expect(!!(x), 0)
#define FORCE_INLINE __attribute__((always_inline)) inline

static FORCE_INLINE int
xdr_iovec_add_offset(
    xdr_iovec *iov,
    int        offset)
{
    if (unlikely(xdr_iovec_len(iov) <= offset)) {
        return -1;
    }

    xdr_iovec_set_len(iov, xdr_iovec_len(iov) - offset);
    xdr_iovec_set_data(iov, xdr_iovec_data(iov) + offset);

    return 0;
} /* xdr_iovec_add_offset */

static FORCE_INLINE uint32_t
xdr_hton32(uint32_t value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap32(value);
#else  /* if __BYTE_ORDER == __LITTLE_ENDIAN */
    return value;
#endif /* if __BYTE_ORDER == __LITTLE_ENDIAN */
} /* xdr_hton32 */

static FORCE_INLINE uint32_t
xdr_ntoh32(uint32_t value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap32(value);
#else  /* if __BYTE_ORDER == __LITTLE_ENDIAN */
    return value;
#endif /* if __BYTE_ORDER == __LITTLE_ENDIAN */
} /* xdr_ntoh32 */

static FORCE_INLINE uint64_t
xdr_hton64(uint64_t value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap64(value);
#else  /* if __BYTE_ORDER == __LITTLE_ENDIAN */
    return value;
#endif /* if __BYTE_ORDER == __LITTLE_ENDIAN */
} /* xdr_hton64 */

static FORCE_INLINE uint64_t
xdr_ntoh64(uint64_t value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap64(value);
#else  /* if __BYTE_ORDER == __LITTLE_ENDIAN */
    return value;
#endif /* if __BYTE_ORDER == __LITTLE_ENDIAN */
} /* xdr_ntoh64 */

static FORCE_INLINE uint32_t
xdr_pad(uint32_t length)
{
    return (4 - (length & 0x3)) & 0x3;
} /* xdr_pad */

struct xdr_read_cursor {
    const xdr_iovec             *cur;
    const xdr_iovec             *last;
    unsigned int                 iov_offset;
    unsigned int                 offset;
    struct evpl_rpc2_rdma_chunk *read_chunk;
};

struct xdr_write_cursor {
    xdr_iovec                   *iov;
    int                          niov;
    int                          maxiov;
    xdr_iovec                   *scratch_iov;
    void                        *scratch_data;
    int                          scratch_size;
    int                          scratch_used;
    int                          total;
    struct evpl_rpc2_rdma_chunk *write_chunk;
};

static FORCE_INLINE void
xdr_read_cursor_init(
    struct xdr_read_cursor      *cursor,
    const xdr_iovec             *iov,
    int                          niov,
    struct evpl_rpc2_rdma_chunk *read_chunk)
{
    cursor->cur        = iov;
    cursor->last       = iov + (niov - 1);
    cursor->iov_offset = 0;
    cursor->offset     = 0;
    cursor->read_chunk = read_chunk;
} /* xdr_read_cursor_init */

static FORCE_INLINE void
xdr_write_cursor_init(
    struct xdr_write_cursor     *cursor,
    xdr_iovec                   *scratch_iov,
    xdr_iovec                   *out_iov,
    int                          out_niov,
    struct evpl_rpc2_rdma_chunk *write_chunk,
    int                          out_offset)
{
    cursor->iov         = out_iov;
    cursor->niov        = 0;
    cursor->maxiov      = out_niov;
    cursor->scratch_iov = scratch_iov;
    cursor->write_chunk = write_chunk;

    cursor->scratch_used = out_offset;
    cursor->scratch_data = xdr_iovec_data(scratch_iov);
    cursor->scratch_size = xdr_iovec_len(scratch_iov);

    xdr_iovec_set_len(scratch_iov, 0);

    cursor->total = 0;

} /* xdr_write_cursor_init */

static FORCE_INLINE void
xdr_write_cursor_flush(struct xdr_write_cursor *cursor)
{
    xdr_iovec *iov;

    if (cursor->scratch_used) {

        if (unlikely(cursor->niov + 1 > cursor->maxiov)) {
            abort();
        }

        iov = &cursor->iov[cursor->niov++];

        xdr_iovec_set_data(iov, cursor->scratch_data);
        xdr_iovec_set_len(iov, cursor->scratch_used);
        xdr_iovec_copy_private(iov, cursor->scratch_iov);

        xdr_iovec_set_len(cursor->scratch_iov, xdr_iovec_len(cursor->scratch_iov) + cursor->scratch_used);

        cursor->scratch_data += cursor->scratch_used;
        cursor->scratch_size -= cursor->scratch_used;
        cursor->total        += cursor->scratch_used;
        cursor->scratch_used  = 0;
    }

} /* xdr_write_cursor_finish */

static inline int
xdr_read_cursor_extract(
    struct xdr_read_cursor *cursor,
    void                   *out,
    unsigned int            bytes)
{
    unsigned int left, chunk;

    left = bytes;

    while (left) {
        chunk = xdr_iovec_len(cursor->cur) - cursor->iov_offset;
        if (left < chunk) {
            chunk = left;
        }

        memcpy(out,
               xdr_iovec_data(cursor->cur) + cursor->iov_offset,
               chunk);

        left               -= chunk;
        out                 = (char *) out + chunk;
        cursor->iov_offset += chunk;
        cursor->offset     += chunk;

        if (cursor->iov_offset == xdr_iovec_len(cursor->cur)) {
            cursor->cur++;
            cursor->iov_offset = 0;
        }
    }

    return bytes;
} /* xdr_read_cursor_extract */

static inline void
xdr_write_cursor_append(
    struct xdr_write_cursor *cursor,
    const void              *in,
    unsigned int             bytes)
{
    unsigned int chunk;

    if (unlikely(cursor->scratch_used + bytes > cursor->scratch_size)) {
        abort();
    }

    memcpy(cursor->scratch_data + cursor->scratch_used, in, bytes);

    cursor->scratch_used += bytes;
} /* xdr_write_cursor_append */

static inline int
xdr_read_cursor_skip(
    struct xdr_read_cursor *cursor,
    unsigned int            bytes)
{
    unsigned int done, chunk;

    if (cursor->iov_offset + bytes <= xdr_iovec_len(cursor->cur)) {
        cursor->iov_offset += bytes;
        cursor->offset     += bytes;
    } else {
        done = 0;
        while (done < bytes) {
            chunk = xdr_iovec_len(cursor->cur) - cursor->iov_offset;
            if (chunk > bytes - done) {
                chunk = bytes - done;
            }

            done               += chunk;
            cursor->iov_offset += chunk;
            cursor->offset     += chunk;

            if (done < bytes && cursor->cur == cursor->last) {
                abort();
            }

            if (cursor->iov_offset == xdr_iovec_len(cursor->cur)) {
                cursor->cur++;
                cursor->iov_offset = 0;
            }
        }
    }

    return bytes;
} /* xdr_read_cursor_skip */

static FORCE_INLINE uint32_t
__marshall_length_uint32_t(const uint32_t *v)
{
    return 4;
} /* __marshall_length_uint32_t */

static FORCE_INLINE uint32_t
__marshall_length_int32_t(const int32_t *v)
{
    return 4;
} /* __unmarshall_length_int32_t */

static FORCE_INLINE uint32_t
__marshall_length_uint64_t(const uint64_t *v)
{
    return 8;
} /* __marshall_length_uint64_t */

static FORCE_INLINE uint32_t
__marshall_length_int64_t(const int64_t *v)
{
    return 8;
} /* __marshall_length_int64_t */

static FORCE_INLINE uint32_t
__marshall_length_float(const float *v)
{
    return 4;
} /* __marshall_length_float */

static FORCE_INLINE uint32_t
__marshall_length_double(const double *v)
{
    return 8;
} /* __marshall_length_double */

static FORCE_INLINE void
__marshall_uint32_t(
    const uint32_t          *v,
    struct xdr_write_cursor *cursor)
{
    if (unlikely(cursor->scratch_used + 4 > cursor->scratch_size)) {
        abort();
    }

    *(uint32_t *) (cursor->scratch_data + cursor->scratch_used) = xdr_hton32(*v);

    cursor->scratch_used += 4;

} /* __marshall_uint32_t */

static FORCE_INLINE int
__unmarshall_uint32_t(
    uint32_t               *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    uint32_t tmp;
    int      rc;

    rc = xdr_read_cursor_extract(cursor, &tmp, 4);

    if (unlikely(rc < 0)) {
        return rc;
    }

    *v = xdr_ntoh32(tmp);

    return 4;
} /* __unmarshall_uint32_t */

static FORCE_INLINE void
__marshall_int32_t(
    const int32_t           *v,
    struct xdr_write_cursor *cursor)
{
    if (unlikely(cursor->scratch_used + 4 > cursor->scratch_size)) {
        abort();
    }

    *(int32_t *) (cursor->scratch_data + cursor->scratch_used) = xdr_hton32(*v);

    cursor->scratch_used += 4;
} /* __marshall_int32_t */

static FORCE_INLINE int
__unmarshall_int32_t(
    int32_t                *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int32_t tmp;
    int     rc;

    rc = xdr_read_cursor_extract(cursor, &tmp, 4);

    if (unlikely(rc < 0)) {
        return rc;
    }

    *v = xdr_ntoh32(tmp);

    return 4;
} /* __unmarshall_int32_t */

static FORCE_INLINE void
__marshall_uint64_t(
    const uint64_t          *v,
    struct xdr_write_cursor *cursor)
{
    if (unlikely(cursor->scratch_used + 8 > cursor->scratch_size)) {
        abort();
    }

    *(uint64_t *) (cursor->scratch_data + cursor->scratch_used) = xdr_hton64(*v);

    cursor->scratch_used += 8;
} /* __marshall_uint64_t */

static FORCE_INLINE int
__unmarshall_uint64_t(
    uint64_t               *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    uint64_t tmp;
    int      rc;

    rc = xdr_read_cursor_extract(cursor, &tmp, 8);

    if (unlikely(rc < 0)) {
        return rc;
    }

    *v = xdr_ntoh64(tmp);

    return 8;
} /* __unmarshall_uint64_t */

static FORCE_INLINE void
__marshall_int64_t(
    const int64_t           *v,
    struct xdr_write_cursor *cursor)
{
    if (unlikely(cursor->scratch_used + 8 > cursor->scratch_size)) {
        abort();
    }

    *(int64_t *) (cursor->scratch_data + cursor->scratch_used) = xdr_hton64(*v);

    cursor->scratch_used += 8;
} /* __marshall_int64_t */

static FORCE_INLINE int
__unmarshall_int64_t(
    int64_t                *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int64_t tmp;
    int     rc;

    rc = xdr_read_cursor_extract(cursor, &tmp, 8);

    if (unlikely(rc < 0)) {
        return rc;
    }

    *v = xdr_ntoh64(tmp);

    return 8;
} /* __unmarshall_int64_t */

static FORCE_INLINE void
__marshall_float(
    const float             *v,
    struct xdr_write_cursor *cursor)
{
    xdr_write_cursor_append(cursor, v, 4);
} /* __marshall_float */

static FORCE_INLINE int
__unmarshall_float(
    float                  *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    return xdr_read_cursor_extract(cursor, v, 4);
} /* __unmarshall_float */

static FORCE_INLINE void
__marshall_double(
    const double            *v,
    struct xdr_write_cursor *cursor)
{
    xdr_write_cursor_append(cursor, v, 8);
} /* __marshall_double */

static FORCE_INLINE int
__unmarshall_double(
    double                 *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    return xdr_read_cursor_extract(cursor, v, 8);
} /* __unmarshall_double */

static FORCE_INLINE void
__marshall_xdr_string(
    const xdr_string        *str,
    struct xdr_write_cursor *cursor)
{
    const uint32_t zero = 0;
    int            pad;

    __marshall_uint32_t(&str->len, cursor);

    xdr_write_cursor_append(cursor, str->str, str->len);

    pad = (4 - (str->len & 0x3)) & 0x3;

    if (pad) {
        xdr_write_cursor_append(cursor, &zero, pad);
    }
} /* __marshall_xdr_string */

static FORCE_INLINE int
__unmarshall_xdr_string(
    xdr_string             *str,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int rc, pad, len = 0;

    rc = __unmarshall_uint32_t(&str->len, cursor, dbuf);

    if (unlikely(rc < 0)) {
        return rc;
    }

    len += rc;

    if (xdr_iovec_len(cursor->cur) - cursor->iov_offset >= str->len) {
        str->str            = xdr_iovec_data(cursor->cur) + cursor->iov_offset;
        cursor->iov_offset += str->len;
        cursor->offset     += str->len;

        if (cursor->iov_offset == xdr_iovec_len(cursor->cur)) {
            cursor->cur++;
            cursor->iov_offset = 0;
        }

    } else {
        str->str    = dbuf->buffer + dbuf->used;
        dbuf->used += str->len;

        rc = xdr_read_cursor_extract(cursor, str->str, str->len);

        if (unlikely(rc < 0)) {
            return rc;
        }

    }

    len += str->len;
    pad  = (4 - (str->len & 0x3)) & 0x3;

    if (pad) {
        rc = xdr_read_cursor_skip(cursor, pad);

        if (unlikely(rc < 0)) {
            return rc;
        }

        len += rc;
    }

    return len;
} /* __unmarshall_xdr_string */





static FORCE_INLINE int
__unmarshall_opaque_fixed(
    xdr_iovecr             *v,
    uint32_t                size,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int pad, chunk, left = size;

    v->iov    = dbuf->buffer + dbuf->used;
    v->length = size;
    v->niov   = 0;

    do {

        xdr_iovec_set_data(&v->iov[v->niov], xdr_iovec_data(cursor->cur) +
                           cursor->iov_offset);
        xdr_iovec_copy_private(&v->iov[v->niov], cursor->cur);

        chunk = xdr_iovec_len(cursor->cur) - cursor->iov_offset;

        if (left < chunk) {
            chunk = left;
        }

        xdr_iovec_set_len(&v->iov[v->niov], chunk);

        left -= chunk;

        cursor->iov_offset += chunk;
        cursor->offset     += chunk;
        if (cursor->iov_offset == xdr_iovec_len(cursor->cur)) {
            cursor->cur++;
            cursor->iov_offset = 0;
        }

        v->niov++;

    } while (left);

    dbuf->used += sizeof(xdr_iovec) * v->niov;

    pad = (4 - (size & 0x3)) & 0x3;

    xdr_read_cursor_skip(cursor, pad);

    return size + pad;
} /* __unmarshall_opaque_fixed */

static FORCE_INLINE void
__marshall_opaque(
    const xdr_opaque        *v,
    uint32_t                 bound,
    struct xdr_write_cursor *cursor)
{
    int      rc, pad;
    uint32_t zero = 0;

    __marshall_uint32_t(&v->len, cursor);
    xdr_write_cursor_append(cursor, v->data, v->len);

    pad = (4 - (v->len & 0x3)) & 0x3;

    if (pad) {
        xdr_write_cursor_append(cursor, &zero, pad);
    }

} /* __marshall_opaque */

static FORCE_INLINE void
__marshall_opaque_zerocopy(
    const xdr_iovecr        *v,
    struct xdr_write_cursor *cursor)
{
    const uint32_t zero = 0;
    xdr_iovec     *iov;
    int            i, pad, left = v->length;

    __marshall_uint32_t(&v->length, cursor);

#if EVPL_RPC2
    if (cursor->write_chunk && cursor->write_chunk->length) {
        cursor->write_chunk->iov          = v->iov;
        cursor->write_chunk->niov         = v->niov;
        cursor->write_chunk->length       = v->length;
        cursor->write_chunk->xdr_position = cursor->scratch_used + cursor->total;
        fprintf(stderr, "write chunk: offset %d, length %d\n", cursor->write_chunk->xdr_position, cursor->write_chunk->
                length);

        return;
    }
 #endif /* if EVPL_RPC2 */

    xdr_write_cursor_flush(cursor);

    for (i = 0; i < v->niov && left; ++i) {

        if (unlikely(cursor->niov + 1 > cursor->maxiov)) {
            abort();
        }

        iov = &cursor->iov[cursor->niov++];

        xdr_iovec_set_data(iov, xdr_iovec_data(&v->iov[i]));
        xdr_iovec_copy_private(iov, &v->iov[i]);
        xdr_iovec_set_len(iov, xdr_iovec_len(&v->iov[i]));

        if (xdr_iovec_len(iov) > left) {
            xdr_iovec_set_len(iov, left);
        }

        left -= xdr_iovec_len(iov);
    }

    if (unlikely(left)) {
        abort();
    }

    cursor->total += v->length;

    pad = (4 - (v->length & 0x3)) & 0x3;

    if (pad) {
        xdr_write_cursor_append(cursor, &zero, pad);
    }
} /* __marshall_opaque_zerocopy */

static FORCE_INLINE int
__unmarshall_opaque(
    xdr_opaque             *v,
    uint32_t                bound,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int rc, pad;

    rc = __unmarshall_uint32_t(&v->len, cursor, dbuf);

    if (unlikely(rc < 0)) {
        return rc;
    }

    if (xdr_iovec_len(cursor->cur) - cursor->iov_offset >= v->len) {
        v->data             = xdr_iovec_data(cursor->cur) + cursor->iov_offset;
        cursor->iov_offset += v->len;
        cursor->offset     += v->len;
        if (cursor->iov_offset == xdr_iovec_len(cursor->cur)) {
            cursor->cur++;
            cursor->iov_offset = 0;
        }

    } else {
        v->data     = dbuf->buffer + dbuf->used;
        dbuf->used += v->len;

        rc = xdr_read_cursor_extract(cursor, v->data, v->len);

        if (unlikely(rc < 0)) {
            return rc;
        }
    }

    pad = (4 - (v->len & 0x3)) & 0x3;

    if (pad) {
        rc = xdr_read_cursor_skip(cursor, pad);

        if (unlikely(rc < 0)) {
            return rc;
        }
    }

    return 4 + v->len + pad;
} /* __unmarshall_opaque_variable_bound */

static FORCE_INLINE int
__unmarshall_opaque_zerocopy(
    xdr_iovecr             *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int                          rc;
    uint32_t                     size;
    struct evpl_rpc2_rdma_chunk *chunk;

    rc = __unmarshall_uint32_t(&size, cursor, dbuf);

    if (unlikely(rc < 0)) {
        return rc;
    }

#if EVPL_RPC2
    if (cursor->read_chunk && cursor->read_chunk->length) {
        chunk = cursor->read_chunk;
        fprintf(stderr, "rdma chunk: offset %d, length %d\n", chunk->xdr_position, chunk->length);

        if (chunk->xdr_position == cursor->offset) {
            fprintf(stderr, "matched rdma segment length %u size %u\n", chunk->length, size);
            v->iov    = chunk->iov;
            v->niov   = chunk->niov;
            v->length = chunk->length;
            return 4;
        }
    }
#endif /* if EVPL_RPC2 */

    rc = __unmarshall_opaque_fixed(v, size, cursor, dbuf);

    if (unlikely(rc < 0)) {
        return rc;
    }

    return 4 + rc;
} /* __unmarshall_opaque_variable */

static FORCE_INLINE int
is_ascii(
    const char *s,
    int         len)
{

    int i;

    if (len == 0) {
        return 0;
    }

    for (i = 0; i < len; i++) {
        if (s[i] < 0x20 || s[i] > 0x7E) {
            return 0;
        }
    }

    return 1;
} /* is_ascii */

static FORCE_INLINE void
dump_opaque(
    char       *out,
    int         outlen,
    const void *v,
    uint32_t    length)
{
    int i;

    if (is_ascii(v, length)) {
        snprintf(out, outlen, "'%.*s' [%u bytes]", length, (const char *) v,
                 length);
        return;
    }

    if (length >= 32) {
        snprintf(out, outlen, "<opaque> [%u bytes]", length);
        return;
    }

    for (i = 0; i < length; ++i) {
        snprintf(out, outlen, "%02x", ((const uint8_t *) v)[i]);
        out    += 2;
        outlen -= 2;
    }

    *out = '\0';
} /* dump_opaque */

#ifndef XDR_CUSTOM_DUMP
void
dump_output(
    const char *format,
    ...)
{
    va_list ap;

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    va_end(ap);
} /* dump_output */
#endif /* ifndef XDR_CUSTOM_DUMP */