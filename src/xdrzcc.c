#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "y.tab.h"

#include "xdr.h"

extern FILE *yyin;

extern const char *embedded_builtin_c;

struct xdr_struct *xdr_structs = NULL;
struct xdr_union *xdr_unions = NULL;
struct xdr_typedef *xdr_typedefs = NULL;
struct xdr_enum *xdr_enums = NULL;
struct xdr_const *xdr_consts = NULL;

struct xdr_buffer {
    void *data;
    unsigned int used;
    unsigned int size;
    struct xdr_buffer *prev;
    struct xdr_buffer *next;
};

struct xdr_buffer *xdr_buffers = NULL;

struct xdr_identifier {
    char *name;
    int type;
    int emitted;
    void *ptr;
    struct UT_hash_handle hh;
};

struct xdr_identifier *xdr_identifiers = NULL;

void *
xdr_alloc(unsigned int size)
{
    struct xdr_buffer *xdr_buffer = xdr_buffers;
    void *ptr;

    size += (8 - (size & 7)) & 7;

    if (xdr_buffer == NULL || (xdr_buffer->size - xdr_buffer->used) < size) {
        xdr_buffer = calloc(1, sizeof(*xdr_buffer));
        xdr_buffer->size = 2*1024*1024;
        xdr_buffer->used = 0;
        xdr_buffer->data = calloc(1, xdr_buffer->size);
        DL_PREPEND(xdr_buffers, xdr_buffer);
    }

    ptr = xdr_buffer->data + xdr_buffer->used;

    xdr_buffer->used += size;

    return ptr;
}

char *
xdr_strdup(const char *str)
{
    int len = strlen(str) + 1;
    char *out = xdr_alloc(len);

    memcpy(out, str, len);

    return out;
}

void
xdr_add_identifier(int type, char *name, void *ptr)
{
    struct xdr_identifier *ident;

    HASH_FIND_STR(xdr_identifiers, name, ident);

    if (ident) {
        fprintf(stderr,"Duplicate symbol '%s' found.\n", name);
        exit(1);
    }

    ident = xdr_alloc(sizeof(*ident));

    ident->type = type;
    ident->name = name;
    ident->ptr = ptr;

    HASH_ADD_STR(xdr_identifiers,  name, ident);

}

void
emit_unmarshall(
    FILE *output,
    const char *name,
    struct xdr_type *type)
{
    char    type_sym[80], *ch;

    snprintf(type_sym, sizeof(type_sym), "%s", type->name);

    ch = type_sym;

    while (*ch) {
        if (*ch == ' ') *ch = '_';
        ch++;
    }

    fprintf(output, "    /* %s */\n", name);

    if (type->vector) {

    } else if (type->array) {

        fprintf(output,"    __unmarshall_%s(out->%s, %s, cursor);\n",
            type_sym, name, type->array_size);

    } else if (type->pointer) {

    } else {
        fprintf(output,"    __unmarshall_%s(&out->%s, 1, cursor);\n",
                type_sym, name);
    }
}

int main(int argc, char *argv[])
{
    struct xdr_struct *xdr_structp;
    struct xdr_struct_member *xdr_struct_memberp;
    struct xdr_union *xdr_unionp;
    struct xdr_union_case *xdr_union_casep;
    struct xdr_typedef *xdr_typedefp;
    struct xdr_type *emit_type;
    struct xdr_enum *xdr_enump;
    struct xdr_enum_entry *xdr_enum_entryp;
    struct xdr_const *xdr_constp;
    struct xdr_buffer *xdr_buffer;
    struct xdr_identifier *xdr_identp, *xdr_identp_tmp, *chk;
    int unemitted, ready;
    FILE *header, *source;

    if (argc < 4) {
        fprintf(stderr,"%s <input.x> <output.c> <output.h>\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");

    if (!yyin) {
        fprintf(stderr,"Failed to open input file %s: %s\n",
                argv[1], strerror(errno));
        return 1;
    }

    yyparse();

    fclose(yyin);

    HASH_ITER(hh, xdr_identifiers, xdr_identp, xdr_identp_tmp) {
        switch(xdr_identp->type) {
        case XDR_TYPEDEF:
            xdr_typedefp = xdr_identp->ptr;
           
            /* Verify typedef refers to a legit type, and if the type
             * it refers to is itself a typedef, resolve it to the
             * type pointed to by that typedef directly
             */
 
            while (xdr_typedefp->type->builtin == 0) {

                HASH_FIND_STR(xdr_identifiers, xdr_typedefp->type->name, chk);

                if (!chk) {
                    fprintf(stderr,"typedef %s uses unknown type %s\n",
                        xdr_typedefp->name,
                        xdr_typedefp->type->name);
                    exit(1);
                }

                if (chk->type != XDR_TYPEDEF) {
                    break;
                }

                xdr_typedefp->type = ((struct xdr_typedef *)chk->ptr)->type;
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

            DL_FOREACH(xdr_structp->members, xdr_struct_memberp) {
                if (xdr_struct_memberp->type->builtin) {
                    continue;
                }

                HASH_FIND_STR(xdr_identifiers, xdr_struct_memberp->type->name, chk);

                if (!chk) {
                    fprintf(stderr,"struct %s element %s uses  unknown type %s\n",
                        xdr_structp->name,
                        xdr_struct_memberp->name,
                        xdr_struct_memberp->type->name);
                    exit(1);
                }

                if (chk && chk->type == XDR_TYPEDEF) {
                    xdr_struct_memberp->type = ((struct xdr_typedef *)chk->ptr)->type;
                }

            }
            break;
        case XDR_UNION:
            break;
        default:
            abort();
        }
    }

    header = fopen(argv[3], "w");

    if (!header) {
        fprintf(stderr,"Failed to open output header file%s: %s\n",
                argv[3], strerror(errno));
        return 1;
    }

    fprintf(header, "#include <stdint.h>\n");

    fprintf(header, "\n");

    fprintf(header,"#ifndef xdr_iovec\n");
    fprintf(header,"struct xdr_iovec {\n");
    fprintf(header,"    void    *iov_base;\n");
    fprintf(header,"    uint32_t iov_len;\n");
    fprintf(header,"};\n");
    fprintf(header,"#define xdr_iovec_data(iov) ((iov)->iov_base)\n");
    fprintf(header,"#define xdr_iovec_len(iov) ((iov)->iov_len)\n");
    fprintf(header,"#endif\n");

    fprintf(header,"\n");

    DL_FOREACH(xdr_consts, xdr_constp) {
        fprintf(header,"#define %-60s %s\n", xdr_constp->name, xdr_constp->value);
    }

    fprintf(header,"\n");

    DL_FOREACH(xdr_structs, xdr_structp) {
        fprintf(header,"typedef struct %s %s;\n", xdr_structp->name, xdr_structp->name);
    }
    
    fprintf(header,"\n");

    DL_FOREACH(xdr_unions, xdr_unionp) {
        fprintf(header,"typedef struct %s %s;\n", xdr_unionp->name, xdr_unionp->name);
    }

    fprintf(header, "\n");

    DL_FOREACH(xdr_enums, xdr_enump) {
        fprintf(header, "typedef enum {\n");

        DL_FOREACH(xdr_enump->entries, xdr_enum_entryp) {
            fprintf(header,"   %-60s = %s,\n",
                xdr_enum_entryp->name,
                xdr_enum_entryp->value);
        }

        fprintf(header,"} %s;\n\n", xdr_enump->name);

    }

    fprintf(header,"\n");

    do {
        unemitted = 0;

        DL_FOREACH(xdr_structs, xdr_structp) {

            HASH_FIND_STR(xdr_identifiers, xdr_structp->name, chk);
        
            if (chk->emitted) continue;

            ready = 1;

            DL_FOREACH(xdr_structp->members, xdr_struct_memberp) {
                if (xdr_struct_memberp->type->builtin ||
                    xdr_struct_memberp->type->pointer) {
                    continue;
                }

                HASH_FIND_STR(xdr_identifiers, xdr_struct_memberp->type->name, chk);

                if (!chk->emitted) {
                    ready = 0;
                    break;
                }
            }

            if (!ready) {
                unemitted = 1;
                continue;
            }

            fprintf(header,"struct %s {\n", xdr_structp->name);

            DL_FOREACH(xdr_structp->members, xdr_struct_memberp) {

                HASH_FIND_STR(xdr_identifiers, xdr_struct_memberp->type->name, chk);

                if (chk && chk->type == XDR_TYPEDEF) {
                    emit_type = ((struct xdr_typedef*)chk->ptr)->type;
                } else {
                    emit_type = xdr_struct_memberp->type;
                }

                if (emit_type->vector) {
                    fprintf(header, "    %-39s  num_%s;\n",
                        "uint32_t", xdr_struct_memberp->name);
                    fprintf(header, "    %-39s *%s;\n",
                        emit_type->name,
                        xdr_struct_memberp->name);
                } else if (emit_type->array) {
                    if (emit_type->opaque) {
                        fprintf(header,"    %-39s  %s[%s];\n",
                            "uint8_t",
                            xdr_struct_memberp->name,
                            emit_type->array_size);
                    } else {
                        fprintf(header, "    %-39s  %s[%s];\n",
                            emit_type->name,
                            xdr_struct_memberp->name,
                            emit_type->array_size);
                    }
                } else {
                    fprintf(header, "    %-39s %s%s;\n",
                        emit_type->name, 
                        emit_type->pointer ? "*" : " ",
                        xdr_struct_memberp->name);
                }
            }
            fprintf(header,"};\n\n");

            HASH_FIND_STR(xdr_identifiers, xdr_structp->name, chk);

            chk->emitted = 1;

        }

        DL_FOREACH(xdr_unions, xdr_unionp) {

            HASH_FIND_STR(xdr_identifiers, xdr_unionp->name, chk);

            if (chk->emitted) continue;

            ready = 1;

            DL_FOREACH(xdr_unionp->cases, xdr_union_casep) {
                if (!xdr_union_casep->type ||
                    xdr_union_casep->type->builtin ||
                    xdr_union_casep->type->pointer) {
                    continue;
                }

                HASH_FIND_STR(xdr_identifiers, xdr_union_casep->type->name, chk);

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
            fprintf(header, "    %-39s %s;\n", xdr_unionp->pivot_type->name, xdr_unionp->pivot_name);
            fprintf(header, "    union {\n");
            DL_FOREACH(xdr_unionp->cases, xdr_union_casep) {

                if (!xdr_union_casep->type) {
                    continue;
                }

                HASH_FIND_STR(xdr_identifiers, xdr_union_casep->type->name, chk);

                if (chk && chk->type == XDR_TYPEDEF) {
                    emit_type = ((struct xdr_typedef*)chk->ptr)->type;
                } else {
                    emit_type = xdr_union_casep->type;
                }

                if (emit_type->vector) {
                    fprintf(header,"        %-34s  num_%s;\n",
                        "uint32_t", xdr_union_casep->name);
                    fprintf(header,"        %-34s *%s;\n",
                        emit_type->name,
                        xdr_union_casep->name);
                } else if (emit_type->array) {
                    if (emit_type->opaque) {
                        fprintf(header,"        %-34s  %s[%s];\n",
                            "uint8_t",
                            xdr_union_casep->name,
                            emit_type->array_size);
                    } else {
                        fprintf(header, "        %-34s  %s[%s];\n",
                            emit_type->name,
                            xdr_union_casep->name,
                            emit_type->array_size);
                    }
                } else {
                    fprintf(header,"        %-34s %s%s;\n",
                        emit_type->name,
                        emit_type->pointer ? "*" : " ",
                        xdr_union_casep->name); 
                }
            }

            fprintf(header,"    };\n");
            fprintf(header,"};\n\n");

            HASH_FIND_STR(xdr_identifiers, xdr_unionp->name, chk);

            chk->emitted = 1;
        }

    } while (unemitted);

    DL_FOREACH(xdr_structs, xdr_structp) {
        fprintf(header,"int unmarshall_%s(\n", xdr_structp->name);
        fprintf(header,"    %s *out,\n", xdr_structp->name);
        fprintf(header,"    int n,\n");
        fprintf(header,"    struct xdr_iovec *iov,\n");
        fprintf(header,"    int niov);\n\n");
    }

    DL_FOREACH(xdr_unions, xdr_unionp) {
        fprintf(header,"int unmarshall_%s(\n", xdr_unionp->name);
        fprintf(header,"    %s *out,\n", xdr_unionp->name);
        fprintf(header,"    int n,\n");
        fprintf(header,"    struct xdr_iovec *iov,\n");
        fprintf(header,"    int niov);\n\n");
    }

    fclose(header);

    source = fopen(argv[2], "w");

    if (!source) {
        fprintf(stderr,"Failed to open output source file %s: %s\n",
                argv[2], strerror(errno));
        return 1;
    }

    fprintf(source,"#include \"%s\"\n", argv[3]);

    fprintf(source,"\n");

    fprintf(source,"%s", embedded_builtin_c);

    fprintf(source,"\n");

    DL_FOREACH(xdr_enums, xdr_enump) {
        fprintf(source,"int\n");
        fprintf(source,"__unmarshall_%s(\n", xdr_enump->name);
        fprintf(source,"    %s *out,\n", xdr_enump->name);
        fprintf(source,"    int n,\n");
        fprintf(source,"    struct xdr_cursor *cursor);\n\n");
    }

    DL_FOREACH(xdr_structs, xdr_structp) {
        fprintf(source,"int\n");
        fprintf(source,"__unmarshall_%s(\n", xdr_structp->name);
        fprintf(source,"    %s *out,\n", xdr_structp->name);
        fprintf(source,"    int n,\n");
        fprintf(source,"    struct xdr_cursor *cursor);\n\n");
    }

    DL_FOREACH(xdr_unions, xdr_unionp) {
        fprintf(source,"int\n");
        fprintf(source,"__unmarshall_%s(\n", xdr_unionp->name);
        fprintf(source,"    %s *out,\n", xdr_unionp->name);
        fprintf(source,"    int n,\n");
        fprintf(source,"    struct xdr_cursor *cursor);\n\n");
    }

    DL_FOREACH(xdr_enums, xdr_enump) {

        fprintf(source,"int\n");
        fprintf(source,"__unmarshall_%s(\n", xdr_enump->name);
        fprintf(source,"    %s *out,\n", xdr_enump->name);
        fprintf(source,"    int n,\n");
        fprintf(source,"    struct xdr_cursor *cursor) {\n");
        fprintf(source,"    return 0;\n");
        fprintf(source, "}\n\n");

        fprintf(source,"int\n");
        fprintf(source,"unmarshall_%s(\n", xdr_enump->name);
        fprintf(source,"    %s *out,\n", xdr_enump->name);
        fprintf(source,"    int n,\n");
        fprintf(source,"    int niov) {\n");
        fprintf(source,"    return 0;\n");
        fprintf(source, "}\n\n");


    }


    DL_FOREACH(xdr_structs, xdr_structp) {
        fprintf(source,"int\n");
        fprintf(source,"__unmarshall_%s(\n", xdr_structp->name);
        fprintf(source,"    %s *out,\n", xdr_structp->name);
        fprintf(source,"    int n,\n");
        fprintf(source,"    struct xdr_cursor *cursor) {\n");

        DL_FOREACH(xdr_structp->members, xdr_struct_memberp) {
            emit_unmarshall(source, xdr_struct_memberp->name, xdr_struct_memberp->type);
        }

        fprintf(source,"    return 0;\n");
        fprintf(source, "}\n\n");
    } 

    DL_FOREACH(xdr_unions, xdr_unionp) {
        fprintf(source,"int\n");
        fprintf(source,"__unmarshall_%s(\n", xdr_unionp->name);
        fprintf(source,"    %s *out,\n", xdr_unionp->name);
        fprintf(source,"    int n,\n");
        fprintf(source,"    struct xdr_cursor *cursor) {\n");


        //emit_unmarshall(source, xdr_struct_memberp->name, xdr_struct_memberp->type);
        fprintf(source,"    return 0;\n");
        fprintf(source,"}\n\n");
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
}
