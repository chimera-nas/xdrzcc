/*
 * SPDX-FileCopyrightText: 2024 Ben Jarvis
 *
 * SPDX-License-Identifier: LGPL
 */

#include <assert.h>

#include "double_xdr.h"

int main(int argc, char *argv[])
{
    struct MyMsg    msg1, msg2;
    xdr_dbuf *dbuf;
    uint8_t buffer[256];
    xdr_iovec iov_in, iov_out;
    int rc, one = 1;

    xdr_iovec_set_data(&iov_in, buffer);
    xdr_iovec_set_len(&iov_in, sizeof(buffer));

    dbuf = xdr_dbuf_alloc();

    msg1.value = 42.7;

    rc = marshall_MyMsg(&msg1, 1, &iov_in, 1, &iov_out, &one);

    assert(rc == 8);

    rc = unmarshall_MyMsg(&msg2, 1, &iov_out, 1, dbuf); 

    assert(rc == 8);

    assert(msg1.value == msg2.value);

    xdr_dbuf_free(dbuf);

    return 0;
}