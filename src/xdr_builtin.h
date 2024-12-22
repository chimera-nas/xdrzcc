#ifndef XDRZCC_XDR_BUILTIN_H
#define XDRZCC_XDR_BUILTIN_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

struct evpl_rpc2_rdma_segment;
#ifndef XDR_MAX_DBUF
#define XDR_MAX_DBUF 4096
#endif

typedef struct
{
    uint32_t len;
    char *str;
} xdr_string;

typedef struct
{
    uint32_t len;
    void *data;
} xdr_opaque;

typedef struct
{
    void *buffer;
    int size;
    int used;
} xdr_dbuf;

static inline xdr_dbuf *
xdr_dbuf_alloc(void)
{
    xdr_dbuf *dbuf;

    dbuf = malloc(sizeof(*dbuf));

    dbuf->buffer = malloc(65536);

    dbuf->used = 0;
    dbuf->size = 65536;

    return dbuf;
}

static inline void
xdr_dbuf_free(xdr_dbuf *dbuf)
{
    free(dbuf->buffer);
    free(dbuf);
}

static inline void
xdr_dbuf_reset(xdr_dbuf *dbuf)
{
    dbuf->used = 0;
}

#define xdr_dbuf_alloc_opaque(opaque, ilen, dbuf) \
    {                                      \
        (opaque)->len = (ilen);             \
        (opaque)->data = (dbuf)->buffer + (dbuf)->used; \
        (dbuf)->used += (ilen);                        \
    }

#define xdr_dbuf_opaque_copy(opaque, ptr, ilen, dbuf) \
    {                                      \
        (opaque)->len = (ilen);             \
        (opaque)->data = (dbuf)->buffer + (dbuf)->used; \
        memcpy((opaque)->data, (ptr), (ilen)); \
        (dbuf)->used += (ilen);              \
    }

#define xdr_dbuf_reserve(structp, member, num, dbuf)      \
    {                                                     \
        (structp)->num_##member = num;                    \
        (structp)->member = dbuf->buffer + dbuf->used;    \
        dbuf->used += num * sizeof(*((structp)->member)); \
    }

#define xdr_dbuf_alloc_space(ptr, size, dbuf)      \
    {                                                     \
        ptr = dbuf->buffer + dbuf->used;    \
        dbuf->used += size; \
    }

#define xdr_dbuf_reserve_str(structp, member, ilen, dbuf) \
    {                                                                  \
        (structp)->member.len = (ilen);                                \
        (structp)->member.str = (char *)(dbuf)->buffer + (dbuf)->used; \
        (dbuf)->used += (ilen) + 1;                                    \
    }

#define xdr_dbuf_strncpy(structp, member, istr, ilen, dbuf)            \
    {                                                                  \
        (structp)->member.len = (ilen);                                \
        (structp)->member.str = (char *)(dbuf)->buffer + (dbuf)->used; \
        memcpy((structp)->member.str, (istr), (ilen) + 1);             \
        (dbuf)->used += (ilen) + 1;                                    \
    }

#define xdr_dbuf_memcpy(object, ibuf, ilen, dbuf)             \
    {                                                                  \
        (object)->len  = (ilen);                               \
        (object)->data = (dbuf)->buffer + (dbuf)->used;        \
        memcpy((object)->data, (ibuf), (ilen));                \
        (dbuf)->used += (ilen);                                        \
    }

#define xdr_set_str_static(structp, member, istr, ilen) \
    {                                                   \
        (structp)->member.len = (ilen);                 \
        (structp)->member.str = (char *)(istr);         \
    }

#define xdr_set_ref(structp, member, iiov, iniov, ilength) \
    {                                                               \
        (structp)->member.iov = (iiov);                             \
        (structp)->member.niov = (iniov);                           \
        (structp)->member.length = (ilength);                       \
    }

#ifdef XDR_CUSTOM_IOVEC
#define QUOTED(x) #x
#define TOSTRING(x) QUOTED(x)
#include TOSTRING(XDR_CUSTOM_IOVEC)
#else

typedef struct
{
    void *iov_base;
    uint32_t iov_len;
} xdr_iovec;

#define xdr_iovec_data(iov) ((iov)->iov_base)
#define xdr_iovec_len(iov) ((iov)->iov_len)

#define xdr_iovec_set_data(iov, ptr) ((iov)->iov_base = (ptr))
#define xdr_iovec_set_len(iov, len) ((iov)->iov_len = (len))

#define xdr_iovec_copy_private(out, in)
#define xdr_iovec_set_private_null(out)

#endif

typedef struct
{
    xdr_iovec *iov;
    int niov;
    uint32_t length;
} xdr_iovecr;

void
dump_output(
    const char *format,
    ...);

#endif
