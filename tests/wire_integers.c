// SPDX-FileCopyrightText: 2026 Ben Jarvis
// SPDX-License-Identifier: LGPL-2.1-only

#include <assert.h>
#include "wire_integers_xdr.h"

#ifdef NDEBUG
#error Tests require assertions in every build configuration
#endif /* ifdef NDEBUG */

int
main(void)
{
    static const unsigned char expected[] = {
        0x89, 0xab, 0xcd, 0xef,
        0x01, 0x23, 0x45, 0x67,0x89,  0xab,  0xcd,  0xef,
        0xff, 0xff, 0xff, 0xfe,
        0xff, 0xff, 0xff, 0xff,0xff,  0xff,  0xff,  0xfe,
        0x00, 0x00, 0x12, 0x34
    };
    struct WireIntegers        in = { UINT32_C(0x89abcdef), UINT64_C(0x0123456789abcdef), -2, -2, WIRE_VALUE };
    struct WireIntegers        out;
    unsigned char              buffer[64];
    xdr_iovec                  scratch = { buffer, sizeof(buffer) };
    xdr_iovec                  encoded, fragmented[2];
    xdr_dbuf                  *dbuf = xdr_dbuf_alloc(1024);
    int                        niov = 1;
    int                        rc;

    rc = marshall_WireIntegers(&in, &scratch, &encoded, &niov, NULL, 0);
    assert(rc == sizeof(expected));
    assert(niov == 1);
    assert(memcmp(encoded.iov_base, expected, sizeof(expected)) == 0);

    rc = unmarshall_WireIntegers(&out, &encoded, 1, NULL, dbuf);
    assert(rc == sizeof(expected));
    assert(out.u32 == in.u32 && out.u64 == in.u64);
    assert(out.i32 == in.i32 && out.i64 == in.i64);
    assert(out.enumeration == WIRE_VALUE);

    fragmented[0].iov_base = buffer;
    fragmented[0].iov_len  = 5;
    fragmented[1].iov_base = buffer + 5;
    fragmented[1].iov_len  = sizeof(expected) - 5;
    rc                     = unmarshall_WireIntegers(&out, fragmented, 2, NULL, dbuf);
    assert(rc == sizeof(expected));
    assert(out.u32 == in.u32 && out.u64 == in.u64);
    assert(out.i32 == in.i32 && out.i64 == in.i64);
    assert(out.enumeration == WIRE_VALUE);

    xdr_dbuf_free(dbuf);
    return 0;
} /* main */
