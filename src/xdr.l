%{

/*
 * SPDX-FileCopyrightText: 2024 Ben Jarvis
 *
 * SPDX-License-Identifier: LGPL
 */


#include "y.tab.h"
int line_num = 1;
int column_num = 1;

char * xdr_strdup(const char *str);

%}

%x C_COMMENT PCT

%%

"/*"            { column_num += yyleng; BEGIN(C_COMMENT); }
<C_COMMENT>"*/" { column_num += yyleng; BEGIN(INITIAL); }
<C_COMMENT>\n   { line_num++; column_num = 1; }
<C_COMMENT>.    { column_num++; }

"%"             { column_num++; BEGIN(PCT); }
<PCT>\n         { line_num++; column_num=1;  BEGIN(INITIAL); }
<PCT>.          { column_num++; }

\n      { line_num++; column_num = 1;  }

"uint32_t"      { column_num += yyleng; return UINT32; }
"int32_t"       { column_num += yyleng; return INT32; }
"uint64_t"      { column_num += yyleng; return UINT64; }
"int64_t"       { column_num += yyleng; return INT64;  }
"int"           { column_num += yyleng; return INT; }
"unsigned"      { column_num += yyleng; return UNSIGNED; }
"float"         { column_num += yyleng; return FLOAT; }
"double"        { column_num += yyleng; return DOUBLE; }
"bool"          { column_num += yyleng; return BOOL; }
"enum"          { column_num += yyleng; return ENUM; }
"struct"        { column_num += yyleng; return STRUCT; }
"typedef"       { column_num += yyleng; return TYPEDEF; }
"void"          { column_num += yyleng; return VOID; }
"string"        { column_num += yyleng; return STRING; }
"opaque"        { column_num += yyleng; return OPAQUE; }
"union"         { column_num += yyleng; return UNION; }
"switch"        { column_num += yyleng; return SWITCH; }
"case"          { column_num += yyleng; return CASE; }
"default"       { column_num += yyleng; return DEFAULT; }
"const"         { column_num += yyleng; return CONST; }
"program"       { column_num += yyleng; return PROGRAM; } 
"version"       { column_num += yyleng; return VERSION; }

"{"             { column_num++; return LBRACE; }
"}"             { column_num++; return RBRACE; }
"("             { column_num++; return LPAREN; }
")"             { column_num++; return RPAREN; }
";"             { column_num++; return SEMICOLON; }
":"             { column_num++; return COLON; }
","             { column_num++; return COMMA; }
"="             { column_num++; return EQUALS; }
"["             { column_num++; return LBRACKET; }
"]"             { column_num++; return RBRACKET; }
"*"             { column_num++; return STAR; }
"<"             { column_num++; return LANGLE; }
">"             { column_num++; return RANGLE; }

[0-9]+          { column_num += yyleng; yylval.str = xdr_strdup(yytext); return NUMBER; }
0x[0-9A-Fa-f]+  { column_num += yyleng; yylval.str = xdr_strdup(yytext); return NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]* { column_num += yyleng; yylval.str = xdr_strdup(yytext); return IDENTIFIER; }

[ \t]+          { column_num += yyleng; }

.               { printf("Unknown character: %s at line %d, column %d\n", yytext, line_num, column_num); }

%%

int yywrap(void) {
    return 1;
}
