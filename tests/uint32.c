#include <assert.h>

#include "uint32_xdr.h"

int main(int argc, char *argv[])
{
    struct MyMsg    msg1, msg2;
    uint8_t buffer[256];
    struct xdr_iovec iov;
    int rc;

    xdr_iovec_set_data(&iov, buffer);
    xdr_iovec_set_len(&iov, sizeof(buffer));

    msg1.value = 42;

    rc = marshall_MyMsg(&msg1, 1, &iov, 1);

    assert(rc == 4);

    rc = unmarshall_MyMsg(&msg2, 1, &iov, 1); 

    assert(rc == 4);

    assert(msg1.value == msg2.value);

    return 0;
}
