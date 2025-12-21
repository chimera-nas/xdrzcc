// SPDX-FileCopyrightText: 2025 Ben Jarvis
// SPDX-License-Identifier: Unlicense
// The SPDX identifiers are stripped when this file is embedded into generated code

#ifndef XDRZCC_XDR_BUILTIN_H
#define XDRZCC_XDR_BUILTIN_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

typedef uint32_t xdr_bool;

#ifndef WARN_UNUSED_RESULT
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#endif /* ifndef WARN_UNUSED_RESULT */

#ifndef FORCE_INLINE
#define FORCE_INLINE       __attribute__((always_inline)) inline
#endif /* ifndef FORCE_INLINE */

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif /* ifndef unlikely */

struct evpl_rpc2_rdma_chunk;

#ifndef XDR_MAX_DBUF
#define XDR_MAX_DBUF 4096
#endif /* ifndef XDR_MAX_DBUF */

typedef struct {
    uint32_t len;
    char    *str;
} xdr_string;

typedef struct {
    uint32_t len;
    void    *data;
} xdr_opaque;

typedef struct {
    void *buffer;
    int   size;
    int   used;
} xdr_dbuf;

static inline xdr_dbuf *
xdr_dbuf_alloc(int bytes)
{
    xdr_dbuf *dbuf;

    dbuf = (xdr_dbuf *) malloc(sizeof(*dbuf));

    dbuf->buffer = malloc(bytes);

    dbuf->used = 0;
    dbuf->size = bytes;

    return dbuf;
} /* xdr_dbuf_alloc */

static inline void
xdr_dbuf_free(xdr_dbuf *dbuf)
{
    free(dbuf->buffer);
    free(dbuf);
} /* xdr_dbuf_free */

static inline void
xdr_dbuf_reset(xdr_dbuf *dbuf)
{
    dbuf->used = 0;
} /* xdr_dbuf_reset */

static FORCE_INLINE void * WARN_UNUSED_RESULT
xdr_dbuf_alloc_space(
    int       isize,
    xdr_dbuf *dbuf)
{
    void *ptr;

    if (unlikely(dbuf->used + isize > dbuf->size)) {
        return NULL;
    }
    ptr         = (char *) dbuf->buffer + dbuf->used;
    dbuf->used += isize;
    dbuf->used  = (dbuf->used + 7) & ~7;
    return ptr;
} // xdr_dbuf_alloc_space

static FORCE_INLINE int WARN_UNUSED_RESULT
xdr_dbuf_alloc_opaque(
    xdr_opaque *opaque,
    uint32_t    ilen,
    xdr_dbuf   *dbuf)
{
    opaque->len  = ilen;
    opaque->data = xdr_dbuf_alloc_space(ilen, dbuf);
    if (unlikely(opaque->data == NULL)) {
        return -1;
    }
    return 0;
} // xdr_dbuf_alloc_opaque

static FORCE_INLINE int WARN_UNUSED_RESULT
xdr_dbuf_opaque_copy(
    xdr_opaque *opaque,
    const void *ptr,
    uint32_t    ilen,
    xdr_dbuf   *dbuf)
{
    opaque->len  = ilen;
    opaque->data = xdr_dbuf_alloc_space(ilen, dbuf);
    if (unlikely(opaque->data == NULL)) {
        return -1;
    }
    memcpy(opaque->data, ptr, ilen);
    return 0;
} // xdr_dbuf_opaque_copy

static FORCE_INLINE int WARN_UNUSED_RESULT
xdr_dbuf_alloc_string(
    xdr_string *string,
    const char *istr,
    uint32_t    ilen,
    xdr_dbuf   *dbuf)
{
    string->len = ilen;
    string->str = xdr_dbuf_alloc_space(ilen, dbuf);
    if (unlikely(string->str == NULL)) {
        return -1;
    }
    memcpy(string->str, istr, ilen);
    return 0;
} // xdr_dbuf_alloc_string

static FORCE_INLINE int WARN_UNUSED_RESULT
__xdr_dbuf_alloc_array(
    void    **datap,
    uint32_t *nump,
    uint32_t  num,
    uint32_t  element_size,
    xdr_dbuf *dbuf)
{
    *nump = num;

    *datap = xdr_dbuf_alloc_space(num * element_size, dbuf);

    if (unlikely(*datap == NULL)) {
        return -1;
    }

    return 0;
} // xdr_dbuf_alloc_array

#define xdr_dbuf_alloc_array(structp, member, num_elements, dbuf) \
        __xdr_dbuf_alloc_array((void **) &(structp)->member, \
                               &(structp)->num_ ## member, \
                               num_elements, \
                               sizeof(*(structp)->member), \
                               dbuf)


#define xdr_set_str_static(structp, member, istr, ilen) \
        {                                                   \
            (structp)->member.len = (ilen);                 \
            (structp)->member.str = (char *) (istr);         \
        }

#define xdr_set_ref(structp, member, iiov, iniov, ilength) \
        {                                                               \
            (structp)->member.iov    = (iiov);                             \
            (structp)->member.niov   = (iniov);                           \
            (structp)->member.length = (ilength);                       \
        }

#ifdef XDR_CUSTOM_IOVEC
#define QUOTED(x)   #x
#define TOSTRING(x) QUOTED(x)
#include TOSTRING(XDR_CUSTOM_IOVEC)
#else  /* ifdef XDR_CUSTOM_IOVEC */

typedef struct {
    void    *iov_base;
    uint32_t iov_len;
} xdr_iovec;

#define xdr_iovec_data(iov)          ((iov)->iov_base)
#define xdr_iovec_len(iov)           ((iov)->iov_len)

#define xdr_iovec_set_data(iov, ptr) ((iov)->iov_base = (ptr))
#define xdr_iovec_set_len(iov, len)  ((iov)->iov_len = (len))

#define xdr_iovec_move_private(out, in)
#define xdr_iovec_copy_private(out, in)
#define xdr_iovec_set_private_null(out)

#endif /* ifdef XDR_CUSTOM_IOVEC */

typedef struct {
    xdr_iovec *iov;
    int        niov;
    uint32_t   length;
} xdr_iovecr;

void
dump_output(
    const char *format,
    ...);


#endif /* ifndef XDRZCC_XDR_BUILTIN_H */