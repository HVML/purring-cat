%{
// This file is a part of Purring Cat, a reference implementation of HVML.
//
// Copyright (C) 2020, <freemine@yeah.net>.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
%}

%{
#include "hvml/hvml_log.h"
#include "hvml/hvml_utf8.h"
#include "hvml_dom_xpath_parser.tab.h"
#include <stdio.h>

// #define YYSTYPE       HVML_DOM_XPATH_YYSTYPE
// #define YYLTYPE       HVML_DOM_XPATH_YYLTYPE

#define S()                                   \
do {                                                \
    for (int i=0; i<yyleng; ++i) {                  \
        const char c = yytext[i];                   \
        if (c=='\n') {                              \
            yylloc->last_line    += 1;              \
            yylloc->last_column   = 0;              \
        } else {                                    \
            yylloc->last_column  += 1;              \
        }                                           \
    }                                               \
    yylloc->first_line   = yylloc->last_line;       \
    yylloc->first_column = yylloc->last_column;     \
} while (0)

#define PUSH_STATE(state)      yy_push_state(state, yyscanner)
#define POP_STATE()            yy_pop_state(yyscanner)

#define CHG_STATE(state)                \
do {                                    \
    yy_pop_state(yyscanner);            \
    yy_push_state(state, yyscanner);    \
} while (0)

#define TOP_STATE(top)                  \
do {                                    \
    yy_push_state(INITIAL, yyscanner);  \
    top = yy_top_state(yyscanner);      \
    yy_pop_state(yyscanner);            \
} while (0)

#define UNPUT()                                 \
do {                                            \
    while (yyleng) unput(yytext[yyleng-1]);     \
} while (0)

#define M()                                     \
do {                                            \
    yylloc->last_column += strlen(yytext);      \
} while (0)

%}

%option prefix="hvml_dom_xpath_yy"
%option bison-bridge bison-locations
%option reentrant
%option noyywrap
%option noinput nounput
%option debug verbose
%option stack
%option unput
%option nodefault
%option warn
%option perf-report
%option 8bit

LTOKEN   [A-Za-z_]([^[:cntrl:][:punct:][:space:]]|[_-])*
LINTEGER 0|([1-9][0-9]*)

NCNAMESTART [^[:cntrl:][:punct:][:space:][:digit:]:]|_
NCNAMECHAR  {NCNAMESTART}|[[:digit:]]|-|\.
NCNAME      {NCNAMESTART}{NCNAMECHAR}*
%x STR QUOTE

%%
<<EOF>> { int state; TOP_STATE(state);
          if (state != INITIAL) return -1;
          yyterminate(); }

"ancestor"                  { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_ANCESTOR; }
"ancestor-or-self"          { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_ANCESTOR_OR_SELF; }
"attribute"                 { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_ATTRIBUTE; }
"child"                     { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_CHILD; }
"descendant"                { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_DESCENDANT; }
"descendant-or-self"        { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_DESCENDANT_OR_SELF; }
"following"                 { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_FOLLOWING; }
"following-sibling"         { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_FOLLOWING_SIBLING; }
"namespace"                 { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_NAMESPACE; }
"parent"                    { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_PARENT; }
"preceding"                 { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_PRECEDING; }
"preceding-sibling"         { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_PRECEDING_SIBLING; }
"self"                      { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_SELF; }
"div"                       { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_DIV; }
"mod"                       { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_MOD; }
"or"                        { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_OR; }
"and"                       { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_AND; }

"comment"                   { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_COMMENT; }
"text"                      { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_TEXT; }
"processing-instruction"    { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_PROCESSING_INSTRUCTION; }
"node"                      { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_NODE; }
"json"                      { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_JSON; }

"//"                        { M(); return TOK_HVML_DOM_XPATH_SLASH2; }
"::"                        { M(); return TOK_HVML_DOM_XPATH_COLON2; }
">="                        { M(); return TOK_HVML_DOM_XPATH_GTE; }
"<="                        { M(); return TOK_HVML_DOM_XPATH_LTE; }
"!="                        { M(); return TOK_HVML_DOM_XPATH_NEQ; }
".."                        { M(); return TOK_HVML_DOM_XPATH_DOT2; }
"."                         { M(); return '.'; }
"*"                         { M(); return '*'; }
"+"                         { M(); return '+'; }
"-"                         { M(); return '-'; }
"="                         { M(); return '='; }
"<"                         { M(); return '<'; }
">"                         { M(); return '>'; }
"("                         { M(); return '('; }
")"                         { M(); return ')'; }
"["                         { M(); return '['; }
"]"                         { M(); return ']'; }
"/"                         { M(); return '/'; }
[[:space:]]+                { M(); }
[0-9]+                      { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_INTEGER; }
[0-9]?\.[0-9]+              { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_DOUBLE; }
{NCNAME}                    { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_NCNAME; }

["]                         { M(); PUSH_STATE(STR); return '"'; }
<STR>["]                    { M(); POP_STATE(); return '"'; }
<STR>[^"]+                  { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_LITERAL; }

[']                         { M(); PUSH_STATE(QUOTE); return TOK_HVML_DOM_XPATH_SQ; }
<QUOTE>[']                  { M(); POP_STATE(); return TOK_HVML_DOM_XPATH_SQ; }
<QUOTE>[^']+                { M(); yylval->cstr = yytext; return TOK_HVML_DOM_XPATH_LITERAL2; }

.|\n                        { M(); return *yytext; }

%%

// int hvml_parser_gen_dom(hvml_dom_t **pdom, FILE *in) {
//   // yylloc->first_line = yylloc->last_line = 1;
//   // yylloc->first_column = yylloc->last_column = 0;
//   yyscan_t arg = {0};
//   hvml_yylex_init(&arg);
//   hvml_yyset_in(in, arg);
//   hvml_yyset_debug(1, arg);
//   int ret =hvml_yyparse(pdom, arg);
//   hvml_yylex_destroy(arg);
//   return ret ? 1 : 0;
// }

