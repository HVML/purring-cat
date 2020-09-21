%code top {
    // code top========================
}

%code requires {
    // code requires===================
#include "hvml/hvml_dom.h"
#include "hvml/hvml_string.h"

#include <stdio.h>
}

%code {
    // code ===========================
#include "hvml/hvml_log.h"
#include "hvml_je_scanner.lex.h"

#include <ctype.h>
#include <math.h>
#include <string.h>
void yyerror(YYLTYPE *yylloc, hvml_dom_t **pdom, void *arg, char const *s);

}

/* Bison declarations. */
%require "3.4"
%define api.prefix {hvml_je_yy}
%define api.pure full
%define api.token.prefix {TOK_HVML_JE_}
%define locations
%define parse.error verbose

%parse-param { hvml_dom_t **pdom }
%param { void *arg }

%union { char            *str; }

%token <str> TEXT LINTEGER LTOKEN LSTR
%token DOLAR1 DOLAR2 SPACE

%destructor { free($$); } <str>

%% /* The grammar follows. */

input:
  %empty
| input TEXT  { D("text: [%s]", $2); free($2); }
| input jee
| input error { return -1; }
;

jee:
  DOLAR1 jae ows
| DOLAR2 ows jae ows '}' ows
| LINTEGER ows       { D("integer: [%s]", $1); free($1); }
| '"'  LSTR '"' ows  { D("str: [%s]", $2); free($2); }
| '\'' LSTR '\'' ows { D("str: [%s]", $2); free($2); }
;

jae:
  var
| jae '.' key
| jae '(' ows args ows ')'
| jae '<' ows args ows '>'
| jae '[' ows jee ows ']'
;

var:
  '?'                { D("?"); }
| '@'                { D("@"); }
| '#'                { D("#"); }
| '%'                { D("%%"); }
| ':'                { D(":"); }
| LINTEGER           { D("integer: [%s]", $1); free($1); }
| LTOKEN             { D("token: [%s]", $1); free($1); }
;

key:
  LTOKEN             { D("token: [%s]", $1); free($1); }
;

args:
  %empty
| jees
;

jees:
  jee
| jees ows ',' ows jee
;

ows:
  %empty
| ows SPACE
;

%%


/* Called by yyparse on error. */
void yyerror(YYLTYPE *yylloc, hvml_dom_t **pdom, void *arg, char const *s) {
  (void)pdom;
  (void)arg;
  (void)yylloc;
  D("%s: (%d,%d)->(%d,%d)", s,
    yylloc->first_line, yylloc->first_column,
    yylloc->last_line, yylloc->last_column);
}

int dummy(hvml_dom_t **pdom, FILE *in) {
  yyscan_t arg = {0};
  hvml_je_yylex_init(&arg);
  hvml_je_yyset_in(in, arg);
  hvml_je_yyset_debug(1, arg);
  int ret =hvml_je_yyparse(pdom, arg);
  D("ret:%d", ret);
  hvml_je_yylex_destroy(arg);
  return ret ? 1 : 0;
}

