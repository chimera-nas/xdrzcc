/*
 * SPDX-FileCopyrightText: 2024 - 2026 Ben Jarvis
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

struct OptInner {
    unsigned int a;
};

/*
 * A bare optional pointer (OptInner *opt) followed by a trailing field.
 * Regression coverage for the contig unmarshaller double-counting the
 * pointed-to length when the optional is present: the returned length must
 * match what was marshalled, otherwise a consumer that uses it to locate the
 * next item (e.g. the RPC body after an RPC-over-RDMA reply chunk) misparses.
 */
struct OptMsg {
    unsigned int     head;
    OptInner        *opt;
    unsigned int     tail;
};
