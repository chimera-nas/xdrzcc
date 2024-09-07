#include <string.h>
#include <arpa/inet.h>

struct xdr_cursor {
    struct xdr_iovec *cur;
    struct xdr_iovec *last;
    unsigned int      offset;
};

static inline void
xdr_cursor_init(
    struct xdr_cursor *cursor,
    struct xdr_iovec *iov,
    int niov)
{
    cursor->cur    = iov;
    cursor->last   = iov + (niov - 1);
    cursor->offset = 0;
}

static inline int
xdr_cursor_extract(
    void *out,
    struct xdr_cursor *cursor,
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
                return 1;
            }

            cursor->cur++;
            cursor->offset = 0;
        }
    }

    return 0;
}

static inline int
__unmarshall_uint32_t(
    uint32_t *v,
    int n,
    struct xdr_cursor *cursor)
{
    return 0;
}

static inline int
__unmarshall_int32_t(
    int32_t *v,
    int n,
    struct xdr_cursor *cursor)
{
    return 0;
}

static inline int
__unmarshall_uint64_t(
    uint64_t *v,
    int n,
    struct xdr_cursor *cursor)
{
    return 0;
}

static inline int
__unmarshall_int64_t(
    int64_t *v,
    int n,
    struct xdr_cursor *cursor)
{
    return 0;
}

static inline int
__unmarshall_struct_xdr_iovec(
    struct xdr_iovec *v,
    int n,
    struct xdr_cursor *cursor)
{
    return 0;
}
