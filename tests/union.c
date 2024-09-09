#include <assert.h>

#include "union_xdr.h"

int main(int argc, char *argv[])
{
    struct MyMsg    msg1, msg2;
    xdr_dbuf *dbuf;
    uint8_t buffer[256];
    xdr_iovec iov;
    int rc;

    xdr_iovec_set_data(&iov, buffer);
    xdr_iovec_set_len(&iov, sizeof(buffer));

    dbuf = xdr_dbuf_alloc();

    msg1.opt = OPTION1;
    msg1.option1.value = 42;

    rc = marshall_MyMsg(&msg1, 1, &iov, 1);

    assert(rc == 8);

    rc = unmarshall_MyMsg(&msg2, 1, &iov, 1, dbuf); 

    assert(rc == 8);

    assert(msg1.opt == msg2.opt);
    assert(msg1.option1.value == msg2.option1.value);

    xdr_dbuf_reset(dbuf);

    msg1.opt = OPTION2;
    xdr_dbuf_strncpy(&msg1.option2, value, "1234567", 7 ,dbuf);

    rc = marshall_MyMsg(&msg1, 1, &iov, 1);

    assert(rc == 16);

    rc = unmarshall_MyMsg(&msg2, 1, &iov, 1, dbuf);

    assert(rc == 16);

    assert(msg1.opt == msg2.opt);
    assert(strcmp(msg1.option2.value.str,msg2.option2.value.str) == 0);

    xdr_dbuf_reset(dbuf);

    msg1.opt = OPTION3;

    rc = marshall_MyMsg(&msg1, 1, &iov, 1);

    assert(rc == 4);

    rc = unmarshall_MyMsg(&msg2, 1, &iov, 1, dbuf);

    assert(rc == 4);

    assert(msg1.opt == msg2.opt);

    xdr_dbuf_free(dbuf);

    return 0;
}