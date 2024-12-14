/*
 * SPDX-FileCopyrightText: 2024 Ben Jarvis
 *
 * SPDX-License-Identifier: LGPL
 */

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

    dbuf = xdr_dbuf_alloc();

    xdr_dbuf_strncpy(&msg1, string1, "1234567", 7, dbuf);
    xdr_dbuf_strncpy(&msg1, string2, "1234", 4, dbuf);
    xdr_dbuf_strncpy(&msg1, string3, "123456789", 9, dbuf);

    rc = marshall_MyMsg(&msg1, &iov_in, &iov_out, &one, 0);

    assert(rc == 36);

    rc = unmarshall_MyMsg(&msg2, &iov_out, one, dbuf);

    fprintf(stderr, "unmarshalled to %d bytes %d niov\n", rc, one);

    assert(rc == 36);

    assert(strcmp(msg1.string1.str, msg2.string1.str) == 0);
    assert(strcmp(msg1.string2.str, msg2.string2.str) == 0);
    assert(strcmp(msg1.string3.str, msg2.string3.str) == 0);

    xdr_dbuf_free(dbuf);

    return 0;
} /* main */
