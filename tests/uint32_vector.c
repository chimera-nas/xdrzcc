// SPDX-FileCopyrightText: 2024 - 2025 Ben Jarvis
//
// SPDX-License-Identifier: LGPL-2.1-only

#include <assert.h>

#include "uint32_vector_xdr.h"

int
main(
    int   argc,
    char *argv[])
{
    struct MyMsg msg1, msg2;
    xdr_dbuf    *dbuf;
    uint8_t      buffer[256];
    xdr_iovec    iov_in, iov_out;
    int          rc, i, one = 1;

    xdr_iovec_set_data(&iov_in, buffer);
    xdr_iovec_set_len(&iov_in, sizeof(buffer));

    dbuf = xdr_dbuf_alloc(16 * 1024);

    msg1.num_value = 16;
    msg1.value = xdr_dbuf_alloc_space(16 * sizeof(*msg1.value), dbuf);
    if (msg1.value == NULL) {
        return 1;
    }

    for (i = 0; i < 16; ++i) {
        msg1.value[i] = i;
    }

    rc = marshall_MyMsg(&msg1, &iov_in, &iov_out, &one, NULL, 0);

    assert(rc == 68);

    rc = unmarshall_MyMsg(&msg2, &iov_out, one, NULL, dbuf);

    assert(rc == 68);

    assert(msg2.num_value == 16);

    for (i = 0; i < 16; ++i) {
        assert(msg2.value[i] == i);
    }

    xdr_dbuf_free(dbuf);

    return 0;
} /* main */
