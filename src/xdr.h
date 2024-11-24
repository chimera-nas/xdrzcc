/*
 * SPDX-FileCopyrightText: 2024 Ben Jarvis
 *
 * SPDX-License-Identifier: LGPL
 */

#pragma once

#include "uthash.h"
#include "utlist.h"

#define XDR_TYPEDEF 1
#define XDR_ENUM    2
#define XDR_STRUCT  3
#define XDR_UNION   4
#define XDR_CONST   5

struct xdr_const {
    char             *name;
    char             *value;
    struct xdr_const *prev;
    struct xdr_const *next;
};

struct xdr_type {
    char *name;
    char *array_size;
    char *vector_bound;
    int   optional;
    int   opaque;
    int   zerocopy;
    int   builtin;
    int   vector;
    int   array;
};

struct xdr_typedef {
    struct xdr_type    *type;
    char               *name;
    struct xdr_typedef *prev;
    struct xdr_typedef *next;
};

struct xdr_enum {
    char                  *name;
    struct xdr_enum_entry *entries;
    struct xdr_enum       *prev;
    struct xdr_enum       *next;
};

struct xdr_enum_entry {
    char                  *name;
    char                  *value;
    struct xdr_enum_entry *prev;
    struct xdr_enum_entry *next;
};

struct xdr_struct_member {
    struct xdr_type          *type;
    char                     *name;
    struct xdr_struct_member *prev;
    struct xdr_struct_member *next;
};

struct xdr_struct {
    char                     *name;
    int                       linkedlist;
    struct xdr_struct_member *members;
    struct xdr_struct        *prev;
    struct xdr_struct        *next;
};

struct xdr_union_case {
    char                  *label;
    struct xdr_type       *type;
    char                  *name;
    int                    voided;
    struct xdr_union_case *prev;
    struct xdr_union_case *next;
};

struct xdr_union {
    char                  *name;
    struct xdr_type       *pivot_type;
    char                  *pivot_name;
    struct xdr_union_case *cases;
    struct xdr_union_case *default_case;
    struct xdr_union      *prev;
    struct xdr_union      *next;

};

struct xdr_function {
    const char          *id;
    const char          *name;
    struct xdr_type     *call_type;
    struct xdr_type     *reply_type;
    struct xdr_function *prev;
    struct xdr_function *next;
};

struct xdr_version {
    const char          *id;
    const char          *name;
    struct xdr_function *functions;
    struct xdr_version  *prev;
    struct xdr_version  *next;
};


struct xdr_program {
    const char         *id;
    const char         *name;
    struct xdr_version *versions;
    struct xdr_program *prev;
    struct xdr_program *next;
};
