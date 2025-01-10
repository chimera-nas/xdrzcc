/*
 * SPDX-FileCopyrightText: 2024 Ben Jarvis
 *
 * SPDX-License-Identifier: LGPL
 */

#include <assert.h>

#include "nested_xdr.h"

int
main(
    int   argc,
    char *argv[])
{
    struct MyMsg msg1, msg2;
    xdr_dbuf    *dbuf;
    uint8_t      buffer[256];
    xdr_iovec    iov_in, iov_out;
    int          rc, one = 1;

    xdr_iovec_set_data(&iov_in, buffer);
    xdr_iovec_set_len(&iov_in, sizeof(buffer));

    dbuf = xdr_dbuf_alloc(16 * 1024);

    msg1.value        = 42;
    msg1.inner1.value = 43;
    msg1.inner2.value = 44;

    rc = marshall_MyMsg(&msg1, &iov_in, &iov_out, &one, NULL, 0);

    assert(rc == 12);

    rc = unmarshall_MyMsg(&msg2, &iov_out, one, NULL, dbuf);

    assert(rc == 12);

    assert(msg1.value == msg2.value);
    assert(msg1.inner1.value == msg2.inner1.value);
    assert(msg1.inner2.value == msg2.inner2.value);

    xdr_dbuf_free(dbuf);

    return 0;
} /* main */
