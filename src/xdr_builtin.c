// SPDX-FileCopyrightText: 2025 Ben Jarvis
//
// SPDX-License-Identifier: LGPL-2.1-only

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

static FORCE_INLINE int WARN_UNUSED_RESULT
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
    int                          scratch_reserved;
    int                          total;
    int                          ref_taken;
    struct evpl_rpc2_rdma_chunk *rdma_chunk;
};

static FORCE_INLINE void
xdr_read_cursor_vector_init(
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
} /* xdr_read_cursor_vector_init */

static FORCE_INLINE void
xdr_write_cursor_init(
    struct xdr_write_cursor     *cursor,
    xdr_iovec                   *scratch_iov,
    xdr_iovec                   *out_iov,
    int                          out_niov,
    struct evpl_rpc2_rdma_chunk *rdma_chunk,
    int                          out_offset)
{
    cursor->iov              = out_iov;
    cursor->niov             = 0;
    cursor->maxiov           = out_niov;
    cursor->scratch_iov      = scratch_iov;
    cursor->rdma_chunk       = rdma_chunk;
    cursor->ref_taken        = 0;
    cursor->scratch_used     = out_offset;
    cursor->scratch_reserved = out_offset;
    cursor->scratch_data     = xdr_iovec_data(scratch_iov);
    cursor->scratch_size     = xdr_iovec_len(scratch_iov);

    xdr_iovec_set_len(scratch_iov, 0);

    cursor->total = 0;

} /* xdr_write_cursor_init */

static FORCE_INLINE int WARN_UNUSED_RESULT
xdr_write_cursor_flush(struct xdr_write_cursor *cursor)
{
    xdr_iovec *iov;

    if (cursor->scratch_used) {

        if (unlikely(cursor->niov + 1 > cursor->maxiov)) {
            return -1;
        }

        iov = &cursor->iov[cursor->niov++];

        xdr_iovec_set_data(iov, cursor->scratch_data);
        xdr_iovec_set_len(iov, cursor->scratch_used);

        if (!cursor->ref_taken) {
            xdr_iovec_move_private(iov, cursor->scratch_iov);
            cursor->ref_taken = 1;
        } else {
            xdr_iovec_copy_private(iov, cursor->scratch_iov);
        }

        xdr_iovec_set_len(cursor->scratch_iov, xdr_iovec_len(cursor->scratch_iov) + cursor->scratch_used);

        cursor->scratch_data += cursor->scratch_used;
        cursor->scratch_size -= cursor->scratch_used;
        cursor->total        += cursor->scratch_used;
        cursor->scratch_used  = 0;
    }

    return 0;
} /* xdr_write_cursor_flush */

static inline int
xdr_read_cursor_vector_extract(
    struct xdr_read_cursor *cursor,
    void                   *out,
    unsigned int            bytes)
{
    unsigned int left, chunk;

    left = bytes;

    while (left) {
        if (unlikely(cursor->cur > cursor->last)) {
            return -1;
        }

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
} /* xdr_read_cursor_vector_extract */

static inline int WARN_UNUSED_RESULT
xdr_write_cursor_append(
    struct xdr_write_cursor *cursor,
    const void              *in,
    unsigned int             bytes)
{
    if (unlikely(cursor->scratch_used + bytes > cursor->scratch_size)) {
        return -1;
    }

    memcpy(cursor->scratch_data + cursor->scratch_used, in, bytes);

    cursor->scratch_used += bytes;
    return 0;
} /* xdr_write_cursor_append */

static inline int
xdr_read_cursor_vector_skip(
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
                return -1;
            }

            if (cursor->iov_offset == xdr_iovec_len(cursor->cur)) {
                cursor->cur++;
                cursor->iov_offset = 0;
            }
        }
    }

    return bytes;
} /* xdr_read_cursor_vector_skip */

static FORCE_INLINE void
xdr_read_cursor_contig_init(
    struct xdr_read_cursor      *cursor,
    const xdr_iovec             *iov,
    struct evpl_rpc2_rdma_chunk *read_chunk)
{
    cursor->cur        = iov;
    cursor->last       = iov;
    cursor->iov_offset = 0;
    cursor->offset     = 0;
    cursor->read_chunk = read_chunk;
} /* xdr_read_cursor_contig_init */

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

static FORCE_INLINE int WARN_UNUSED_RESULT
__marshall_uint32_t(
    const uint32_t          *v,
    struct xdr_write_cursor *cursor)
{
    if (unlikely(cursor->scratch_used + 4 > cursor->scratch_size)) {
        return -1;
    }

    *(uint32_t *) (cursor->scratch_data + cursor->scratch_used) = xdr_hton32(*v);

    cursor->scratch_used += 4;

    return 0;
} /* __marshall_uint32_t */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_uint32_t_vector(
    uint32_t               *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    uint32_t tmp;
    int      rc;

    rc = xdr_read_cursor_vector_extract(cursor, &tmp, 4);

    if (unlikely(rc < 0)) {
        return rc;
    }

    *v = xdr_ntoh32(tmp);

    return 4;
} /* __unmarshall_uint32_t_vector */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_uint32_t_contig(
    uint32_t               *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    if (unlikely(cursor->iov_offset + 4 > xdr_iovec_len(cursor->cur))) {
        return -1;
    }

    *v                  = xdr_ntoh32(*(const uint32_t *) (xdr_iovec_data(cursor->cur) + cursor->iov_offset));
    cursor->iov_offset += 4;
    cursor->offset     += 4;
    return 4;
} /* __unmarshall_uint32_t_contig */

static FORCE_INLINE int WARN_UNUSED_RESULT
__marshall_int32_t(
    const int32_t           *v,
    struct xdr_write_cursor *cursor)
{
    if (unlikely(cursor->scratch_used + 4 > cursor->scratch_size)) {
        return -1;
    }

    *(int32_t *) (cursor->scratch_data + cursor->scratch_used) = xdr_hton32(*v);

    cursor->scratch_used += 4;
    return 0;
} /* __marshall_int32_t */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_int32_t_vector(
    int32_t                *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int32_t tmp;
    int     rc;

    rc = xdr_read_cursor_vector_extract(cursor, &tmp, 4);

    if (unlikely(rc < 0)) {
        return rc;
    }

    *v = xdr_ntoh32(tmp);

    return 4;
} /* __unmarshall_int32_t_vector */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_int32_t_contig(
    int32_t                *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    if (unlikely(cursor->iov_offset + 4 > xdr_iovec_len(cursor->cur))) {
        return -1;
    }

    *v                  = xdr_ntoh32(*(const int32_t *) (xdr_iovec_data(cursor->cur) + cursor->iov_offset));
    cursor->iov_offset += 4;
    cursor->offset     += 4;
    return 4;
} /* __unmarshall_int32_t_contig */

static FORCE_INLINE int WARN_UNUSED_RESULT
__marshall_uint64_t(
    const uint64_t          *v,
    struct xdr_write_cursor *cursor)
{
    if (unlikely(cursor->scratch_used + 8 > cursor->scratch_size)) {
        return -1;
    }

    *(uint64_t *) (cursor->scratch_data + cursor->scratch_used) = xdr_hton64(*v);

    cursor->scratch_used += 8;
    return 0;
} /* __marshall_uint64_t */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_uint64_t_vector(
    uint64_t               *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    uint64_t tmp;
    int      rc;

    rc = xdr_read_cursor_vector_extract(cursor, &tmp, 8);

    if (unlikely(rc < 0)) {
        return rc;
    }

    *v = xdr_ntoh64(tmp);

    return 8;
} /* __unmarshall_uint64_t_vector */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_uint64_t_contig(
    uint64_t               *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    if (unlikely(cursor->iov_offset + 8 > xdr_iovec_len(cursor->cur))) {
        return -1;
    }

    *v                  = xdr_ntoh64(*(const uint64_t *) (xdr_iovec_data(cursor->cur) + cursor->iov_offset));
    cursor->iov_offset += 8;
    cursor->offset     += 8;
    return 8;
} /* __unmarshall_uint64_t_contig */

static FORCE_INLINE int WARN_UNUSED_RESULT
__marshall_int64_t(
    const int64_t           *v,
    struct xdr_write_cursor *cursor)
{
    if (unlikely(cursor->scratch_used + 8 > cursor->scratch_size)) {
        return -1;
    }

    *(int64_t *) (cursor->scratch_data + cursor->scratch_used) = xdr_hton64(*v);

    cursor->scratch_used += 8;
    return 0;
} /* __marshall_int64_t */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_int64_t_vector(
    int64_t                *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int64_t tmp;
    int     rc;

    rc = xdr_read_cursor_vector_extract(cursor, &tmp, 8);

    if (unlikely(rc < 0)) {
        return rc;
    }

    *v = xdr_ntoh64(tmp);

    return 8;
} /* __unmarshall_int64_t_vector */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_int64_t_contig(
    int64_t                *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    if (unlikely(cursor->iov_offset + 8 > xdr_iovec_len(cursor->cur))) {
        return -1;
    }

    *v                  = xdr_ntoh64(*(const int64_t *) (xdr_iovec_data(cursor->cur) + cursor->iov_offset));
    cursor->iov_offset += 8;
    cursor->offset     += 8;
    return 8;
} /* __unmarshall_int64_t_contig */

static FORCE_INLINE int WARN_UNUSED_RESULT
__marshall_float(
    const float             *v,
    struct xdr_write_cursor *cursor)
{
    return xdr_write_cursor_append(cursor, v, 4);
} /* __marshall_float */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_float_vector(
    float                  *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    return xdr_read_cursor_vector_extract(cursor, v, 4);
} /* __unmarshall_float_vector */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_float_contig(
    float                  *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    if (unlikely(cursor->iov_offset + 4 > xdr_iovec_len(cursor->cur))) {
        return -1;
    }

    *v                  = *(const float *) (xdr_iovec_data(cursor->cur) + cursor->iov_offset);
    cursor->iov_offset += 4;
    cursor->offset     += 4;
    return 4;
} /* __unmarshall_float_contig */

static FORCE_INLINE int WARN_UNUSED_RESULT
__marshall_double(
    const double            *v,
    struct xdr_write_cursor *cursor)
{
    return xdr_write_cursor_append(cursor, v, 8);
} /* __marshall_double */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_double_vector(
    double                 *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    return xdr_read_cursor_vector_extract(cursor, v, 8);
} /* __unmarshall_double_vector */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_double_contig(
    double                 *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    if (unlikely(cursor->iov_offset + 8 > xdr_iovec_len(cursor->cur))) {
        return -1;
    }

    *v                  = *(const double *) (xdr_iovec_data(cursor->cur) + cursor->iov_offset);
    cursor->iov_offset += 8;
    cursor->offset     += 8;
    return 8;
} /* __unmarshall_double_contig */

static FORCE_INLINE int WARN_UNUSED_RESULT
__marshall_xdr_string(
    const xdr_string        *str,
    struct xdr_write_cursor *cursor)
{
    const uint32_t zero = 0;
    int            pad;
    int            rc;

    rc = __marshall_uint32_t(&str->len, cursor);
    if (unlikely(rc < 0)) {
        return rc;
    }

    rc = xdr_write_cursor_append(cursor, str->str, str->len);
    if (unlikely(rc < 0)) {
        return rc;
    }

    pad = (4 - (str->len & 0x3)) & 0x3;

    if (pad) {
        rc = xdr_write_cursor_append(cursor, &zero, pad);
        if (unlikely(rc < 0)) {
            return rc;
        }
    }

    return 0;
} /* __marshall_xdr_string */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_xdr_string_vector(
    xdr_string             *str,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int rc, pad, len = 0;

    rc = __unmarshall_uint32_t_vector(&str->len, cursor, dbuf);

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
        str->str = xdr_dbuf_alloc_space(str->len, dbuf);
        if (unlikely(str->str == NULL)) {
            return -1;
        }

        rc = xdr_read_cursor_vector_extract(cursor, str->str, str->len);

        if (unlikely(rc < 0)) {
            return rc;
        }

    }

    len += str->len;
    pad  = (4 - (str->len & 0x3)) & 0x3;

    if (pad) {
        rc = xdr_read_cursor_vector_skip(cursor, pad);

        if (unlikely(rc < 0)) {
            return rc;
        }

        len += rc;
    }

    return len;
} /* __unmarshall_xdr_string_vector */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_xdr_string_contig(
    xdr_string             *str,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int rc, pad, len = 0;

    rc = __unmarshall_uint32_t_contig(&str->len, cursor, dbuf);
    if (unlikely(rc < 0)) {
        return rc;
    }
    len += rc;

    pad = (4 - (str->len & 0x3)) & 0x3;
    if (unlikely(cursor->iov_offset + str->len + pad > xdr_iovec_len(cursor->cur))) {
        return -1;
    }

    str->str            = (char *) (xdr_iovec_data(cursor->cur) + cursor->iov_offset);
    cursor->iov_offset += str->len;
    cursor->offset     += str->len;
    len                += str->len;

    if (pad) {
        cursor->iov_offset += pad;
        cursor->offset     += pad;
        len                += pad;
    }

    return len;
} /* __unmarshall_xdr_string_contig */





static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_opaque_fixed_vector(
    xdr_iovecr             *v,
    uint32_t                size,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int pad, chunk, left = size;

    v->iov = xdr_dbuf_alloc_space(sizeof(*v->iov) * ((cursor->last - cursor->cur) + 1), dbuf);
    if (unlikely(v->iov == NULL)) {
        return -1;
    }

    v->length = size;
    v->niov   = 0;

    do {
        if (unlikely(cursor->cur > cursor->last)) {
            return -1;
        }

        xdr_iovec_set_data(&v->iov[v->niov], xdr_iovec_data(cursor->cur) +
                           cursor->iov_offset);
        xdr_iovec_move_private(&v->iov[v->niov], cursor->cur);

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

    pad = (4 - (size & 0x3)) & 0x3;

    xdr_read_cursor_vector_skip(cursor, pad);

    return size + pad;
} /* __unmarshall_opaque_fixed_vector */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_opaque_fixed_contig(
    xdr_iovecr             *v,
    uint32_t                size,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int pad;

    pad = (4 - (size & 0x3)) & 0x3;
    if (unlikely(cursor->iov_offset + size + pad > xdr_iovec_len(cursor->cur))) {
        return -1;
    }

    v->length = size;
    v->niov   = 1;
    v->iov    = xdr_dbuf_alloc_space(sizeof(*v->iov), dbuf);
    if (unlikely(v->iov == NULL)) {
        return -1;
    }

    xdr_iovec_set_data(&v->iov[0], (void *) (xdr_iovec_data(cursor->cur) + cursor->iov_offset));
    xdr_iovec_set_len(&v->iov[0], size);
    xdr_iovec_move_private(&v->iov[0], cursor->cur);

    cursor->iov_offset += size;
    cursor->offset     += size;

    if (pad) {
        cursor->iov_offset += pad;
        cursor->offset     += pad;
    }

    return size + pad;
} /* __unmarshall_opaque_fixed_contig */

static FORCE_INLINE int WARN_UNUSED_RESULT
__marshall_opaque(
    const xdr_opaque        *v,
    uint32_t                 bound,
    struct xdr_write_cursor *cursor)
{
    int      rc, pad;
    uint32_t zero = 0;

    rc = __marshall_uint32_t(&v->len, cursor);
    if (unlikely(rc < 0)) {
        return rc;
    }

    rc = xdr_write_cursor_append(cursor, v->data, v->len);
    if (unlikely(rc < 0)) {
        return rc;
    }

    pad = (4 - (v->len & 0x3)) & 0x3;

    if (pad) {
        rc = xdr_write_cursor_append(cursor, &zero, pad);
        if (unlikely(rc < 0)) {
            return rc;
        }
    }

    return 0;
} /* __marshall_opaque */

static FORCE_INLINE int WARN_UNUSED_RESULT
__marshall_opaque_zerocopy(
    const xdr_iovecr        *v,
    struct xdr_write_cursor *cursor)
{
    const uint32_t zero = 0;
    xdr_iovec     *iov;
    int            i, pad, left = v->length;
    int            rc;

    rc = __marshall_uint32_t(&v->length, cursor);
    if (unlikely(rc < 0)) {
        return rc;
    }

#if EVPL_RPC2
    if (cursor->rdma_chunk && v->length <= cursor->rdma_chunk->max_length) {
        cursor->rdma_chunk->iov          = v->iov;
        cursor->rdma_chunk->niov         = v->niov;
        cursor->rdma_chunk->length       = v->length;
        cursor->rdma_chunk->xdr_position = cursor->scratch_used - cursor->scratch_reserved;
        return 0;
    }
 #endif /* if EVPL_RPC2 */

    rc = xdr_write_cursor_flush(cursor);
    if (unlikely(rc < 0)) {
        return rc;
    }

    for (i = 0; i < v->niov && left; ++i) {

        if (unlikely(cursor->niov + 1 > cursor->maxiov)) {
            return -1;
        }

        iov = &cursor->iov[cursor->niov++];

        xdr_iovec_set_data(iov, xdr_iovec_data(&v->iov[i]));
        xdr_iovec_move_private(iov, &v->iov[i]);
        xdr_iovec_set_len(iov, xdr_iovec_len(&v->iov[i]));

        if (xdr_iovec_len(iov) > left) {
            xdr_iovec_set_len(iov, left);
        }

        left -= xdr_iovec_len(iov);
    }

    if (unlikely(left)) {
        return -1;
    }

    cursor->total += v->length;

    pad = (4 - (v->length & 0x3)) & 0x3;

    if (pad) {
        rc = xdr_write_cursor_append(cursor, &zero, pad);
        if (unlikely(rc < 0)) {
            return rc;
        }
    }

    return 0;
} /* __marshall_opaque_zerocopy */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_opaque_vector(
    xdr_opaque             *v,
    uint32_t                bound,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int rc, pad;

    rc = __unmarshall_uint32_t_vector(&v->len, cursor, dbuf);

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
        v->data = xdr_dbuf_alloc_space(v->len, dbuf);
        if (unlikely(v->data == NULL)) {
            return -1;
        }

        rc = xdr_read_cursor_vector_extract(cursor, v->data, v->len);

        if (unlikely(rc < 0)) {
            return rc;
        }
    }

    pad = (4 - (v->len & 0x3)) & 0x3;

    if (pad) {
        rc = xdr_read_cursor_vector_skip(cursor, pad);

        if (unlikely(rc < 0)) {
            return rc;
        }
    }

    return 4 + v->len + pad;
} /* __unmarshall_opaque_variable_bound_vector */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_opaque_contig(
    xdr_opaque             *v,
    uint32_t                bound,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int rc, pad, len = 0;

    rc = __unmarshall_uint32_t_contig(&v->len, cursor, dbuf);
    if (unlikely(rc < 0)) {
        return rc;
    }
    len += rc;

    pad = (4 - (v->len & 0x3)) & 0x3;
    if (unlikely(cursor->iov_offset + v->len + pad > xdr_iovec_len(cursor->cur))) {
        return -1;
    }

    v->data             = (void *) (xdr_iovec_data(cursor->cur) + cursor->iov_offset);
    cursor->iov_offset += v->len;
    cursor->offset     += v->len;
    len                += v->len;

    if (pad) {
        cursor->iov_offset += pad;
        cursor->offset     += pad;
        len                += pad;
    }

    return len;
} /* __unmarshall_opaque_contig */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_opaque_zerocopy_vector(
    xdr_iovecr             *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int                          rc;
    uint32_t                     size;
    struct evpl_rpc2_rdma_chunk *chunk;

    rc = __unmarshall_uint32_t_vector(&size, cursor, dbuf);

    if (unlikely(rc < 0)) {
        return rc;
    }

#if EVPL_RPC2
    if (cursor->read_chunk && cursor->read_chunk->length) {
        chunk = cursor->read_chunk;
        if (chunk->xdr_position == cursor->offset ||
            chunk->xdr_position == UINT32_MAX) {
            v->iov    = chunk->iov;
            v->niov   = chunk->niov;
            v->length = chunk->length;
            return 4;
        }
    }
#endif /* if EVPL_RPC2 */

    rc = __unmarshall_opaque_fixed_vector(v, size, cursor, dbuf);

    if (unlikely(rc < 0)) {
        return rc;
    }

    return 4 + rc;
} /* __unmarshall_opaque_variable_vector */

static FORCE_INLINE int WARN_UNUSED_RESULT
__unmarshall_opaque_zerocopy_contig(
    xdr_iovecr             *v,
    struct xdr_read_cursor *cursor,
    xdr_dbuf               *dbuf)
{
    int      rc, len = 0;
    uint32_t size;

    rc = __unmarshall_uint32_t_contig(&size, cursor, dbuf);
    if (unlikely(rc < 0)) {
        return rc;
    }
    len += rc;

#if EVPL_RPC2
    if (cursor->read_chunk && cursor->read_chunk->length) {
        struct evpl_rpc2_rdma_chunk *chunk = cursor->read_chunk;
        if (chunk->xdr_position == cursor->offset ||
            chunk->xdr_position == UINT32_MAX) {
            v->iov    = chunk->iov;
            v->niov   = chunk->niov;
            v->length = chunk->length;
            return len;
        }
    }
#endif /* if EVPL_RPC2 */

    rc = __unmarshall_opaque_fixed_contig(v, size, cursor, dbuf);
    if (unlikely(rc < 0)) {
        return rc;
    }
    len += rc;

    return len;
} /* __unmarshall_opaque_zerocopy_contig */

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