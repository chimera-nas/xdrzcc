/*
 * SPDX-FileCopyrightText: 2024 - 2025 Ben Jarvis
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

struct MyMsg {
    zcopaque  data<>;
};

typedef opaque fixed_id[16];

struct FixedMsg {
    opaque odd[3];
    opaque verifier[8];
    fixed_id id;
    unsigned int tail;
};
