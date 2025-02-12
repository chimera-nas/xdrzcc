/*
 * SPDX-FileCopyrightText: 2024 Ben Jarvis
 *
 * SPDX-License-Identifier: LGPL
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include "y.tab.h"

#include "xdr.h"

extern FILE        *yyin;

extern int yyparse();

extern const char  *embedded_builtin_c;
extern const char  *embedded_builtin_h;

struct xdr_struct  *xdr_structs  = NULL;
struct xdr_union   *xdr_unions   = NULL;
struct xdr_typedef *xdr_typedefs = NULL;
struct xdr_enum    *xdr_enums    = NULL;
struct xdr_const   *xdr_consts   = NULL;
struct xdr_program *xdr_programs = NULL;

struct xdr_buffer {
    void              *data;
    unsigned int       used;
    unsigned int       size;
    struct xdr_buffer *prev;
    struct xdr_buffer *next;
};

struct xdr_buffer *xdr_buffers = NULL;

struct xdr_identifier {
    char                 *name;
    int                   type;
    int                   emitted;
    void                 *ptr;
    struct UT_hash_handle hh;
};

struct xdr_identifier *xdr_identifiers = NULL;

void *
xdr_alloc(unsigned int size)
{
    struct xdr_buffer *xdr_buffer = xdr_buffers;
    void              *ptr;

    size += (8 - (size & 7)) & 7;

    if (xdr_buffer == NULL || (xdr_buffer->size - xdr_buffer->used) < size) {
        xdr_buffer       = calloc(1, sizeof(*xdr_buffer));
        xdr_buffer->size = 2 * 1024 * 1024;
        xdr_buffer->used = 0;
        xdr_buffer->data = calloc(1, xdr_buffer->size);
        DL_PREPEND(xdr_buffers, xdr_buffer);
    }

    ptr = xdr_buffer->data + xdr_buffer->used;

    xdr_buffer->used += size;

    return ptr;
} /* xdr_alloc */

char *
xdr_strdup(const char *str)
{
    int   len = strlen(str) + 1;
    char *out = xdr_alloc(len);

    memcpy(out, str, len);

    return out;
} /* xdr_strdup */

void
xdr_add_identifier(
    int   type,
    char *name,
    void *ptr)
{
    struct xdr_identifier *ident;

    HASH_FIND_STR(xdr_identifiers, name, ident);

    if (ident) {
        fprintf(stderr, "Duplicate symbol '%s' found.\n", name);
        exit(1);
    }

    ident = xdr_alloc(sizeof(*ident));

    ident->type = type;
    ident->name = name;
    ident->ptr  = ptr;

    HASH_ADD_STR(xdr_identifiers, name, ident);
} /* xdr_add_identifier */
void
emit_marshall(
    FILE            *output,
    const char      *name,
    struct xdr_type *type)
{
    struct xdr_identifier *chk;
    struct xdr_struct     *liststruct;

    if (type->opaque) {
        if (type->array) {
            fprintf(output,
                    "    xdr_write_cursor_append(cursor, in->%s, %s);\n",
                    name, type->array_size);
        } else if (type->zerocopy) {
            fprintf(output,
                    "    __marshall_opaque_zerocopy(&in->%s, cursor);\n",
                    name);
        } else {
            fprintf(output,
                    "    __marshall_opaque(&in->%s, %s, cursor);\n",
                    name, type->vector_bound ? type->vector_bound : "0");
        }
    } else if (strcmp(type->name, "xdr_string") == 0) {
        fprintf(output,
                "    __marshall_xdr_string(&in->%s, cursor);\n",
                name);
    } else if (type->linkedlist) {

        HASH_FIND_STR(xdr_identifiers, type->name, chk);

        if (!chk) {
            fprintf(stderr, "Linked list '%s' not found.\n", type->name);
            exit(1);
        }

        liststruct = (struct xdr_struct *) chk->ptr;

        fprintf(output, "    {\n");
        fprintf(output, "            uint32_t more;\n");
        fprintf(output, "        const struct %s *current = in->%s;\n", type->
                name, name);
        fprintf(output, "        while (current != NULL) {\n");
        fprintf(output, "            more = 1;\n");
        fprintf(output,
                "            __marshall_uint32_t(&more, cursor);\n");
        fprintf(output,
                "            __marshall_%s(current, cursor);\n",
                type->name);
        fprintf(output, "            current = current->%s;\n", liststruct->
                nextmember);
        fprintf(output, "        }\n");
        fprintf(output, "        more = 0;\n");
        fprintf(output, "        __marshall_uint32_t(&more, cursor);\n");
        fprintf(output, "    }\n");
    } else if (type->optional) {
        fprintf(output, "    {\n");
        fprintf(output, "        uint32_t more = !!(in->%s);\n", name);
        fprintf(output,
                "        __marshall_uint32_t(&more, cursor);\n");
        fprintf(output, "        if (more) {\n");
        fprintf(output,
                "        __marshall_%s(in->%s, cursor);\n",
                type->name, name);
        fprintf(output, "        }\n");
        fprintf(output, "    }\n");
    } else if (type->vector) {
        fprintf(output,
                "    __marshall_uint32_t(&in->num_%s, cursor);\n",
                name);
        fprintf(output, "    for (int i = 0; i < in->num_%s; i++) {\n", name);
        fprintf(output, "        __marshall_%s(&in->%s[i], cursor);\n",
                type->name, name);
        fprintf(output, "    }\n");
    } else if (type->array) {
        fprintf(output, "    for (int i = 0; i < %s; ++i) {\n",
                type->array_size);
        fprintf(output, "        __marshall_%s(&in->%s[i], cursor);\n",
                type->name, name);
        fprintf(output, "    }\n");
    } else {
        fprintf(output, "    __marshall_%s(&in->%s, cursor);\n",
                type->name, name);
    }
} /* emit_marshall */

void
emit_unmarshall(
    FILE            *output,
    const char      *name,
    struct xdr_type *type)
{
    struct xdr_identifier *chk;
    struct xdr_struct     *liststruct;

    if (type->opaque) {
        if (type->array) {
            fprintf(output,
                    "    rc = xdr_read_cursor_extract(cursor, out->%s, %s);\n",
                    name, type->array_size);
        } else if (type->zerocopy) {
            fprintf(output,
                    "    rc = __unmarshall_opaque_zerocopy(&out->%s, cursor, dbuf);\n",
                    name);
        } else {
            fprintf(output,
                    "    rc = __unmarshall_opaque(&out->%s, %s, cursor, dbuf);\n",
                    name, type->vector_bound ? type->vector_bound : "0");
        }
    } else if (strcmp(type->name, "xdr_string") == 0) {
        fprintf(output,
                "    rc = __unmarshall_%s(&out->%s, cursor, dbuf);\n",
                type->name, name);
    } else if (type->linkedlist) {

        HASH_FIND_STR(xdr_identifiers, type->name, chk);

        if (!chk) {
            fprintf(stderr, "Linked list '%s' not found.\n", type->name);
            exit(1);
        }

        liststruct = (struct xdr_struct *) chk->ptr;

        fprintf(output, "    {\n");
        fprintf(output, "            uint32_t more;\n");
        fprintf(output,
                "        rc = __unmarshall_uint32_t(&more, cursor, dbuf);\n")
        ;
        fprintf(output, "        if (unlikely(rc < 0)) return rc;\n");
        fprintf(output, "        len += rc;\n");

        fprintf(output, "        out->%s = NULL;\n", name);
        fprintf(output, "        struct %s *current = NULL, *last = NULL;\n", type->name);
        fprintf(output, "        while (more) {\n");
        fprintf(output, "          xdr_dbuf_alloc_space(current, sizeof(*current), dbuf);\n");
        fprintf(output,
                "        rc = __unmarshall_%s(current, cursor, dbuf);\n",
                type->name);
        fprintf(output, "         if (unlikely(rc < 0)) return rc;\n");
        fprintf(output, "         len += rc;\n");
        fprintf(output, "         if (last) {\n");
        fprintf(output, "             last->%s = current;\n", liststruct->nextmember);
        fprintf(output, "         } else {\n");
        fprintf(output, "             out->%s = current;\n", name);
        fprintf(output, "        }\n");
        fprintf(output, "         last = current;\n");
        fprintf(output, "        last->%s = NULL;\n", liststruct->nextmember);
        fprintf(output, "         rc = __unmarshall_uint32_t(&more, cursor, dbuf);\n");
        fprintf(output, "         if (unlikely(rc < 0)) return rc;\n");
        fprintf(output, "         len += rc;\n");
        fprintf(output, "        }\n");
        fprintf(output, "        rc = 0;\n");
        fprintf(output, "    }\n");
    } else if (type->optional) {
        fprintf(output, "    {\n");
        fprintf(output, "        uint32_t more;\n");
        fprintf(output,
                "        rc = __unmarshall_uint32_t(&more, cursor, dbuf);\n")
        ;
        fprintf(output, "        if (unlikely(rc < 0)) return rc;\n");
        fprintf(output, "        len += rc;\n");
        fprintf(output, "        rc = 0;\n");
        fprintf(output, "        if (more) {\n");
        fprintf(output, "         xdr_dbuf_alloc_space(out->%s, sizeof(*out->%s), dbuf);\n", name, name);
        fprintf(output,
                "        rc = __unmarshall_%s(out->%s, cursor, dbuf);\n",
                type->name, name);
        fprintf(output, "        } else {\n");
        fprintf(output, "            out->%s = NULL;\n", name);
        fprintf(output, "        };\n");
        fprintf(output, "    }\n");
    } else if (type->vector) {
        fprintf(output,
                "    rc = __unmarshall_uint32_t(&out->num_%s, cursor, dbuf);\n",
                name);
        fprintf(output, "    if (unlikely(rc < 0)) return rc;\n");
        fprintf(output, "    len += rc;\n");
        fprintf(output, "     xdr_dbuf_reserve(out, %s, out->num_%s, dbuf);\n",
                name, name);
        fprintf(output, "    for (int i = 0; i < out->num_%s; i++) {\n", name);
        fprintf(output,
                "    rc = __unmarshall_%s(&out->%s[i], cursor, dbuf);\n",
                type->name, name);
        fprintf(output, "        if (unlikely(rc < 0)) return rc;\n");
        fprintf(output, "        len += rc;\n");
        fprintf(output, "    }\n");
        fprintf(output, "    rc = 0;\n");
    } else if (type->array) {
        fprintf(output, "    for (int i = 0; i < %s; i++) {\n",
                type->array_size);
        fprintf(output,
                "    rc = __unmarshall_%s(&out->%s[i], cursor, dbuf);\n",
                type->name, name);
        fprintf(output, "        if (unlikely(rc < 0)) return rc;\n");
        fprintf(output, "        len += rc;\n");
        fprintf(output, "    }\n");
        fprintf(output, "    rc = 0;\n");
    } else {
        fprintf(output,
                "    rc = __unmarshall_%s(&out->%s, cursor, dbuf);\n",
                type->name, name);
    }

    fprintf(output, "    if (unlikely(rc < 0)) return rc;\n");
    fprintf(output, "    len += rc;\n");
} /* emit_unmarshall */

void
emit_internal_headers(
    FILE       *source,
    const char *name)
{

    fprintf(source, "static void\n");
    fprintf(source, "__marshall_%s(\n", name);
    fprintf(source, "    const struct %s *in,\n", name);
    fprintf(source, "    struct xdr_write_cursor *cursor);\n\n");

    fprintf(source, "static int\n");
    fprintf(source, "__unmarshall_%s(\n", name);
    fprintf(source, "    struct %s *out,\n", name);
    fprintf(source, "    struct xdr_read_cursor *cursor,\n");
    fprintf(source, "    xdr_dbuf *dbuf);\n\n");

    fprintf(source, "static int\n");
    fprintf(source, "__marshall_length_%s(\n", name);
    fprintf(source, "    const struct %s *in);\n", name);
} /* emit_internal_headers */

void
emit_wrapper_headers(
    FILE       *header,
    const char *name)
{
    fprintf(header, "int marshall_%s(\n", name);
    fprintf(header, "    const struct %s *in,\n", name);
    fprintf(header, "    xdr_iovec *iov_in,\n");
    fprintf(header, "    xdr_iovec *iov_out,\n");
    fprintf(header, "    int *niov_out,\n");
    fprintf(header, "    struct evpl_rpc2_rdma_chunk *write_chunk,\n");
    fprintf(header, "    int out_offset);\n\n");

    fprintf(header, "int unmarshall_%s(\n", name);
    fprintf(header, "    struct %s *out,\n", name);
    fprintf(header, "    const xdr_iovec *iov,\n");
    fprintf(header, "    int niov,\n");
    fprintf(header, "    struct evpl_rpc2_rdma_chunk *read_chunk,\n");
    fprintf(header, "    xdr_dbuf *dbuf);\n\n");

    fprintf(header, "int marshall_length_%s(const struct %s *in);\n\n", name, name);
} /* emit_wrapper_headers */

void
emit_dump_headers(
    FILE       *header,
    const char *name)
{
    fprintf(header,
            "void dump_%s(const char *name, const struct %s *in);\n\n", name,
            name);
} /* emit_dump_headers */

void
emit_dump_member(
    FILE            *source,
    const char      *name,
    struct xdr_type *type)
{
    if (type->builtin) {
        if (strcmp(type->name, "uint32_t") == 0) {
            if (type->vector) {
                fprintf(source,
                        "    dump_output(\"%%s.num_%s = %%u\", subprefix, in->num_%s);\n",
                        name, name);
                fprintf(source, "    for (int i = 0; i < in->num_%s; i++) {\n",
                        name);
                fprintf(source,
                        "        char subsubprefix[160];\n");
                fprintf(source,
                        "        snprintf(subsubprefix, sizeof(subsubprefix), \"%%s.%s[%%d]\", subprefix, i);\n",
                        name);
                fprintf(source,
                        "        dump_output(\"%%s.%s = %%08x\", subsubprefix, in->%s[i]);\n",
                        name, name);
                fprintf(source, "    }\n");
            } else {
                fprintf(source,
                        "    dump_output(\"%%s.%s = %%08x\", subprefix, in->%s);\n",
                        name, name);
            }
        } else if (type->opaque) {
            fprintf(source, "    {\n");
            fprintf(source, "        char opaquestr[80];\n");
            if (type->zerocopy) {
                fprintf(source,
                        "    dump_output(\"%%s.%s = <opaque> [%%u bytes]\", subprefix, in->%s.length);\n",
                        name, name);
            } else if (type->array) {
                fprintf(source,
                        "        dump_opaque(opaquestr, sizeof(opaquestr), in->%s, %s);\n",
                        name, type->array_size);
                fprintf(source,
                        "    dump_output(\"%%s.%s = %%s [%%u bytes]\", subprefix, opaquestr, %s);\n",
                        name, type->array_size);
            } else {
                fprintf(source,
                        "        dump_opaque(opaquestr, sizeof(opaquestr), in->%s.data, in->%s.len);\n",
                        name, name);
                fprintf(source,
                        "    dump_output(\"%%s.%s = %%s [%%u bytes]\", subprefix, opaquestr, in->%s.len);\n",
                        name, name);
            }
            fprintf(source, "    }\n");
        } else {
            fprintf(source,
                    "    dump_output(\"%%s.%s = builtin\", subprefix);\n",
                    name);
        }
    } else {
        if (type->enumeration) {
            fprintf(source,
                    "    dump_output(\"%%s.%s = enum\", subprefix);\n",
                    name);
        } else if (type->opaque) {
            fprintf(source,
                    "    dump_output(\"%%s.%s = opaque\", subprefix);\n",
                    name);
        } else if (type->array) {
            fprintf(source,
                    "    dump_output(\"%%s.%s = array\", subprefix);\n",
                    name);
        } else if (type->vector) {
            fprintf(source,
                    "    dump_output(\"%%s.num_%s = %%u\", subprefix, in->num_%s);\n",
                    name, name);
            fprintf(source, "   for (int i = 0; i < in->num_%s; i++) {\n", name)
            ;
            fprintf(source, "       char subsubprefix[80];\n");
            fprintf(source,
                    "       snprintf(subsubprefix, sizeof(subsubprefix), \"%%s.%s[%%d]\", subprefix, i);\n",
                    name);
            fprintf(source,
                    "       _dump_%s(subsubprefix, \"%s\", &in->%s[i]);\n",
                    type->name, name, name);
            fprintf(source, "   }\n");
        } else if (type->optional) {
            fprintf(source,
                    "    dump_output(\"%%s.%s = optional\", subprefix);\n",
                    name);
        } else {
            fprintf(source,
                    "    _dump_%s(subprefix, \"%s\", &in->%s);\n",
                    type->name, name, name);
        }
    }
} /* emit_dump_member */

void
emit_length_member(
    FILE            *source,
    const char      *name,
    struct xdr_type *type)
{
    struct xdr_type       *emit_type;
    struct xdr_identifier *chk;

    HASH_FIND_STR(xdr_identifiers, type->name, chk);

    if (chk && chk->type == XDR_TYPEDEF) {
        emit_type = ((struct xdr_typedef *) chk->ptr)->type;
    } else {
        emit_type = type;
    }

    if (emit_type->opaque) {
        if (emit_type->array) {
            fprintf(source, "    length += xdr_pad(%s);\n", emit_type->array_size);
        } else if (emit_type->zerocopy) {
            fprintf(source, "    length += 4 + xdr_pad(in->%s.length);\n", name);
        } else {
            fprintf(source, "    length += 4 + xdr_pad(in->%s.len);\n", name);
        }
    } else if (strcmp(emit_type->name, "xdr_string") == 0) {
        fprintf(source, "    length += 4 + xdr_pad(in->%s.len);\n", name);
    } else if (emit_type->vector) {
        fprintf(source, "    length += 4;\n");
        fprintf(source, "    for (int i = 0; i < in->num_%s; i++) {\n", name);
        fprintf(source, "        length += __marshall_length_%s(&in->%s[i]);\n", type->name, name);
        fprintf(source, "    }\n");
    } else if (emit_type->optional) {
        fprintf(source, "    length += 4;\n");
        fprintf(source, "    if (in->%s) {\n", name);
        fprintf(source, "        length += __marshall_length_%s(in->%s);\n", type->name, name);
        fprintf(source, "    }\n");
    } else if (emit_type->array) {
        fprintf(source, "    for (int i = 0; i < %s; i++) {\n", emit_type->array_size);
        fprintf(source, "        length += __marshall_length_%s(&in->%s[i]);\n", type->name, name);
        fprintf(source, "    }\n");
    } else {
        fprintf(source, "    length += __marshall_length_%s(&in->%s);\n", type->name, name);
    }
} /* emit_length_member */

void
emit_dump_internal(
    FILE       *source,
    const char *name)
{
    fprintf(source,
            "static void _dump_%s(const char *prefix, const char *name, const struct %s *in);\n",
            name, name);
} /* emit_dump_internal */

void
emit_dump_struct(
    FILE              *source,
    const char        *name,
    struct xdr_struct *xdr_structp)
{
    struct xdr_struct_member *member;

    fprintf(source,
            "static void _dump_%s(const char *prefix, const char *name, const struct %s *in)\n",
            name, name);

    fprintf(source, "{\n");
    fprintf(source, "    char subprefix[80];\n");
    fprintf(source,
            "    snprintf(subprefix, sizeof(subprefix), \"%%s%%s%%s\", "
            "prefix, prefix[0] ? \".\" : \"\", name);\n");

    DL_FOREACH(xdr_structp->members, member)
    {
        emit_dump_member(source, member->name, member->type);
    }
    fprintf(source, "}\n\n");

    fprintf(source, "void dump_%s(const char *name, const struct %s *in)\n",
            name,
            name);
    fprintf(source, "{\n");
    fprintf(source, "    _dump_%s(\"\", \"%s\", in);\n", name, name);
    fprintf(source, "}\n\n");
} /* emit_dump_struct */

void
emit_length_struct(
    FILE              *source,
    const char        *name,
    struct xdr_struct *xdr_structp)
{
    struct xdr_struct_member *member;

    fprintf(source,
            "static int __marshall_length_%s(const struct %s *in)\n",
            name, name);

    fprintf(source, "{\n");
    fprintf(source, "    uint32_t length = 0;\n");

    DL_FOREACH(xdr_structp->members, member)
    {
        emit_length_member(source, member->name, member->type);
    }
    fprintf(source, "    return length;\n");
    fprintf(source, "}\n\n");

    fprintf(source, "int marshall_length_%s(const struct %s *in)\n",
            name, name);
    fprintf(source, "{\n");
    fprintf(source, "    return __marshall_length_%s(in);\n", name);
    fprintf(source, "}\n\n");
} /* emit_length_struct */

void
emit_dump_union(
    FILE             *source,
    const char       *name,
    struct xdr_union *xdr_unionp)
{
    struct xdr_union_case *casep;

    fprintf(source,
            "static void _dump_%s(const char *prefix, const char *name, const struct %s *in)\n",
            name, name);
    fprintf(source, "{\n");
    fprintf(source, "    char subprefix[80];\n");
    fprintf(source,
            "    snprintf(subprefix, sizeof(subprefix), \"%%s%%s%%s\", "
            "prefix, prefix[0] ? \".\" : \"\", name);\n");
    emit_dump_member(source, xdr_unionp->pivot_name, xdr_unionp->pivot_type);
    fprintf(source, "    switch (in->%s) {\n", xdr_unionp->pivot_name);

    DL_FOREACH(xdr_unionp->cases, casep)
    {
        if (strcmp(casep->label, "default") != 0) {
            fprintf(source, "    case %s:\n", casep->label);
            if (casep->type) {
                emit_dump_member(source, casep->name, casep->type);
            }
            fprintf(source, "        break;\n");
        }
    }

    DL_FOREACH(xdr_unionp->cases, casep)
    {
        if (strcmp(casep->label, "default") == 0) {
            fprintf(source, "    default:\n");
            fprintf(source, "        break;\n");
        }
    }

    fprintf(source, "    }\n");
    fprintf(source, "}\n\n");

    fprintf(source, "void dump_%s(const char *name, const struct %s *in)\n",
            name, name);
    fprintf(source, "{\n");
    fprintf(source, "    _dump_%s(\"\", \"%s\", in);\n", name, name);
    fprintf(source, "}\n\n");
} /* emit_dump_union */

void
emit_length_union(
    FILE             *source,
    const char       *name,
    struct xdr_union *xdr_unionp)
{
    struct xdr_union_case *casep;

    fprintf(source,
            "static int __marshall_length_%s(const struct %s *in)\n",
            name, name);
    fprintf(source, "{\n");
    fprintf(source, "    uint32_t length = 0;\n");
    emit_length_member(source, xdr_unionp->pivot_name, xdr_unionp->pivot_type);
    fprintf(source, "    switch (in->%s) {\n", xdr_unionp->pivot_name);

    DL_FOREACH(xdr_unionp->cases, casep)
    {
        if (strcmp(casep->label, "default") != 0) {
            fprintf(source, "    case %s:\n", casep->label);
            if (casep->type) {
                emit_length_member(source, casep->name, casep->type);
            }
            fprintf(source, "        break;\n");
        }
    }

    DL_FOREACH(xdr_unionp->cases, casep)
    {
        if (strcmp(casep->label, "default") == 0) {
            fprintf(source, "    default:\n");
            fprintf(source, "        break;\n");
        }
    }

    fprintf(source, "    }\n");
    fprintf(source, "    return length;\n");
    fprintf(source, "}\n\n");

    fprintf(source, "int marshall_length_%s(const struct %s *in)\n",
            name, name);
    fprintf(source, "{\n");
    fprintf(source, "    return __marshall_length_%s(in);\n", name);
    fprintf(source, "}\n\n");
} /* emit_length_union */

void
emit_program_header(
    FILE               *header,
    struct xdr_program *program,
    struct xdr_version *version)
{
    struct xdr_function *functionp;

    fprintf(header, "#include \"evpl/evpl_rpc2_program.h\"\n");

    fprintf(header, "struct %s {\n", version->name);
    fprintf(header, "    struct evpl_rpc2_program rpc2;\n");

    DL_FOREACH(version->functions, functionp)
    {

        if (strcmp(functionp->call_type->name, "void")) {
            fprintf(header,
                    "   void (*recv_call_%s)(struct evpl *evpl, struct evpl_rpc2_conn *conn, struct %s *, struct evpl_rpc2_msg *, void *);\n",
                    functionp->name,
                    functionp->call_type->name);
        } else {
            fprintf(header,
                    "   void (*recv_call_%s)(struct evpl *evpl, struct evpl_rpc2_conn *conn, struct evpl_rpc2_msg *, void *);\n",
                    functionp->name);
        }

        if (strcmp(functionp->reply_type->name, "void")) {
            fprintf(header,
                    "   void (*send_reply_%s)(struct evpl *evpl, struct %s *, void *);\n",
                    functionp->name,
                    functionp->reply_type->name);

        } else {
            fprintf(header,
                    "   void (*send_reply_%s)(struct evpl *evpl, void *);\n",
                    functionp->name);
        }

        if (strcmp(functionp->reply_type->name, "void")) {
            fprintf(header, "   void (*reply_%s)(struct %s *);\n",
                    functionp->name,
                    functionp->reply_type->name);
        } else {
            fprintf(header, "   void (*reply_%s)(void);\n",
                    functionp->name);
        }
    }
    fprintf(header, "};\n\n");

    fprintf(header, "void %s_init(struct %s *);\n",
            version->name, version->name);

} /* emit_program_header */

void
emit_program(
    FILE               *source,
    struct xdr_program *program,
    struct xdr_version *version)
{
    struct xdr_function *functionp;
    int                  maxproc = 0;

    fprintf(source, "#include <evpl/evpl.h>\n");
    fprintf(source, "#include \"evpl/evpl_rpc2_program.h\"\n");

    fprintf(source, "const static char *%s_%s_procs[] = {\n",
            program->name, version->name);
    for (functionp = version->functions; functionp != NULL; functionp =
             functionp->next) {
        fprintf(source, "    [%d] = \"%s\",\n", functionp->id, functionp->name);
        if (functionp->id > maxproc) {
            maxproc = functionp->id;
        }
    }

    fprintf(source, "};\n\n");

    fprintf(source, "static int\n");
    fprintf(source, "call_dispatch_%s(\n", version->name);
    fprintf(source, "    struct evpl *evpl,\n");
    fprintf(source, "    struct evpl_rpc2_conn *conn,\n");
    fprintf(source, "    struct evpl_rpc2_msg *msg,\n");
    fprintf(source, "    xdr_iovec *iov,\n");
    fprintf(source, "    int niov,\n");
    fprintf(source, "    int length,\n");
    fprintf(source, "    void *private_data)\n");
    fprintf(source, "{\n");
    fprintf(source, "    struct %s *prog = msg->program->program_data;\n",
            version->name);
    fprintf(source, "    int len;\n");
    fprintf(source, "    switch (msg->proc) {\n");

    for (functionp = version->functions; functionp != NULL; functionp =
             functionp->next) {

        if (functionp->id > maxproc) {
            maxproc = functionp->id;
        }

        fprintf(source, "    case %d:\n", functionp->id);

        /* Check if the function is implemented */
        fprintf(source, "        if (prog->recv_call_%s == NULL) {\n",
                functionp->name);
        fprintf(source, "            return 1;\n");
        fprintf(source, "        }\n");

        /* Call has an argument */
        if (strcmp(functionp->call_type->name, "void")) {
            /* We will unmarshall argument into provided buffer */
            fprintf(source, "        struct %s *%s_arg;\n",
                    functionp->call_type->name,
                    functionp->name);
            fprintf(source, "        xdr_dbuf_alloc_space(%s_arg, sizeof(*%s_arg), msg->dbuf);\n",
                    functionp->name, functionp->name);
            fprintf(source,
                    "        len = unmarshall_%s(%s_arg, iov, niov, &msg->read_chunk, msg->dbuf);\n",
                    functionp->call_type->name, functionp->name);
            fprintf(source, "        if (unlikely(len != length)) abort();\n");
            fprintf(source, "        if (len < 0) return 2;\n");

            /* Then make the call */
            fprintf(source,
                    "        prog->recv_call_%s(evpl, conn, %s_arg, msg, private_data);\n",
                    functionp->name, functionp->name);
        } else {
            /* No argument, just make the call */
            fprintf(source,
                    "        prog->recv_call_%s(evpl, conn, msg, private_data);\n",
                    functionp->name);

        }
        fprintf(source, "        break;\n\n");
    }

    fprintf(source, "    default:\n");
    fprintf(source, "        return 1;\n");
    fprintf(source, "    }\n");
    fprintf(source, "    return 0;\n");
    fprintf(source, "}\n\n");

    for (functionp = version->functions; functionp != NULL; functionp =
             functionp->next) {

        if (strcmp(functionp->reply_type->name, "void")) {
            fprintf(source,
                    "void send_reply_%s(struct evpl *evpl, struct %s *arg, void *private_data)\n",
                    functionp->name, functionp->reply_type->name);
            fprintf(source, "{\n");
            fprintf(source, "    struct evpl_rpc2_msg *msg = private_data;\n");
            fprintf(source, "    struct evpl_iovec iov, *msg_iov;\n");
            fprintf(source, "    int niov, msg_niov = 256,len;\n");
            fprintf(source, "    xdr_dbuf_alloc_space(msg_iov, sizeof(*msg_iov) * 256, msg->dbuf);\n");
            fprintf(source,
                    "    niov = evpl_iovec_reserve(evpl, 128*1024, 8, 1, &iov);\n");
            fprintf(source, "    if (unlikely(niov != 1)) return;\n");
            fprintf(source,
                    "    len = marshall_%s(arg, &iov, msg_iov, &msg_niov, &msg->write_chunk, msg->program->reserve);\n",
                    functionp->reply_type->name);
            fprintf(source, "    if (unlikely(len < 0)) abort();\n");
            fprintf(source, "    xdr_iovec_set_len(&iov, len + msg->program->reserve);\n");
            fprintf(source,
                    "    evpl_iovec_commit(evpl, 0, &iov, 1);\n");
            fprintf(source,
                    "    msg->program->reply_dispatch(evpl, msg, msg_iov, msg_niov, len);\n");
        } else {
            fprintf(source,
                    "void send_reply_%s(struct evpl *evpl, void *private_data)\n",
                    functionp->name);
            fprintf(source, "{\n");
            fprintf(source, "    struct evpl_rpc2_msg *msg = private_data;\n");
            fprintf(source, "    struct evpl_iovec iov;\n");
            fprintf(source, "    int niov, len;\n");
            fprintf(source, "    niov = evpl_iovec_alloc(evpl, msg->program->reserve, 8, 1, &iov);\n");
            fprintf(source,
                    "    msg->program->reply_dispatch(evpl, msg, &iov, niov, msg->program->reserve);\n")
            ;
        }

        fprintf(source, "}\n\n");
    }

    fprintf(source, "void %s_init(struct %s *prog)\n", version->name, version->
            name);
    fprintf(source, "{\n");
    fprintf(source, "    memset(prog, 0, sizeof(*prog));\n");
    fprintf(source, "    prog->rpc2.program = %s;\n", program->id);
    fprintf(source, "    prog->rpc2.version = %s;\n", version->id);
    fprintf(source, "    prog->rpc2.maxproc = %d;\n", maxproc);
    fprintf(source, "    prog->rpc2.reserve = 256;\n");
    fprintf(source, "    prog->rpc2.procs = %s_%s_procs;\n", program->name,
            version->name);
    fprintf(source, "    prog->rpc2.program_data = prog;\n");
    fprintf(source, "    prog->rpc2.call_dispatch = call_dispatch_%s;\n",
            version->name);

    for (functionp = version->functions; functionp != NULL; functionp =
             functionp->next) {
        fprintf(source, "    prog->send_reply_%s = send_reply_%s;\n",
                functionp->name, functionp->name);
    }

    fprintf(source, "}\n\n");
} /* emit_program */

static void
emit_member(
    FILE            *header,
    const char      *name,
    struct xdr_type *type)
{
    struct xdr_type       *emit_type;
    struct xdr_identifier *chk;
    const char            *structstr;

    HASH_FIND_STR(xdr_identifiers, type->name, chk);

    if (chk && chk->type == XDR_TYPEDEF) {
        emit_type = ((struct xdr_typedef *) chk->ptr)->type;
    } else {
        emit_type = type;
    }

    if (emit_type->builtin || emit_type->enumeration) {
        structstr = "";
    } else {
        structstr = "struct";
    }

    if (emit_type->opaque) {
        if (emit_type->array) {
            fprintf(header, "    uint8_t %s[%s];\n",
                    name,
                    emit_type->array_size);
        } else if (emit_type->zerocopy) {
            fprintf(header, "    %s  %s;\n",
                    "xdr_iovecr",
                    name);
        } else {
            fprintf(header, "    %s  %s;\n",
                    "xdr_opaque",
                    name);
        }
    } else if (strcmp(emit_type->name, "xdr_string") == 0) {
        fprintf(header, "    %s  %s;\n",
                emit_type->name,
                name);
    } else if (emit_type->vector) {
        fprintf(header, "    uint32_t  num_%s;\n",
                name);
        fprintf(header, "    %s %s *%s;\n",
                structstr,
                emit_type->name,
                name);
    } else if (emit_type->optional) {
        fprintf(header, "    %s %s *%s;\n",
                structstr,
                emit_type->name,
                name);
    } else if (emit_type->array) {
        fprintf(header, "    %s %s  %s[%s];\n",
                structstr,
                emit_type->name,
                name,
                emit_type->array_size);
    } else {
        fprintf(header, "    %s %s  %s;\n",
                structstr,
                emit_type->name,
                name);
    }

    if (chk && chk->type == XDR_ENUM) {
        /* Now that we've emitted the struct member,
         * treat this as a builtin uint32 for
         * the purpose of marshall/unmarshall
         */
        type->name    = "uint32_t";
        type->builtin = 1;
    }
} /* emit_member */

void
emit_wrappers(
    FILE       *source,
    const char *name)
{
    fprintf(source, "int\n");
    fprintf(source, "marshall_%s(\n", name);
    fprintf(source, "    const struct %s *out,\n", name);
    fprintf(source, "    xdr_iovec *iov_in,\n");
    fprintf(source, "    xdr_iovec *iov_out,\n");
    fprintf(source, "    int *niov_out,\n");
    fprintf(source, "    struct evpl_rpc2_rdma_chunk *write_chunk,\n");
    fprintf(source, "    int out_offset) {\n");
    fprintf(source, "    struct xdr_write_cursor cursor;\n");
    fprintf(source,
            "    xdr_write_cursor_init(&cursor, iov_in, iov_out, *niov_out, write_chunk, out_offset);\n");
    fprintf(source, "    __marshall_%s(out, &cursor);\n", name);
    fprintf(source, "    xdr_write_cursor_flush(&cursor);\n");
    fprintf(source, "    *niov_out = cursor.niov;\n");
    fprintf(source, "    return cursor.total;\n");
    fprintf(source, "}\n\n");

    fprintf(source, "int\n");
    fprintf(source, "unmarshall_%s(\n", name);
    fprintf(source, "    struct %s *out,\n", name);
    fprintf(source, "    const xdr_iovec *iov,\n");
    fprintf(source, "    int niov,\n");
    fprintf(source, "    struct evpl_rpc2_rdma_chunk *read_chunk,\n");
    fprintf(source, "    xdr_dbuf *dbuf) {\n");
    fprintf(source, "    struct xdr_read_cursor cursor;\n");
    fprintf(source, "    xdr_read_cursor_init(&cursor, iov, niov, read_chunk);\n");
    fprintf(source, "    return __unmarshall_%s(out, &cursor, dbuf);\n", name
            );
    fprintf(source, "}\n\n");
} /* emit_wrappers */

void
print_usage(const char *prog_name)
{
    fprintf(stderr, "Usage: %s <input.x> <output.c> <output.h>\n", prog_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h            Display this help message and exit\n");
} /* print_usage */

int
main(
    int   argc,
    char *argv[])
{
    struct xdr_struct        *xdr_structp;
    struct xdr_struct_member *xdr_struct_memberp;
    struct xdr_union         *xdr_unionp;
    struct xdr_union_case    *xdr_union_casep;
    struct xdr_typedef       *xdr_typedefp;
    struct xdr_enum          *xdr_enump;
    struct xdr_enum_entry    *xdr_enum_entryp;
    struct xdr_program       *xdr_programp;
    struct xdr_version       *xdr_versionp;
    struct xdr_const         *xdr_constp;
    struct xdr_buffer        *xdr_buffer;
    struct xdr_identifier    *xdr_identp, *xdr_identp_tmp, *chk, *chkm;
    int                       unemitted, ready, emit_rpc2 = 0;
    FILE                     *header, *source;
    const char               *input_file;
    const char               *output_c;
    const char               *output_h;
    int                       opt;

    while ((opt = getopt(argc, argv, "hr")) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'r':
                emit_rpc2 = 1;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        } /* switch */
    }

    if (argc - optind < 3) {
        fprintf(stderr, "Error: Missing required arguments.\n");
        print_usage(argv[0]);
        return 1;
    }

    input_file = argv[optind];
    output_c   = argv[optind + 1];
    output_h   = argv[optind + 2];

    yyin = fopen(input_file, "r");

    if (!yyin) {
        fprintf(stderr, "Failed to open input file %s: %s\n",
                input_file, strerror(errno));
        return 1;
    }

    yyparse();

    fclose(yyin);

    HASH_ITER(hh, xdr_identifiers, xdr_identp, xdr_identp_tmp)
    {
        switch (xdr_identp->type) {
            case XDR_TYPEDEF:
                xdr_typedefp = xdr_identp->ptr;

                /* Verify typedef refers to a legit type, and if the type
                 * it refers to is itself a typedef, resolve it to the
                 * type pointed to by that typedef directly
                 */

                while (xdr_typedefp->type->builtin == 0) {

                    HASH_FIND_STR(xdr_identifiers, xdr_typedefp->type->name, chk
                                  );

                    if (!chk) {
                        fprintf(stderr, "typedef %s uses unknown type %s\n",
                                xdr_typedefp->name,
                                xdr_typedefp->type->name);
                        exit(1);
                    }

                    if (chk->type == XDR_ENUM) {
                        xdr_typedefp->type->enumeration = 1;
                    }

                    if (chk->type != XDR_TYPEDEF) {
                        break;
                    }

                    xdr_typedefp->type = ((struct xdr_typedef *) chk->ptr)->type
                    ;

                }

                xdr_identp->emitted = 1;
                break;
            case XDR_ENUM:
                xdr_identp->emitted = 1;
                break;
            case XDR_CONST:
                xdr_identp->emitted = 1;
                break;
            case XDR_STRUCT:
                xdr_structp = xdr_identp->ptr;

                DL_FOREACH(xdr_structp->members, xdr_struct_memberp)
                {
                    if (xdr_struct_memberp->type->builtin) {
                        continue;
                    }

                    HASH_FIND_STR(xdr_identifiers, xdr_struct_memberp->type->
                                  name, chk);

                    if (!chk) {
                        fprintf(stderr,
                                "struct %s element %s uses  unknown type %s\n",
                                xdr_structp->name,
                                xdr_struct_memberp->name,
                                xdr_struct_memberp->type->name);
                        exit(1);
                    }

                    if (chk && chk->type == XDR_ENUM) {
                        xdr_struct_memberp->type->enumeration = 1;
                    } else if (chk && chk->type == XDR_TYPEDEF) {
                        xdr_struct_memberp->type = ((struct xdr_typedef *) chk->
                                                    ptr)->type;
                    } else if (chk && chk->type == XDR_STRUCT &&
                               ((struct xdr_struct *) chk->ptr)->linkedlist) {
                        xdr_struct_memberp->type->linkedlist = 1;
                    }
                }
                break;
            case XDR_UNION:

                xdr_unionp = xdr_identp->ptr;

                if (!xdr_unionp->pivot_type->builtin) {

                    HASH_FIND_STR(xdr_identifiers, xdr_unionp->pivot_type->name,
                                  chk);

                    if (!chk) {
                        fprintf(stderr,
                                "union %s element %s uses  unknown type %s\n",
                                xdr_structp->name,
                                xdr_struct_memberp->name,
                                xdr_struct_memberp->type->name);
                        exit(1);
                    }

                    if (chk && chk->type == XDR_TYPEDEF) {
                        xdr_unionp->pivot_type = ((struct xdr_typedef *) chk->
                                                  ptr)->type;
                    }
                }

                DL_FOREACH(xdr_unionp->cases, xdr_union_casep)
                {

                    if (xdr_union_casep->type == NULL ||
                        xdr_union_casep->type->builtin) {
                        continue;
                    }

                    HASH_FIND_STR(xdr_identifiers, xdr_union_casep->type->name,
                                  chk);

                    if (!chk) {
                        fprintf(stderr,
                                "union %s element %s uses  unknown type %s\n",
                                xdr_structp->name,
                                xdr_struct_memberp->name,
                                xdr_struct_memberp->type->name);
                        exit(1);
                    }

                    if (chk && chk->type == XDR_ENUM) {
                        xdr_union_casep->type->enumeration = 1;
                    } else if (chk && chk->type == XDR_TYPEDEF) {
                        xdr_union_casep->type = ((struct xdr_typedef *) chk->ptr
                                                 )->type;
                    }
                }
                break;
            default:
                abort();
        } /* switch */
    }

    header = fopen(output_h, "w");

    if (!header) {
        fprintf(stderr, "Failed to open output header file%s: %s\n",
                output_h, strerror(errno));
        return 1;
    }

    fprintf(header, "#pragma once\n");
    fprintf(header, "%s", embedded_builtin_h);

    fprintf(header, "\n");

    DL_FOREACH(xdr_consts, xdr_constp)
    {
        fprintf(header, "#define %-60s %s\n", xdr_constp->name, xdr_constp->
                value);
    }

    fprintf(header, "\n");

    DL_FOREACH(xdr_enums, xdr_enump)
    {
        fprintf(header, "typedef enum {\n");

        DL_FOREACH(xdr_enump->entries, xdr_enum_entryp)
        {
            fprintf(header, "   %-60s = %s,\n",
                    xdr_enum_entryp->name,
                    xdr_enum_entryp->value);
        }

        fprintf(header, "} %s;\n\n", xdr_enump->name);
    }

    fprintf(header, "\n");

    do{
        unemitted = 0;

        DL_FOREACH(xdr_structs, xdr_structp)
        {

            HASH_FIND_STR(xdr_identifiers, xdr_structp->name, chk);

            if (chk->emitted) {
                continue;
            }

            ready = 1;

            DL_FOREACH(xdr_structp->members, xdr_struct_memberp)
            {
                if (xdr_struct_memberp->type->builtin) {
                    continue;
                }

                HASH_FIND_STR(xdr_identifiers, xdr_struct_memberp->type->name,
                              chkm);

                if (chk != chkm && !chkm->emitted) {
                    ready = 0;
                    break;
                }
            }

            if (!ready) {
                unemitted = 1;
                continue;
            }

            fprintf(header, "struct %s {\n", xdr_structp->name);

            DL_FOREACH(xdr_structp->members, xdr_struct_memberp)
            {
                emit_member(header, xdr_struct_memberp->name,
                            xdr_struct_memberp->type);
            }
            fprintf(header, "};\n\n");

            HASH_FIND_STR(xdr_identifiers, xdr_structp->name, chk);

            chk->emitted = 1;
        }

        DL_FOREACH(xdr_unions, xdr_unionp)
        {

            HASH_FIND_STR(xdr_identifiers, xdr_unionp->name, chk);

            if (chk->emitted) {
                continue;
            }

            ready = 1;

            DL_FOREACH(xdr_unionp->cases, xdr_union_casep)
            {
                if (!xdr_union_casep->type ||
                    xdr_union_casep->type->builtin) {
                    continue;
                }

                HASH_FIND_STR(xdr_identifiers, xdr_union_casep->type->name, chk)
                ;

                if (!chk->emitted) {
                    ready = 0;
                    break;
                }
            }

            if (!ready) {
                unemitted = 1;
                continue;
            }

            fprintf(header, "struct %s {\n", xdr_unionp->name);
            fprintf(header, "    %-39s %s;\n", xdr_unionp->pivot_type->name,
                    xdr_unionp->pivot_name);
            fprintf(header, "    union {\n");

            DL_FOREACH(xdr_unionp->cases, xdr_union_casep)
            {

                if (!xdr_union_casep->type) {
                    continue;
                }

                fprintf(header, "    struct {\n");


                emit_member(header, xdr_union_casep->name,
                            xdr_union_casep->type);

                fprintf(header, "    };\n");
            }

            HASH_FIND_STR(xdr_identifiers, xdr_unionp->pivot_type->name, chkm);

            if (chkm && chkm->type == XDR_ENUM) {
                xdr_unionp->pivot_type->name    = "uint32_t";
                xdr_unionp->pivot_type->builtin = 1;
            }

            fprintf(header, "    };\n");
            fprintf(header, "};\n\n");

            HASH_FIND_STR(xdr_identifiers, xdr_unionp->name, chk);

            chk->emitted = 1;
        }

    } while (unemitted);

    DL_FOREACH(xdr_structs, xdr_structp)
    {
        emit_wrapper_headers(header, xdr_structp->name);
        emit_dump_headers(header, xdr_structp->name);
    }

    DL_FOREACH(xdr_unions, xdr_unionp)
    {
        emit_wrapper_headers(header, xdr_unionp->name);
        emit_dump_headers(header, xdr_unionp->name);
    }


    if (emit_rpc2) {
        DL_FOREACH(xdr_programs, xdr_programp)
        {
            DL_FOREACH(xdr_programp->versions, xdr_versionp)
            {
                emit_program_header(header, xdr_programp, xdr_versionp);
            }
        }
    }

    fclose(header);

    source = fopen(output_c, "w");

    if (!source) {
        fprintf(stderr, "Failed to open output source file %s: %s\n",
                output_c, strerror(errno));
        return 1;
    }

    fprintf(source, "#include <stdio.h>\n");
    fprintf(source, "#include \"%s\"\n", output_h);

    fprintf(source, "\n");

    fprintf(source, "%s", embedded_builtin_c);

    fprintf(source, "\n");

    DL_FOREACH(xdr_structs, xdr_structp)
    {
        emit_internal_headers(source, xdr_structp->name);
        emit_dump_internal(source, xdr_structp->name);
    }

    DL_FOREACH(xdr_unions, xdr_unionp)
    {
        emit_internal_headers(source, xdr_unionp->name);
        emit_dump_internal(source, xdr_unionp->name);
    }

    DL_FOREACH(xdr_structs, xdr_structp)
    {

        fprintf(source, "static void\n");
        fprintf(source, "__marshall_%s(\n", xdr_structp->name);
        fprintf(source, "    const struct %s *in,\n", xdr_structp->name);
        fprintf(source, "    struct xdr_write_cursor *cursor) {\n");

        DL_FOREACH(xdr_structp->members, xdr_struct_memberp)
        {
            if (xdr_structp->linkedlist &&
                strncmp(xdr_struct_memberp->name, "next", 4) == 0) {
                continue;
            }

            emit_marshall(source, xdr_struct_memberp->name,
                          xdr_struct_memberp->type);
        }

        fprintf(source, "}\n\n");

        fprintf(source, "static int\n");
        fprintf(source, "__unmarshall_%s(\n", xdr_structp->name);
        fprintf(source, "    struct %s *out,\n", xdr_structp->name);
        fprintf(source, "    struct xdr_read_cursor *cursor,\n");
        fprintf(source, "    xdr_dbuf *dbuf) {\n");
        fprintf(source, "    int rc, len = 0;\n");

        DL_FOREACH(xdr_structp->members, xdr_struct_memberp)
        {

            if (xdr_structp->linkedlist &&
                strncmp(xdr_struct_memberp->name, "next", 4) == 0) {
                continue;
            }

            emit_unmarshall(source, xdr_struct_memberp->name, xdr_struct_memberp
                            ->type);
        }
        fprintf(source, "    return len;\n");
        fprintf(source, "}\n\n");

        emit_wrappers(source, xdr_structp->name);

        emit_dump_struct(source, xdr_structp->name, xdr_structp);
        emit_length_struct(source, xdr_structp->name, xdr_structp);
    } /* main */

    DL_FOREACH(xdr_unions, xdr_unionp)
    {
        fprintf(source, "static void\n");
        fprintf(source, "__marshall_%s(\n", xdr_unionp->name);
        fprintf(source, "    const struct %s *in,\n", xdr_unionp->name);
        fprintf(source, "    struct xdr_write_cursor *cursor) {\n");

        emit_marshall(source, xdr_unionp->pivot_name, xdr_unionp->pivot_type);

        fprintf(source, "    switch (in->%s) {\n", xdr_unionp->pivot_name);

        DL_FOREACH(xdr_unionp->cases, xdr_union_casep)
        {
            if (strcmp(xdr_union_casep->label, "default") != 0) {
                fprintf(source, "    case %s:\n", xdr_union_casep->label);
                if (xdr_union_casep->voided) {
                    fprintf(source, "        break;\n");
                } else if (xdr_union_casep->type) {
                    emit_marshall(source, xdr_union_casep->name, xdr_union_casep
                                  ->type);
                    fprintf(source, "        break;\n");
                }
            }
        }

        DL_FOREACH(xdr_unionp->cases, xdr_union_casep)
        {
            if (strcmp(xdr_union_casep->label, "default") == 0) {
                fprintf(source, "    default:\n");
                if (xdr_union_casep->voided) {
                    fprintf(source, "        break;\n");
                } else if (xdr_union_casep->type) {
                    emit_marshall(source, xdr_union_casep->name, xdr_union_casep
                                  ->type);
                    fprintf(source, "        break;\n");
                }
            }
        }

        fprintf(source, "    }\n");
        fprintf(source, "    ;\n");
        fprintf(source, "}\n\n");

        fprintf(source, "static int\n");
        fprintf(source, "__unmarshall_%s(\n", xdr_unionp->name);
        fprintf(source, "    struct %s *out,\n", xdr_unionp->name);
        fprintf(source, "    struct xdr_read_cursor *cursor,\n");
        fprintf(source, "    xdr_dbuf *dbuf) {\n");
        fprintf(source, "    int rc, len = 0;\n");

        emit_unmarshall(source, xdr_unionp->pivot_name, xdr_unionp->pivot_type);

        fprintf(source, "    switch (out->%s) {\n", xdr_unionp->pivot_name);

        DL_FOREACH(xdr_unionp->cases, xdr_union_casep)
        {
            if (strcmp(xdr_union_casep->label, "default") != 0) {
                fprintf(source, "    case %s:\n", xdr_union_casep->label);
                if (xdr_union_casep->voided) {
                    fprintf(source, "        break;\n");
                } else if (xdr_union_casep->type) {
                    emit_unmarshall(source, xdr_union_casep->name,
                                    xdr_union_casep->type);
                    fprintf(source, "        break;\n");
                }
            }
        }

        DL_FOREACH(xdr_unionp->cases, xdr_union_casep)
        {
            if (strcmp(xdr_union_casep->label, "default") == 0) {
                fprintf(source, "    default:\n");
                fprintf(source, "        break;\n");
            }
        }
        fprintf(source, "    }\n");
        fprintf(source, "    return len;\n");
        fprintf(source, "}\n\n");

        emit_wrappers(source, xdr_unionp->name);

        emit_dump_union(source, xdr_unionp->name, xdr_unionp);
        emit_length_union(source, xdr_unionp->name, xdr_unionp);
    }

    if (emit_rpc2) {
        DL_FOREACH(xdr_programs, xdr_programp)
        {
            DL_FOREACH(xdr_programp->versions, xdr_versionp)
            {
                emit_program(source, xdr_programp, xdr_versionp);
            }
        }
    }

    fclose(source);

    HASH_CLEAR(hh, xdr_identifiers);

    while (xdr_buffers) {
        xdr_buffer = xdr_buffers;
        DL_DELETE(xdr_buffers, xdr_buffer);

        free(xdr_buffer->data);
        free(xdr_buffer);
    }

    return 0;
} /* main */