// SPDX-FileCopyrightText: 2024 - 2025 Ben Jarvis
//
// SPDX-License-Identifier: LGPL-2.1-only

#include <assert.h>

#include "string_xdr.h"

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

    msg1.string1.len = 7;
    msg1.string1.str = (char *)xdr_dbuf_alloc_space(7 + 1, dbuf);
    if (msg1.string1.str == NULL) {
        return 1;
    }
    memcpy(msg1.string1.str, "1234567", 7 + 1);

    msg1.string2.len = 4;
    msg1.string2.str = (char *)xdr_dbuf_alloc_space(4 + 1, dbuf);
    if (msg1.string2.str == NULL) {
        return 1;
    }
    memcpy(msg1.string2.str, "1234", 4 + 1);

    msg1.string3.len = 9;
    msg1.string3.str = (char *)xdr_dbuf_alloc_space(9 + 1, dbuf);
    if (msg1.string3.str == NULL) {
        return 1;
    }
    memcpy(msg1.string3.str, "123456789", 9 + 1);

    rc = marshall_MyMsg(&msg1, &iov_in, &iov_out, &one, NULL, 0);

    assert(rc == 36);

    rc = unmarshall_MyMsg(&msg2, &iov_out, one, NULL, dbuf);

    fprintf(stderr, "unmarshalled to %d bytes %d niov\n", rc, one);

    assert(rc == 36);

    assert(strcmp(msg1.string1.str, msg2.string1.str) == 0);
    assert(strcmp(msg1.string2.str, msg2.string2.str) == 0);
    assert(strcmp(msg1.string3.str, msg2.string3.str) == 0);

    xdr_dbuf_free(dbuf);

    return 0;
} /* main */
