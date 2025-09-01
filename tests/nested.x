/*
 * SPDX-FileCopyrightText: 2024 - 2025 Ben Jarvis
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

struct MyInner1 {
    unsigned int value;
};

struct MyInner2 {
    unsigned int value;
};

struct MyMsg {
    unsigned int    value;
    MyInner1        inner1;
    MyInner2        inner2;
};
