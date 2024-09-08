#include <assert.h>

#include "uint32_vector_one_xdr.h"

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

    xdr_dbuf_reserve(&msg1, value, 1, dbuf);

    msg1.value[0] = 42;

    rc = marshall_MyMsg(&msg1, 1, &iov, 1);

    assert(rc == 8);

    rc = unmarshall_MyMsg(&msg2, 1, &iov, 1, dbuf); 

    assert(rc == 8);

    assert(msg2.num_value == 1);
    assert(msg2.value[0] == 42);

    xdr_dbuf_free(dbuf);

    return 0;
}
