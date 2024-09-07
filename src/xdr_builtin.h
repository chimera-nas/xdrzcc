#include <stdint.h>

#ifndef xdr_iovec
struct xdr_iovec {
    void    *iov_base;
    uint32_t iov_len;
};

#define xdr_iovec_data(iov) ((iov)->iov_base)
#define xdr_iovec_len(iov) ((iov)->iov_len)

#define xdr_iovec_set_data(iov, ptr) ((iov)->iov_base = (ptr))
#define xdr_iovec_set_len(iov, len) ((iov)->iov_len = (len))

#endif
