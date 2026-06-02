// SPDX-FileCopyrightText: 2024 - 2026 Ben Jarvis
//
// SPDX-License-Identifier: LGPL-2.1-only

#include <assert.h>

#include "optional_xdr.h"

/*
 * Round-trip an OptMsg both with and without the optional pointer present.
 * The unmarshalled length must equal the marshalled length; a contig
 * unmarshaller that double-counts the pointed-to object would return a
 * length that is too long when opt is present (offset past the trailing
 * field), which is what breaks RPC-over-RDMA reply-chunk framing.
 */

static void
roundtrip(struct OptMsg *in)
{
    struct OptMsg out;
    xdr_dbuf     *dbuf;
    uint8_t       buffer[256];
    xdr_iovec     iov_in, iov_out;
    int           marshalled, unmarshalled, one = 1;

    xdr_iovec_set_data(&iov_in, buffer);
    xdr_iovec_set_len(&iov_in, sizeof(buffer));

    dbuf = xdr_dbuf_alloc(16 * 1024);

    marshalled = marshall_OptMsg(in, &iov_in, &iov_out, &one, NULL, 0);
    assert(marshalled > 0);

    /* one iovec out -> exercises the contig unmarshall path */
    unmarshalled = unmarshall_OptMsg(&out, &iov_out, one, NULL, dbuf);

    assert(unmarshalled == marshalled);

    assert(out.head == in->head);
    assert(out.tail == in->tail);

    if (in->opt) {
        assert(out.opt != NULL);
        assert(out.opt->a == in->opt->a);
    } else {
        assert(out.opt == NULL);
    }

    xdr_dbuf_free(dbuf);
} /* roundtrip */

int
main(
    int   argc,
    char *argv[])
{
    struct OptMsg   msg;
    struct OptInner inner;

    /* optional present */
    inner.a  = 20;
    msg.head = 10;
    msg.opt  = &inner;
    msg.tail = 30;
    roundtrip(&msg);

    /* optional absent */
    msg.head = 11;
    msg.opt  = NULL;
    msg.tail = 31;
    roundtrip(&msg);

    return 0;
} /* main */
