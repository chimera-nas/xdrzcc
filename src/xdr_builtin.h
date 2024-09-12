#pragma once

#include <stdint.h>
#include <string.h>

#ifndef XDR_MAX_DBUF
#define XDR_MAX_DBUF 4096
#endif

typedef struct {
    uint32_t len;
    char    *str;
} xdr_string;

typedef struct {
    void *buffer;
    int size;
    int used;
} xdr_dbuf;

xdr_dbuf *xdr_dbuf_alloc(void);
void xdr_dbuf_free(xdr_dbuf *dbuf);

static inline void
xdr_dbuf_reset(xdr_dbuf *dbuf)
{
    dbuf->used = 0;
}

#define xdr_dbuf_reserve(structp, member, num, dbuf) { \
    (structp)->num_##member = num; \
    (structp)->member = dbuf->buffer + dbuf->used; \
    dbuf->used += num * sizeof(*((structp)->member)); \
}

#define xdr_dbuf_strncpy(structp, member, istr, ilen, dbuf) { \
    (structp)->member.len = (ilen); \
    (structp)->member.str = (char *)(dbuf->buffer + (dbuf)->used); \
    memcpy((structp)->member.str, (istr), (ilen) + 1); \
    (dbuf)->used += (ilen) + 1; \
}

#define xdr_set_ref(structp, member, iiov, iniov, ioffset, ilength) { \
    (structp)->member.iov  = (iiov); \
    (structp)->member.niov = (iniov); \
    (structp)->member.offset = (ioffset); \
    (structp)->member.length = (ilength); \
}

#ifndef xdr_iovec
typedef struct {
    void    *iov_base;
    uint32_t iov_len;
} xdr_iovec;

#define xdr_iovec_data(iov) ((iov)->iov_base)
#define xdr_iovec_len(iov) ((iov)->iov_len)

#define xdr_iovec_set_data(iov, ptr) ((iov)->iov_base = (ptr))
#define xdr_iovec_set_len(iov, len) ((iov)->iov_len = (len))

#endif


typedef struct {
    const xdr_iovec *iov;
    int              niov;
    int              offset;
    uint32_t         length;
} xdr_iovecr;
