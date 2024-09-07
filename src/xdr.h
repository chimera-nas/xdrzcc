#pragma once

#include "uthash.h"
#include "utlist.h"

#define XDR_TYPEDEF 1
#define XDR_ENUM 2
#define XDR_STRUCT 3
#define XDR_UNION 4
#define XDR_CONST 5

struct xdr_const {
    char *name;
    char *value;
    struct xdr_const *prev;
    struct xdr_const *next;
};

struct xdr_type {
    char *name;
    char *array_size;
    char *vector_bound;
    int builtin;
    int pointer;
    int vector;
    int array;
    int opaque;
};

struct xdr_typedef {
    struct xdr_type *type;
    char *name;
    struct xdr_typedef *prev;
    struct xdr_typedef *next;
};

struct xdr_enum {
    char *name;
    struct xdr_enum_entry *entries;
    struct xdr_enum *prev;
    struct xdr_enum *next;
};
    
struct xdr_enum_entry {
    char *name;
    char *value;
    struct xdr_enum_entry *prev;
    struct xdr_enum_entry *next;
};

struct xdr_struct_member {
    struct xdr_type *type;
    char   *name;
    struct xdr_struct_member *prev;
    struct xdr_struct_member *next;
};

struct xdr_struct {
    char *name;
    struct xdr_struct_member *members;
    struct xdr_struct *prev;
    struct xdr_struct *next;
};

struct xdr_union_case {
    char *label;
    struct xdr_type *type;
    char   *name;
    int voided;
    struct xdr_union_case *prev;
    struct xdr_union_case *next;
};

struct xdr_union {
    char *name;
    struct xdr_type *pivot_type;
    char   *pivot_name;
    struct xdr_union_case *cases;
    struct xdr_union *prev;
    struct xdr_union *next;

};
