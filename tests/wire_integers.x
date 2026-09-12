/* SPDX-FileCopyrightText: 2026 Ben Jarvis
 * SPDX-License-Identifier: LGPL-2.1-only */
enum WireEnum { WIRE_VALUE = 0x1234 };

struct WireIntegers {
    uint32_t u32;
    uint64_t u64;
    int32_t i32;
    int64_t i64;
    WireEnum enumeration;
};
