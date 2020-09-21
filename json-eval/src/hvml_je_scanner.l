%{
#include "hvml/hvml_log.h"
#include "hvml/hvml_utf8.h"
#include "hvml_je_parser.tab.h"
#include <stdio.h>

#define YYSTYPE       HVML_JE_YYSTYPE
#define YYLTYPE       HVML_JE_YYLTYPE

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

%}

%option prefix="hvml_je_yy"
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

%x DOLAR STR STR1

%%
<<EOF>> { int state; TOP_STATE(state);
          if (state != INITIAL) return -1;
          yyterminate(); }
"$"     { S();
          PUSH_STATE(DOLAR);
          return TOK_HVML_JE_DOLAR1; }
"{$"    { S();
          PUSH_STATE(DOLAR);
          return TOK_HVML_JE_DOLAR2; }
[^${]+  { S();
          yylval->str = strdup(yytext);
          return TOK_HVML_JE_TEXT; }
.       { S();
          return *yytext; }

<DOLAR>"}"          { S();
                      POP_STATE();
                      return *yytext; }
<DOLAR>[?@#%:]      { S();
                      return *yytext; }
<DOLAR>{LTOKEN}     { S();
                      yylval->str = strdup(yytext);
                      return TOK_HVML_JE_LTOKEN; }
<DOLAR>{LINTEGER}   { S();
                      yylval->str = strdup(yytext);
                      return TOK_HVML_JE_LINTEGER; }
<DOLAR>[.]        { S();
                      return *yytext; }
<DOLAR>[[(<]      { S();
                      return *yytext; }
<DOLAR>["]           { S();
                      PUSH_STATE(STR);
                      return *yytext; }
<DOLAR>[']           { S();
                      PUSH_STATE(STR1);
                      return *yytext; }
<DOLAR>[,]           { S();
                      return *yytext; }
<DOLAR>"$"           { S();
                      PUSH_STATE(DOLAR);
                      return TOK_HVML_JE_DOLAR1; }
<DOLAR>"{$"          { S();
                      PUSH_STATE(DOLAR);
                      return TOK_HVML_JE_DOLAR2; }
<DOLAR>[])>]         { S();
                      return *yytext; }
<DOLAR>[[:space:]]+  { S();
                       yylval->str = strdup(yytext);
                       return TOK_HVML_JE_SPACE; }
<DOLAR>.|\n          { unput(*yytext);
                       POP_STATE(); }

    /* no escape support yet */
<STR>["]        { S();
                  POP_STATE();
                  return *yytext; }
<STR>[^"]+      { S();
                  yylval->str = strdup(yytext);
                  return TOK_HVML_JE_LSTR; }

    /* no escape support yet */
<STR1>[']       { S();
                  POP_STATE();
                  return *yytext; }
<STR1>[^']+     { S();
                  yylval->str = strdup(yytext);
                  return TOK_HVML_JE_LSTR; }

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

