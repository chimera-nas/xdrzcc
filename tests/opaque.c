// SPDX-FileCopyrightText: 2024 - 2025 Ben Jarvis
//
// SPDX-License-Identifier: LGPL-2.1-only

#include <assert.h>

#include "opaque_xdr.h"

int
main(
    int   argc,
    char *argv[])
{
    struct MyMsg msg1, msg2;
    xdr_dbuf    *dbuf;
    uint8_t      buffer[256], data[7];
    xdr_iovec    iov_in, iov_out[3], iov_data;
    int          i, rc, niov_out = 3;

    xdr_iovec_set_data(&iov_in, buffer);
    xdr_iovec_set_len(&iov_in, sizeof(buffer));

    xdr_iovec_set_data(&iov_data, data);
    xdr_iovec_set_len(&iov_data, 7);

    for (i = 0; i < 7; ++i) {
        data[i] = i;
    }

    dbuf = xdr_dbuf_alloc(16 * 1024);

    xdr_set_ref(&msg1, data, &iov_data, 1, 7);

    assert(xdr_iovec_len(msg1.data.iov) == 7);
    assert(memcmp(xdr_iovec_data(msg1.data.iov), data, 7) == 0);

    rc = marshall_MyMsg(&msg1, &iov_in, iov_out, &niov_out, NULL, 0);

    assert(niov_out == 3);
    assert(rc == 12);

    fprintf(stderr, "marshalled to %d bytes %d niov\n", rc, niov_out);
    for (int i = 0; i < niov_out; ++i) {
        fprintf(stderr, "iov_out[%d] = %d\n", i, xdr_iovec_len(&iov_out[i]));
    }
    rc = unmarshall_MyMsg(&msg2, iov_out, niov_out, NULL, dbuf);

    assert(rc == 12);

    assert(xdr_iovec_len(msg1.data.iov) == xdr_iovec_len(msg2.data.iov));

    assert(memcmp(xdr_iovec_data(msg2.data.iov), data, 7) == 0);

    xdr_dbuf_free(dbuf);

    return 0;
} /* main */
