%{

/*
 * SPDX-FileCopyrightText: 2024 Ben Jarvis
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xdr.h"

extern FILE *yyin;

extern int line_num;
extern int column_num;

extern struct xdr_struct *xdr_structs;
extern struct xdr_union *xdr_unions;
extern struct xdr_typedef *xdr_typedefs;
extern struct xdr_enum *xdr_enums;
extern struct xdr_const *xdr_consts;
extern struct xdr_program *xdr_programs;

void * xdr_alloc(unsigned int size);

void xdr_add_identifier(int type, const char *name, void *ptr);

extern int yylex();

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s at line %d, column %d\n", s, line_num, column_num);
    exit(1);
}

%}

%union {
    char *str;
    struct xdr_struct *xdr_struct;
    struct xdr_struct_member *xdr_struct_member;
    struct xdr_type *xdr_type;
    struct xdr_typedef *xdr_typedef;
    struct xdr_enum *xdr_enum;
    struct xdr_enum_entry *xdr_enum_entry;
    struct xdr_union *xdr_union;
    struct xdr_union_case *xdr_union_case;
    struct xdr_program *xdr_program;
    struct xdr_version *xdr_version;
    struct xdr_function *xdr_function;
}

%token <str> IDENTIFIER NUMBER
%token UINT32 INT32 UINT64 INT64
%token INT UNSIGNED FLOAT DOUBLE BOOL ENUM STRUCT TYPEDEF
%token VOID STRING OPAQUE ZCOPAQUE UNION SWITCH CASE DEFAULT CONST
%token LBRACE RBRACE LPAREN RPAREN SEMICOLON COLON EQUALS
%token LBRACKET RBRACKET STAR LANGLE RANGLE COMMA PROGRAM VERSION

%type <xdr_struct> struct_def
%type <xdr_struct_member> struct_member struct_body
%type <xdr_type> type
%type <xdr_typedef> typedef
%type <xdr_enum> enum_def
%type <xdr_enum_entry> enum_entry enum_body
%type <xdr_union> union_def
%type <xdr_union_case> union_body case_clause
%type <str> case_label
%type <xdr_program> program
%type <xdr_version> version versions
%type <xdr_function> function functions

%start xdr_file

%%

xdr_file:
    xdr_defs
    ;

xdr_defs:
    xdr_def
    | xdr_defs xdr_def
    ;

xdr_def:
    typedef SEMICOLON
    {
        DL_APPEND(xdr_typedefs, $1);
        xdr_add_identifier(XDR_TYPEDEF, $1->name, $1);
    }
    | CONST IDENTIFIER EQUALS NUMBER SEMICOLON
    {
        struct xdr_const *xdr_constp = xdr_alloc(sizeof(*xdr_constp));

        xdr_constp->name = $2;
        xdr_constp->value = $4;
        DL_APPEND(xdr_consts, xdr_constp);
        xdr_add_identifier(XDR_CONST, xdr_constp->name, xdr_constp);
    }
    | enum_def SEMICOLON
    {
        DL_APPEND(xdr_enums, $1);
        xdr_add_identifier(XDR_ENUM, $1->name, $1);
    }
    | struct_def SEMICOLON
    {
        DL_APPEND(xdr_structs, $1);
        xdr_add_identifier(XDR_STRUCT, $1->name, $1);
    }
    | union_def SEMICOLON
    {
        DL_APPEND(xdr_unions, $1);
        xdr_add_identifier(XDR_UNION, $1->name, $1);
    }
    | program SEMICOLON
    {
        DL_APPEND(xdr_programs, $1);
    }
    ;

program:
    PROGRAM IDENTIFIER LBRACE versions RBRACE EQUALS NUMBER
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = $2;
        $$->id = $7;
        $$->versions = $4;
    }

version:
    VERSION IDENTIFIER LBRACE functions RBRACE EQUALS NUMBER SEMICOLON
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = $2;
        $$->id = $7;
        $$->functions = $4;
    }

versions:
    version
    {
        $$ = NULL;
        DL_APPEND($$, $1);
    }
    | versions version
    {
        $$ = $1;
        DL_APPEND($$, $2);
    }

functions:
    function 
    {
        $$ = NULL;
        DL_APPEND($$, $1);
    }
    | functions function
    {
        $$ = $1;
        DL_APPEND($$, $2);
    
    }

function:
    type IDENTIFIER LPAREN type RPAREN EQUALS NUMBER SEMICOLON
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->id = atoi($7);
        $$->name = $2;
        $$->reply_type = $1;
        $$->call_type = $4;
    }

typedef:
    TYPEDEF type IDENTIFIER
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = $3;
        $$->type = $2;
    }
    | TYPEDEF type IDENTIFIER LANGLE RANGLE
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = $3;
        $$->type = $2;
        $$->type->vector = 1;
        $$->type->vector_bound = NULL;
    }
    | TYPEDEF type IDENTIFIER LANGLE IDENTIFIER RANGLE
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = $3;
        $$->type = $2;
        $$->type->vector = 1;
        $$->type->vector_bound = $5;
    }
    | TYPEDEF type IDENTIFIER LBRACKET IDENTIFIER RBRACKET
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = $3;
        $$->type = $2;
        $$->type->array = 1;
        $$->type->array_size = $5;
    }
    | TYPEDEF type IDENTIFIER LBRACKET NUMBER RBRACKET
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = $3;
        $$->type = $2;
        $$->type->array = 1;
        $$->type->array_size = $5;
    }
    ;

enum_def:
    ENUM IDENTIFIER LBRACE enum_body RBRACE
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = $2;
        $$->entries = $4;
    }
    ;

enum_body:
    enum_entry
    {
        $$ = NULL;
        DL_APPEND($$, $1);
    }
    | enum_body COMMA enum_entry
    {
        $$ = $1;
        DL_APPEND($$, $3);
    }
    ;

enum_entry:
    IDENTIFIER EQUALS NUMBER
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = $1;
        $$->value = $3;
    }

    | IDENTIFIER EQUALS IDENTIFIER
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = $1;
        $$->value = $3;
    }
    ;

struct_def:
    STRUCT IDENTIFIER LBRACE struct_body RBRACE
    {
        struct xdr_struct_member *member;
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = $2;
        $$->members = $4;

        DL_FOREACH($$->members, member)
        if (member->type->optional && 
            strncmp(member->name, "next", 4) == 0) {
            $$->linkedlist = 1;
            $$->nextmember = member->name;
            member->type->linkedlist = 1;
        }
    }
    ;

struct_body:
    struct_member
    {
        $$ = NULL;
        DL_APPEND($$, $1);
    }
    | struct_body struct_member
    {
        $$ = $1;
        DL_APPEND($$, $2);
    }
    ;

struct_member:
    type IDENTIFIER SEMICOLON
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->type = $1;
        $$->name = $2;
    }
    | type IDENTIFIER LANGLE NUMBER RANGLE SEMICOLON
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->type = $1;
        $$->name = $2;
        $$->type->vector = 1;
        $$->type->vector_bound = $4;
    }
    | type IDENTIFIER LANGLE IDENTIFIER RANGLE SEMICOLON
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->type = $1;
        $$->name = $2;
        $$->type->vector = 1;
        $$->type->vector_bound = $4;
    }
    | type IDENTIFIER LANGLE RANGLE SEMICOLON
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->type = $1;
        $$->name = $2;
        $$->type->vector = 1;
        $$->type->vector_bound = NULL;
    }
    | type IDENTIFIER LBRACKET IDENTIFIER RBRACKET SEMICOLON
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->type = $1;
        $$->name = $2;
        $$->type->array = 1;
        $$->type->array_size = $4;
    }
    | type IDENTIFIER LBRACKET NUMBER RBRACKET SEMICOLON
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->type = $1;
        $$->name = $2;
        $$->type->array = 1;
        $$->type->array_size = $4;
    }
    ;

union_def:
    UNION IDENTIFIER SWITCH LPAREN type IDENTIFIER RPAREN LBRACE union_body RBRACE
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = $2;
        $$->pivot_name = $6;
        $$->pivot_type = $5;
        $$->cases = $9;
    }
    ;

union_body:
    case_clause
    {
        $$ = NULL;
        DL_APPEND($$, $1);
    }
    | union_body case_clause
    {
        $$ = $1;
        DL_APPEND($$, $2);
    }
    ;

case_label:
    DEFAULT COLON
    {
        $$ = "default";
    }
    | CASE IDENTIFIER COLON
    {
        $$ = $2;
    }

case_clause:
    case_label type IDENTIFIER SEMICOLON
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->label = $1;
        $$->type = $2;
        $$->name = $3;
    }
    | case_label 
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->label = $1;
    }
    | case_label VOID SEMICOLON
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->label = $1;
        $$->voided = 1;
    }
    ;

type:
    INT32
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = "int32_t";
        $$->builtin = 1;
    }
    | UINT32
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = "uint32_t";
        $$->builtin = 1;
    }
    | INT64 
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = "int64_t";
        $$->builtin = 1;
    }
    | UINT64
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = "uint64_t";
        $$->builtin = 1;
    }
    | INT
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = "int32_t";
        $$->builtin = 1;
    }
    | VOID
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = "void";
        $$->builtin = 1;
    }
    | UNSIGNED INT
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = "uint32_t";
        $$->builtin = 1;
    }
    | FLOAT
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = "float";
        $$->builtin = 1;
    }
    | DOUBLE
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = "double";
        $$->builtin = 1;
    }
    | BOOL
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = "xdr_bool";
        $$->builtin = 1;
    }
    | STRING
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = "xdr_string";
        $$->builtin = 1;
    }
    | OPAQUE
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = "xdr_iovec";
        $$->builtin = 1;
        $$->opaque = 1;
        $$->zerocopy = 0;
    }
    | ZCOPAQUE
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = "xdr_iovec";
        $$->builtin = 1;
        $$->opaque = 1;
        $$->zerocopy = 1;
    }
    | IDENTIFIER
    {
        $$ = xdr_alloc(sizeof(*$$));
        $$->name = $1;
        $$->builtin = 0;
    }
    | type STAR
    {
        $$ = $1;
        $$->optional = 1;

    }
    | type LBRACKET NUMBER RBRACKET
    {
        $$ = $1;
        $$->array = 1;
        $$->array_size = $3;
    }
    ;

%%
