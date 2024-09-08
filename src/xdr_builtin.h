#include <stdint.h>

#ifndef XDR_MAX_DBUF
#define XDR_MAX_DBUF 4096
#endif

#ifndef xdr_dbuf
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

#endif

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
