#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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

struct xdr_read_cursor {
    const xdr_iovec *cur;
    const xdr_iovec *last;
    unsigned int     offset;
};

struct xdr_write_cursor {
    xdr_iovec       *cur;
    xdr_iovec       *first;
    xdr_iovec       *last;
    const xdr_iovec *scratch_iov;
    const xdr_iovec *scratch_last;
    unsigned int     size;
};

static FORCE_INLINE void
xdr_read_cursor_init(
    struct xdr_read_cursor *cursor,
    const xdr_iovec        *iov,
    int                     niov)
{
    cursor->cur    = iov;
    cursor->last   = iov + (niov - 1);
    cursor->offset = 0;
} /* xdr_read_cursor_init */

static FORCE_INLINE void
xdr_write_cursor_init(
    struct xdr_write_cursor *cursor,
    const xdr_iovec         *scratch_iov,
    int                      scratch_niov,
    xdr_iovec               *out_iov,
    int                      out_niov,
    int                      out_offset)
{
    cursor->scratch_iov  = scratch_iov;
    cursor->scratch_last = scratch_iov + (scratch_niov - 1);
    cursor->cur          = out_iov;
    cursor->first        = out_iov;
    cursor->last         = out_iov + (out_niov - 1);

    xdr_iovec_set_data(cursor->cur, xdr_iovec_data(cursor->scratch_iov));
    xdr_iovec_copy_private(cursor->cur, cursor->scratch_iov);
    xdr_iovec_set_len(cursor->cur, out_offset);

    cursor->size = xdr_iovec_len(cursor->scratch_iov);

    cursor->scratch_iov++;
} /* xdr_write_cursor_init */

static FORCE_INLINE int
xdr_write_cursor_finish(struct xdr_write_cursor *cursor)
{
    if (cursor->cur > cursor->first && xdr_iovec_len(cursor->cur) == 0) {
        --cursor->cur;
    }

    return (cursor->cur - cursor->first) + 1;
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

        chunk = xdr_iovec_len(cursor->cur) - cursor->offset;

        if (left < chunk) {
            chunk = left;
        }

        memcpy(out,
               xdr_iovec_data(cursor->cur) + cursor->offset,
               chunk);

        left           -= chunk;
        out            += chunk;
        cursor->offset += chunk;

        if (cursor->offset == xdr_iovec_len(cursor->cur)) {
            cursor->cur++;
            cursor->offset = 0;
        }
    }

    return bytes;

} /* xdr_read_cursor_extract */

static inline int
xdr_write_cursor_append(
    struct xdr_write_cursor *cursor,
    const void              *in,
    unsigned int             bytes)
{
    unsigned int done, chunk;

    if (bytes <= cursor->size - xdr_iovec_len(cursor->cur)) {
        memcpy(xdr_iovec_data(cursor->cur) + xdr_iovec_len(cursor->cur), in,
               bytes);
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

            if (done < bytes && (cursor->cur == cursor->last || cursor->
                                 scratch_iov == cursor->scratch_last)) {
                abort();
            }

            cursor->cur++;

            xdr_iovec_set_data(cursor->cur, xdr_iovec_data(cursor->scratch_iov))
            ;
            xdr_iovec_copy_private(cursor->cur, cursor->scratch_iov);
            xdr_iovec_set_len(cursor->cur, 0);
            cursor->size = xdr_iovec_len(cursor->scratch_iov);
            cursor->scratch_iov++;
        }
    }

    return bytes;
} /* xdr_write_cursor_append */

static inline int
xdr_read_cursor_skip(
    struct xdr_read_cursor *cursor,
    unsigned int            bytes)
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
                abort();
            }

            cursor->cur++;
            cursor->offset = 0;
        }
    }

    return bytes;
} /* xdr_read_cursor_skip */

static FORCE_INLINE int
__marshall_uint32_t(
    const uint32_t          *v,
    struct xdr_write_cursor *cursor)
{
    uint32_t tmp;
    int      rc;

    tmp = xdr_hton32(*v);

    rc = xdr_write_cursor_append(cursor, &tmp, 4);

    if (unlikely(rc < 0)) {
        abort();
    }

    return 4;
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

static FORCE_INLINE int
__marshall_int32_t(
    const int32_t           *v,
    struct xdr_write_cursor *cursor)
{
    int32_t tmp;
    int     rc;

    tmp = xdr_hton32(*v);
    rc  = xdr_write_cursor_append(cursor, &tmp, 4);

    if (unlikely(rc < 0)) {
        abort();
    }

    return 4;
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

static FORCE_INLINE int
__marshall_uint64_t(
    const uint64_t          *v,
    struct xdr_write_cursor *cursor)
{
    uint64_t tmp;
    int      rc;

    tmp = xdr_hton64(*v);

    rc = xdr_write_cursor_append(cursor, &tmp, 8);

    if (unlikely(rc < 0)) {
        abort();
    }

    return 8;
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

static FORCE_INLINE int
__marshall_int64_t(
    const int64_t           *v,
    struct xdr_write_cursor *cursor)
{
    int64_t tmp;
    int     rc;

    tmp = xdr_hton64(*v);
    rc  = xdr_write_cursor_append(cursor, &tmp, 8);

    if (unlikely(rc < 0)) {
        abort();
    }

    return 8;
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

static FORCE_INLINE int
__marshall_float(
    const float             *v,
    struct xdr_write_cursor *cursor)
{
    return xdr_write_cursor_append(cursor, v, 4);
} /* __marshall_float */

static FORCE_INLINE int
__unmarshall_float(
    float                  *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    return xdr_read_cursor_extract(cursor, v, 4);
} /* __unmarshall_float */

static FORCE_INLINE int
__marshall_double(
    const double            *v,
    struct xdr_write_cursor *cursor)
{
    return xdr_write_cursor_append(cursor, v, 8);
} /* __marshall_double */

static FORCE_INLINE int
__unmarshall_double(
    double                 *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    return xdr_read_cursor_extract(cursor, v, 8);
} /* __unmarshall_double */

static FORCE_INLINE int
__marshall_xdr_string(
    const xdr_string        *str,
    struct xdr_write_cursor *cursor)
{
    const uint32_t zero = 0;
    int            rc, pad, len = 0;

    len += __marshall_uint32_t(&str->len, cursor);

    len += xdr_write_cursor_append(cursor, str->str, str->len);

    pad = (4 - (len & 0x3)) & 0x3;

    if (pad) {
        len += xdr_write_cursor_append(cursor, &zero, pad);
    }

    return len;
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

    str->str = dbuf->buffer + dbuf->used;

    dbuf->used += str->len + 1;

    rc = xdr_read_cursor_extract(cursor, str->str, str->len);

    if (unlikely(rc < 0)) {
        return rc;
    }

    len += rc;

    str->str[str->len] = '\0';

    pad = (4 - (str->len & 0x3)) & 0x3;

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
__marshall_opaque_fixed(
    const xdr_iovecr        *v,
    uint32_t                 size,
    struct xdr_write_cursor *cursor)
{
    const uint32_t zero = 0;
    xdr_iovec     *prev = cursor->cur;
    int            i, rc, pad, left = v->length;

    if (size == 0) {
        return 0;
    }

    for (i = 0; i < v->niov && left; ++i) {

        if (xdr_iovec_len(cursor->cur)) {

            if (unlikely(cursor->cur == cursor->last)) {
                abort();
            }

            cursor->cur++;
        }

        *cursor->cur = v->iov[i];

        if (xdr_iovec_len(cursor->cur) > left) {
            xdr_iovec_set_len(cursor->cur, left);
        }

        left -= xdr_iovec_len(cursor->cur);
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
    }

    return size + pad;
} /* __marshall_opaque_fixed */


static FORCE_INLINE int
__unmarshall_opaque_fixed(
    xdr_iovecr             *v,
    uint32_t                size,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int pad, left = size;

    v->iov    = dbuf->buffer + dbuf->used;
    v->length = size;
    v->niov   = 0;

    do {

        xdr_iovec_set_data(&v->iov[v->niov], xdr_iovec_data(cursor->cur) +
                           cursor->offset);
        xdr_iovec_set_len(&v->iov[v->niov], xdr_iovec_len(cursor->cur));

        xdr_iovec_copy_private(&v->iov[v->niov], cursor->cur);

        if (left < xdr_iovec_len(&v->iov[v->niov])) {
            xdr_iovec_set_len(&v->iov[v->niov], left);
        }

        left -= xdr_iovec_len(&v->iov[v->niov]);

        cursor->cur++;
        cursor->offset = 0;
        v->niov++;
    } while (left);

    dbuf->used += sizeof(xdr_iovec) * v->niov;

    pad = (4 - (size & 0x3)) & 0x3;

    xdr_read_cursor_skip(cursor, size + pad);

    return size + pad;
} /* __unmarshall_opaque_fixed */

static FORCE_INLINE int
__marshall_opaque(
    const xdr_opaque        *v,
    uint32_t                 bound,
    struct xdr_write_cursor *cursor)
{
    int      rc, pad;
    uint32_t zero = 0;

    rc = __marshall_uint32_t(&v->len, cursor);

    if (unlikely(rc < 0)) {
        abort();
    }

    rc = xdr_write_cursor_append(cursor, v->data, v->len);

    if (unlikely(rc < 0)) {
        abort();
    }


    pad = (4 - (v->len & 0x3)) & 0x3;

    if (pad) {
        rc = xdr_write_cursor_append(cursor, &zero, pad);

        if (unlikely(rc < 0)) {
            abort();
        }
    }

    return 4 + v->len + pad;
} /* __marshall_opaque_variable */

static FORCE_INLINE int
__marshall_opaque_zerocopy(
    const xdr_iovecr        *v,
    struct xdr_write_cursor *cursor)
{
    int rc;

    rc = __marshall_uint32_t(&v->length, cursor);

    if (unlikely(rc < 0)) {
        abort();
    }

    rc = __marshall_opaque_fixed(v, v->length, cursor);

    if (unlikely(rc < 0)) {
        abort();
    }

    return 4 + rc;
} /* __marshall_opaque_variable */

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

    v->data     = dbuf->buffer + dbuf->used;
    dbuf->used += v->len;

    rc = xdr_read_cursor_extract(cursor, v->data, v->len);

    if (unlikely(rc < 0)) {
        return rc;
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
    int      rc;
    uint32_t size;

    rc = __unmarshall_uint32_t(&size, cursor, dbuf);

    if (unlikely(rc < 0)) {
        return rc;
    }

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