#include <assert.h>

#include "uint32_vector_xdr.h"

int main(int argc, char *argv[])
{
    struct MyMsg    msg1, msg2;
    xdr_dbuf *dbuf;
    uint8_t buffer[256];
    xdr_iovec iov;
    int rc, i;

    xdr_iovec_set_data(&iov, buffer);
    xdr_iovec_set_len(&iov, sizeof(buffer));

    dbuf = xdr_dbuf_alloc();

    xdr_dbuf_reserve(&msg1, value, 16, dbuf);

    for (i = 0; i < 16; ++i) {
        msg1.value[i] = i;
    }

    rc = marshall_MyMsg(&msg1, 1, &iov, 1);

    assert(rc == 68);

    rc = unmarshall_MyMsg(&msg2, 1, &iov, 1, dbuf); 

    assert(rc == 68);

    assert(msg2.num_value == 16);

    for (i = 0; i < 16; ++i) {
        assert(msg2.value[i] == i);
    }

    xdr_dbuf_free(dbuf);

    return 0;
}
