#include <assert.h>

#include "uint32_array_xdr.h"

int main(int argc, char *argv[])
{
    struct MyMsg    msg1, msg2;
    xdr_dbuf *dbuf;
    uint8_t buffer[256];
    xdr_iovec iov;
    int rc, i;

    xdr_iovec_set_data(&iov, buffer);
    xdr_iovec_set_len(&iov, sizeof(buffer));

    for (i = 0; i < 16; ++i) {
        msg1.value[i] = i;
    }

    dbuf = xdr_dbuf_alloc();

    rc = marshall_MyMsg(&msg1, 1, &iov, 1);

    assert(rc == 64);

    rc = unmarshall_MyMsg(&msg2, 1, &iov, 1, dbuf); 

    assert(rc == 64);

    for (i = 0; i < 16; ++i) {
        assert(msg2.value[i] == i);
    }

    xdr_dbuf_free(dbuf);

    return 0;
}
