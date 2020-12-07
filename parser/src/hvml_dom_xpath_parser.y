%code top {
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
}

%code top {
    // code top========================
}

%code requires {
    // code requires===================
    #include "hvml/hvml_dom.h"
    #include "hvml/hvml_string.h"
    #include "hvml_dom_xpath_parser.h"

    #include <stdint.h>
    #include <stdio.h>
}

%code provides {
    #define YYSTYPE       HVML_DOM_XPATH_YYSTYPE
    #define YYLTYPE       HVML_DOM_XPATH_YYLTYPE
}

%code {
    // code ===========================

    #include "hvml/hvml_log.h"
    #include "hvml_dom_xpath_scanner.lex.h"

    #include <ctype.h>
    #include <math.h>
    #include <string.h>


    void yyerror(YYLTYPE *yylloc, int param, void *arg, char const *s);

    int check_func_name(const char *prefix, const char *local_part) {
        if (prefix) return 1;

        A(local_part, "internal logic error");

        if (strcmp(local_part, "comment")==0) return 0;
        if (strcmp(local_part, "text")==0) return 0;
        if (strcmp(local_part, "node")==0) return 0;
        if (strcmp(local_part, "json")==0) return 0;
        if (strcmp(local_part, "processing-instruction")==0) return 0;

        return 1;
    }

    int hvml_dom_xpath_expr_gen(hvml_dom_xpath_expr_t *expr, HVML_DOM_XPATH_OP_TYPE op, hvml_dom_xpath_expr_t *left, hvml_dom_xpath_expr_t *right) {
        A(memcmp(expr, &null_expr, sizeof(null_expr))==0, "internal logic error");
        expr->is_binary_op = 1;
        expr->left  = (hvml_dom_xpath_expr_t*)calloc(1, sizeof(*expr->left));
        expr->right = (hvml_dom_xpath_expr_t*)calloc(1, sizeof(*expr->right));
        if (!expr->left || !expr->right) {
            hvml_dom_xpath_expr_destroy(expr);
            *expr = null_expr;
            return -1;
        }
        *expr->left  = *left;
        *expr->right = *right;
        expr->op = op;
        return 0;
    }

    int hvml_dom_xpath_steps_append_step(hvml_dom_xpath_steps_t *steps, hvml_dom_xpath_step_t *step) {
        hvml_dom_xpath_step_t *e = (hvml_dom_xpath_step_t*)steps->steps;
        size_t                 n = steps->nsteps;
        e = (hvml_dom_xpath_step_t*)realloc(e, (n+1)*sizeof(*e));
        if (!e) return -1;
        e[n]    = *step;
        *step   = null_step;
        steps->steps      = e;
        steps->nsteps    += 1;
        return 0;
    }

    int hvml_dom_xpath_steps_append_steps(hvml_dom_xpath_steps_t *steps, hvml_dom_xpath_steps_t *r) {
        hvml_dom_xpath_step_t *e  = (hvml_dom_xpath_step_t*)steps->steps;
        size_t                 n  = steps->nsteps;
        hvml_dom_xpath_step_t *re = (hvml_dom_xpath_step_t*)r->steps;
        size_t                 rn = r->nsteps;

        e = (hvml_dom_xpath_step_t*)realloc(e, (n+rn)*sizeof(*e));
        if (!e) return -1;
        for (size_t i=0; i<rn; ++i) {
            e[n+i] = re[i];
            re[i]  = null_step;
        }
        steps->steps      = e;
        steps->nsteps    += rn;

        hvml_dom_xpath_steps_cleanup(r);

        return 0;
    }

    int hvml_dom_xpath_steps_append_slash(hvml_dom_xpath_steps_t *steps) {
        hvml_dom_xpath_step_t slash  = null_step;
        slash.axis                   = HVML_DOM_XPATH_AXIS_SLASH;
        slash.node_test.is_name_test = 0;
        slash.node_test.node_type    = HVML_DOM_XPATH_NT_NODE;
        return hvml_dom_xpath_steps_append_step(steps, &slash);
    }

    int hvml_dom_xpath_steps_append_slash2(hvml_dom_xpath_steps_t *steps) {
        hvml_dom_xpath_step_t slash2  = null_step;
        slash2.axis                   = HVML_DOM_XPATH_AXIS_DESCENDANT_OR_SELF;
        slash2.node_test              = null_node_test;
        slash2.node_test.is_name_test = 0;
        slash2.node_test.node_type    = HVML_DOM_XPATH_NT_NODE;
        return hvml_dom_xpath_steps_append_step(steps, &slash2);
    }
}

/* Bison declarations. */
%require "3.0.4"
%define api.prefix {hvml_dom_xpath_yy}
%define api.pure full
%define api.token.prefix {TOK_HVML_DOM_XPATH_}
%define locations
%define parse.error verbose
%defines

%parse-param { int param }
%param { void *arg }
// %parse-param { yyscan_t arg }
// %lex-param { yyscan_t arg }

%token COLON2 DOT2
%union { const char *cstr; }
%token <cstr> NCNAME
%token <cstr> ANCESTOR
%token <cstr> ANCESTOR_OR_SELF
%token <cstr> ATTRIBUTE
%token <cstr> CHILD
%token <cstr> DESCENDANT
%token <cstr> DESCENDANT_OR_SELF
%token <cstr> FOLLOWING
%token <cstr> FOLLOWING_SIBLING
%token <cstr> NAMESPACE
%token <cstr> PARENT
%token <cstr> PRECEDING
%token <cstr> PRECEDING_SIBLING
%token <cstr> SELF
%token <cstr> PROCESSING_INSTRUCTION
%token <cstr> COMMENT
%token <cstr> TEXT
%token <cstr> NODE
%token <cstr> JSON         /* extension */
%token <cstr> LITERAL
%token <cstr> LITERAL2
%token SLASH2
%token SQ
%token <cstr> INTEGER DOUBLE

%token <cstr> DIV
%token <cstr> MOD
%token <cstr> OR
%token <cstr> AND
%token NEQ
%token LTE
%token GTE

%union { char *str; }
%nterm <str> ncname literal
%destructor { free($$); } <str>

%union { hvml_dom_xpath_qname_t qname; }
%nterm <qname> qname
%destructor { hvml_dom_xpath_qname_cleanup(&($$)); } <qname>

%union { HVML_DOM_XPATH_AXIS_TYPE axis; }
%nterm <axis> axis_name

%union { HVML_DOM_XPATH_NT_TYPE node_type; }
%nterm <node_type> node_type

%union { hvml_dom_xpath_node_test_t node_test; }
%nterm <node_test> node_test
%destructor { hvml_dom_xpath_node_test_cleanup(&($$)); } <node_test>

%union { hvml_dom_xpath_step_t step; }
%nterm <step> step
%destructor { hvml_dom_xpath_step_cleanup(&($$)); } <step>

%union { hvml_dom_xpath_steps_t steps; }
%nterm <steps> location_path relative_location_path absolute_location_path abbreviated_relative_location_path
%destructor { hvml_dom_xpath_steps_cleanup(&($$)); } <steps>

%union { hvml_dom_xpath_expr_t expr; }
%nterm <expr> expr predicate argument
%destructor { hvml_dom_xpath_expr_cleanup(&($$)); } <expr>

%union { hvml_dom_xpath_exprs_t exprs; }
%nterm <exprs> predicates arguments
%destructor { hvml_dom_xpath_exprs_cleanup(&($$)); } <exprs>

%union { hvml_dom_xpath_func_t func_call; }
%nterm <func_call> function_call
%destructor { hvml_dom_xpath_func_cleanup(&($$)); } <func_call>

%union { HVML_DOM_XPATH_PREDEFINED_FUNC_TYPE predefined_func; }
%nterm <predefined_func> function_name

%union { hvml_dom_xpath_primary_t primary; }
%nterm <primary> primary_expr
%destructor { hvml_dom_xpath_primary_cleanup(&($$)); } <primary>

%union { hvml_dom_xpath_filter_expr_t filter_expr; }
%nterm <filter_expr> filter_expr
%destructor { hvml_dom_xpath_filter_expr_cleanup(&($$)); } <filter_expr>

%union { hvml_dom_xpath_path_expr_t path_expr; }
%nterm <path_expr> path_expr
%destructor { hvml_dom_xpath_path_expr_cleanup(&($$)); } <path_expr>

%union { hvml_dom_xpath_union_expr_t union_expr; }
%nterm <union_expr> union_expr unary_expr
%destructor { hvml_dom_xpath_union_expr_cleanup(&($$)); } <union_expr>

%left '/' SLASH2
%left '|'
%left OR AND
%left '=' NEQ
%left '<' '>' LTE GTE
%left '+' '-'
%left '*' DIV MOD
%left UMINUS

%% /* The grammar follows. */

input:
  %empty            { *(hvml_dom_xpath_steps_t*)hvml_dom_xpath_yyget_extra(arg) = null_steps; }
| location_path     { *(hvml_dom_xpath_steps_t*)hvml_dom_xpath_yyget_extra(arg) = $1; }
;

location_path:
  relative_location_path    { $$ = $1; }
| absolute_location_path    { $$ = $1; }
;

absolute_location_path:
  '/'                                   { $$ = null_steps;
                                          int r = 0;
                                          do {
                                              r = hvml_dom_xpath_steps_append_slash(&($$));
                                          } while (0);
                                          if (r) {
                                            hvml_dom_xpath_steps_cleanup(&($$));
                                            YYABORT;
                                          } }
| '/' relative_location_path            { $$ = null_steps;
                                          int r = 0;
                                          do {
                                              r = hvml_dom_xpath_steps_append_slash(&($$));
                                              if (r) break;
                                              r = hvml_dom_xpath_steps_append_steps(&($$), &($2));
                                          } while (0);
                                          if (r) {
                                            hvml_dom_xpath_steps_cleanup(&($$));
                                            hvml_dom_xpath_steps_cleanup(&($2));
                                            YYABORT;
                                          } }
| SLASH2 relative_location_path         { $$ = null_steps;
                                          int r = 0;
                                          do {
                                              r = hvml_dom_xpath_steps_append_slash(&($$));
                                              if (r) break;
                                              r = hvml_dom_xpath_steps_append_slash2(&($$));
                                              if (r) break;
                                              r = hvml_dom_xpath_steps_append_steps(&($$), &($2));
                                          } while (0);
                                          if (r) {
                                            hvml_dom_xpath_steps_cleanup(&($$));
                                            hvml_dom_xpath_steps_cleanup(&($2));
                                            YYABORT;
                                          } }
;

relative_location_path:
  step                                  { $$ = null_steps;
                                          if (hvml_dom_xpath_steps_append_step(&($$), &($1))) {
                                            hvml_dom_xpath_step_cleanup(&($1));
                                            YYABORT;
                                          } }
| relative_location_path '/' step       { $$ = null_steps;
                                          if (hvml_dom_xpath_steps_append_step(&($1), &($3))) {
                                            hvml_dom_xpath_steps_cleanup(&($1));
                                            hvml_dom_xpath_step_cleanup(&($3));
                                            YYABORT;
                                          }
                                          $$ = $1; }
| abbreviated_relative_location_path    { $$ = $1; }
;

abbreviated_relative_location_path:
  relative_location_path SLASH2 step    { $$ = null_steps;
                                          if (hvml_dom_xpath_steps_append_slash2(&($1))) {
                                            hvml_dom_xpath_steps_cleanup(&($1));
                                            hvml_dom_xpath_step_cleanup(&($3));
                                            YYABORT;
                                          }
                                          if (hvml_dom_xpath_steps_append_step(&($1), &($3))) {
                                            hvml_dom_xpath_steps_cleanup(&($1));
                                            hvml_dom_xpath_step_cleanup(&($3));
                                            YYABORT;
                                          }
                                          $$ = $1; }
;

step:
  node_test                             { $$ = null_step; $$.axis = HVML_DOM_XPATH_AXIS_CHILD; $$.node_test = $1; }
| node_test predicates                  { $$ = null_step; 
                                          $$.axis = HVML_DOM_XPATH_AXIS_CHILD; $$.node_test = $1;
                                          $$.exprs = $2; }
| axis_name COLON2 node_test            { $$ = null_step; $$.axis = $1; $$.node_test = $3; }
| axis_name COLON2 node_test predicates { $$ = null_step; $$.axis = $1; $$.node_test = $3;
                                          $$.exprs = $4; }
| '@' node_test                         { $$ = null_step;
                                          $$.axis = HVML_DOM_XPATH_AXIS_ATTRIBUTE;
                                          $$.node_test = $2; }
| '@' node_test predicates              { $$ = null_step;
                                          $$.axis = HVML_DOM_XPATH_AXIS_ATTRIBUTE;
                                          $$.node_test = $2;
                                          $$.exprs = $3; }
| '.'                                   { $$ = null_step;
                                          $$.axis = HVML_DOM_XPATH_AXIS_SELF;
                                          $$.node_test.is_name_test = 0;
                                          $$.node_test.node_type = HVML_DOM_XPATH_NT_NODE; }
| DOT2                                  { $$ = null_step;
                                          $$.axis = HVML_DOM_XPATH_AXIS_PARENT;
                                          $$.node_test.is_name_test = 0;
                                          $$.node_test.node_type = HVML_DOM_XPATH_NT_NODE; }
;

axis_name:
  ANCESTOR              { $$ = HVML_DOM_XPATH_AXIS_ANCESTOR; }
| ANCESTOR_OR_SELF      { $$ = HVML_DOM_XPATH_AXIS_ANCESTOR_OR_SELF; }
| ATTRIBUTE             { $$ = HVML_DOM_XPATH_AXIS_ATTRIBUTE; }
| CHILD                 { $$ = HVML_DOM_XPATH_AXIS_CHILD; }
| DESCENDANT            { $$ = HVML_DOM_XPATH_AXIS_DESCENDANT; }
| DESCENDANT_OR_SELF    { $$ = HVML_DOM_XPATH_AXIS_DESCENDANT_OR_SELF; }
| FOLLOWING             { $$ = HVML_DOM_XPATH_AXIS_FOLLOWING; }
| FOLLOWING_SIBLING     { $$ = HVML_DOM_XPATH_AXIS_FOLLOWING_SIBLING; }
| NAMESPACE             { $$ = HVML_DOM_XPATH_AXIS_NAMESPACE; }
| PARENT                { $$ = HVML_DOM_XPATH_AXIS_PARENT; }
| PRECEDING             { $$ = HVML_DOM_XPATH_AXIS_PRECEDING; }
| PRECEDING_SIBLING     { $$ = HVML_DOM_XPATH_AXIS_PRECEDING_SIBLING; }
| SELF                  { $$ = HVML_DOM_XPATH_AXIS_SELF; }
;

node_test:
  '*'               { $$ = null_node_test;
                      $$.is_name_test = 1;
                      $$.name_test.local_part = strdup("*");
                      if (!$$.name_test.local_part) {
                        YYABORT;
                      } }
| qname             { $$ = null_node_test;
                      $$.is_name_test = 1;
                      $$.name_test = $1; }
| ncname ':' '*'    { $$ = null_node_test;
                      $$.is_name_test = 1;
                      $$.name_test.prefix = $1;
                      $$.name_test.local_part = strdup("*");
                      if (!$$.name_test.local_part) {
                        YYABORT;
                      } }
| node_type '(' ')' { $$ = null_node_test;
                      $$.is_name_test = 0;
                      $$.node_type = $1; }
;

node_type:
  COMMENT                   { $$ = HVML_DOM_XPATH_NT_COMMENT; }
| TEXT                      { $$ = HVML_DOM_XPATH_NT_TEXT; }
| PROCESSING_INSTRUCTION    { $$ = HVML_DOM_XPATH_NT_PROCESSING_INSTRUCTION; }
| NODE                      { $$ = HVML_DOM_XPATH_NT_NODE; }
| JSON                      { $$ = HVML_DOM_XPATH_NT_JSON; }
;

ncname:
  NCNAME                { $$ = strdup($1); if (!($$)) YYABORT; }
| ANCESTOR              { $$ = strdup($1); if (!($$)) YYABORT; }
| ANCESTOR_OR_SELF      { $$ = strdup($1); if (!($$)) YYABORT; }
| ATTRIBUTE             { $$ = strdup($1); if (!($$)) YYABORT; }
| CHILD                 { $$ = strdup($1); if (!($$)) YYABORT; }
| DESCENDANT            { $$ = strdup($1); if (!($$)) YYABORT; }
| DESCENDANT_OR_SELF    { $$ = strdup($1); if (!($$)) YYABORT; }
| FOLLOWING             { $$ = strdup($1); if (!($$)) YYABORT; }
| FOLLOWING_SIBLING     { $$ = strdup($1); if (!($$)) YYABORT; }
| NAMESPACE             { $$ = strdup($1); if (!($$)) YYABORT; }
| PARENT                { $$ = strdup($1); if (!($$)) YYABORT; }
| PRECEDING             { $$ = strdup($1); if (!($$)) YYABORT; }
| PRECEDING_SIBLING     { $$ = strdup($1); if (!($$)) YYABORT; }
| SELF                  { $$ = strdup($1); if (!($$)) YYABORT; }
| DIV                   { $$ = strdup($1); if (!($$)) YYABORT; }
| MOD                   { $$ = strdup($1); if (!($$)) YYABORT; }
| OR                    { $$ = strdup($1); if (!($$)) YYABORT; }
| AND                   { $$ = strdup($1); if (!($$)) YYABORT; }
;

qname:
  ncname ':' ncname     { $$ = null_qname;
                          $$.prefix = $1; $$.local_part = $3; }
| ncname                { $$ = null_qname;
                          $$.local_part = $1; }
;

predicates:
  predicate             { $$ = null_exprs;
                          if (hvml_dom_xpath_exprs_append_expr(&($$), &($1))) {
                            hvml_dom_xpath_expr_cleanup(&($1));
                            YYABORT;
                          } }
| predicates predicate  { $$ = null_exprs;
                          if (hvml_dom_xpath_exprs_append_expr(&($1), &($2))) {
                            hvml_dom_xpath_exprs_cleanup(&($1));
                            hvml_dom_xpath_expr_cleanup(&($2));
                            YYABORT;
                          }
                          $$ = $1; }
;


predicate:
  '[' expr ']'          { $$ = $2; }

expr:
  unary_expr        { $$ = null_expr; $$.is_binary_op = 0; 
                      $$.unary = (hvml_dom_xpath_union_expr_t*)calloc(1, sizeof(*($$.unary)));
                      if (!$$.unary) { hvml_dom_xpath_union_expr_cleanup(&($1)); YYABORT; }
                      *($$.unary) = $1; }
| expr OR expr      { $$ = null_expr;
                      if (hvml_dom_xpath_expr_gen(&($$), HVML_DOM_XPATH_OP_OR, &($1), &($3))) {
                        hvml_dom_xpath_expr_cleanup(&($1)); hvml_dom_xpath_expr_cleanup(&($3));
                        YYABORT;
                      } }
| expr AND expr     { $$ = null_expr;
                      if (hvml_dom_xpath_expr_gen(&($$), HVML_DOM_XPATH_OP_AND, &($1), &($3))) {
                        hvml_dom_xpath_expr_cleanup(&($1)); hvml_dom_xpath_expr_cleanup(&($3));
                        YYABORT;
                      } }
| expr '=' expr     { $$ = null_expr;
                      if (hvml_dom_xpath_expr_gen(&($$), HVML_DOM_XPATH_OP_EQ, &($1), &($3))) {
                        hvml_dom_xpath_expr_cleanup(&($1)); hvml_dom_xpath_expr_cleanup(&($3));
                        YYABORT;
                      } }
| expr NEQ expr     { $$ = null_expr;
                      if (hvml_dom_xpath_expr_gen(&($$), HVML_DOM_XPATH_OP_NEQ, &($1), &($3))) {
                        hvml_dom_xpath_expr_cleanup(&($1)); hvml_dom_xpath_expr_cleanup(&($3));
                        YYABORT;
                      } }
| expr '<' expr     { $$ = null_expr;
                      if (hvml_dom_xpath_expr_gen(&($$), HVML_DOM_XPATH_OP_LT, &($1), &($3))) {
                        hvml_dom_xpath_expr_cleanup(&($1)); hvml_dom_xpath_expr_cleanup(&($3));
                        YYABORT;
                      } }
| expr '>' expr     { $$ = null_expr;
                      if (hvml_dom_xpath_expr_gen(&($$), HVML_DOM_XPATH_OP_GT, &($1), &($3))) {
                        hvml_dom_xpath_expr_cleanup(&($1)); hvml_dom_xpath_expr_cleanup(&($3));
                        YYABORT;
                      } }
| expr LTE expr     { $$ = null_expr;
                      if (hvml_dom_xpath_expr_gen(&($$), HVML_DOM_XPATH_OP_LTE, &($1), &($3))) {
                        hvml_dom_xpath_expr_cleanup(&($1)); hvml_dom_xpath_expr_cleanup(&($3));
                        YYABORT;
                      } }
| expr GTE expr     { $$ = null_expr;
                      if (hvml_dom_xpath_expr_gen(&($$), HVML_DOM_XPATH_OP_GTE, &($1), &($3))) {
                        hvml_dom_xpath_expr_cleanup(&($1)); hvml_dom_xpath_expr_cleanup(&($3));
                        YYABORT;
                      } }
| expr '+' expr     { $$ = null_expr;
                      if (hvml_dom_xpath_expr_gen(&($$), HVML_DOM_XPATH_OP_PLUS, &($1), &($3))) {
                        hvml_dom_xpath_expr_cleanup(&($1)); hvml_dom_xpath_expr_cleanup(&($3));
                        YYABORT;
                      } }
| expr '-' expr     { $$ = null_expr;
                      if (hvml_dom_xpath_expr_gen(&($$), HVML_DOM_XPATH_OP_MINUS, &($1), &($3))) {
                        hvml_dom_xpath_expr_cleanup(&($1)); hvml_dom_xpath_expr_cleanup(&($3));
                        YYABORT;
                      } }
| expr '*' expr     { $$ = null_expr;
                      if (hvml_dom_xpath_expr_gen(&($$), HVML_DOM_XPATH_OP_MULTI, &($1), &($3))) {
                        hvml_dom_xpath_expr_cleanup(&($1)); hvml_dom_xpath_expr_cleanup(&($3));
                        YYABORT;
                      } }
| expr DIV expr     { $$ = null_expr;
                      if (hvml_dom_xpath_expr_gen(&($$), HVML_DOM_XPATH_OP_DIV, &($1), &($3))) {
                        hvml_dom_xpath_expr_cleanup(&($1)); hvml_dom_xpath_expr_cleanup(&($3));
                        YYABORT;
                      } }
| expr MOD expr     { $$ = null_expr;
                      if (hvml_dom_xpath_expr_gen(&($$), HVML_DOM_XPATH_OP_MOD, &($1), &($3))) {
                        hvml_dom_xpath_expr_cleanup(&($1)); hvml_dom_xpath_expr_cleanup(&($3));
                        YYABORT;
                      } }
;

unary_expr:
  union_expr                    { $$ = $1; $$.uminus = 0; }
| '-' unary_expr %prec UMINUS   { $$ = $2; $$.uminus = $2.uminus ? 0 : 1; }
;

union_expr:
  path_expr                 { $$.paths = (hvml_dom_xpath_path_expr_t*)calloc(1, sizeof(*($$.paths)));
                              if (!$$.paths) {
                                hvml_dom_xpath_path_expr_cleanup(&($1));
                                YYABORT;
                              }
                              *($$.paths) = $1;
                              $$.npaths = 1; }
| union_expr '|' path_expr  { hvml_dom_xpath_path_expr_t *paths = $$.paths;
                              paths = (hvml_dom_xpath_path_expr_t*)realloc(paths, ($1.npaths+1) * sizeof(*paths));
                              if (!paths) {
                                hvml_dom_xpath_union_expr_cleanup(&($1));
                                hvml_dom_xpath_path_expr_cleanup(&($3));
                                YYABORT;
                              }
                              paths[$1.npaths] = $3;
                              $$.paths = paths;
                              $$.npaths += 1; }

;

path_expr:
  location_path                             { $$ = null_path_expr; $$.is_location = 1; $$.location = $1; }
| filter_expr                               { $$ = null_path_expr; $$.filter_expr = $1; }
| filter_expr '/' relative_location_path    { $$ = null_path_expr; $$.filter_expr = $1;
                                              if (hvml_dom_xpath_steps_append_slash(&($$.location))) {
                                                hvml_dom_xpath_path_expr_cleanup(&($$));
                                                hvml_dom_xpath_steps_cleanup(&($3));
                                                YYABORT;
                                              }
                                              if (hvml_dom_xpath_steps_append_steps(&($$.location), &($3))) {
                                                hvml_dom_xpath_path_expr_cleanup(&($$));
                                                hvml_dom_xpath_steps_cleanup(&($3));
                                                YYABORT;
                                              } }
| filter_expr SLASH2 relative_location_path { $$ = null_path_expr; $$.filter_expr = $1;
                                              if (hvml_dom_xpath_steps_append_slash2(&($$.location))) {
                                                hvml_dom_xpath_path_expr_cleanup(&($$));
                                                hvml_dom_xpath_steps_cleanup(&($3));
                                                YYABORT;
                                              }
                                              if (hvml_dom_xpath_steps_append_steps(&($$.location), &($3))) {
                                                hvml_dom_xpath_path_expr_cleanup(&($$));
                                                hvml_dom_xpath_steps_cleanup(&($3));
                                                YYABORT;
                                              } }
;

filter_expr:
  primary_expr          { $$ = null_filter_expr; $$.primary = $1; }
| filter_expr predicate { $$ = null_filter_expr;
                          hvml_dom_xpath_expr_t *exprs = $1.exprs.exprs;
                          exprs = (hvml_dom_xpath_expr_t*)realloc(exprs, ($1.exprs.nexprs+1) * sizeof(*exprs));
                          int ok = 0;
                          do {
                            if (!exprs) break;
                            exprs[$1.exprs.nexprs] = $2;
                            $$.exprs.exprs   = exprs;
                            $$.exprs.nexprs  = $1.exprs.nexprs + 1;
                            $$.primary = $1.primary;
                            ok = 1;
                          } while (0);
                          if (!ok) {
                            hvml_dom_xpath_filter_expr_cleanup(&($1));
                            hvml_dom_xpath_expr_cleanup(&($2));
                            YYABORT;
                          } }
;

literal:
  LITERAL           { $$ = strdup($1); if (!$$) YYABORT; }
| LITERAL2          { $$ = strdup($1); if (!$$) YYABORT; }

primary_expr:
  '$' qname         { $$ = null_primary; $$.primary_type = HVML_DOM_XPATH_PRIMARY_VARIABLE; $$.variable = $2; }
| '(' expr ')'      { $$ = null_primary; $$.primary_type = HVML_DOM_XPATH_PRIMARY_EXPR; $$.expr = $2; }
| '"' literal '"'   { $$ = null_primary; $$.primary_type = HVML_DOM_XPATH_PRIMARY_LITERAL; $$.literal = $2;
                      if (!($$.literal)) { hvml_dom_xpath_primary_cleanup(&($$)); free($2); YYABORT; } }
| SQ literal SQ     { $$ = null_primary; $$.primary_type = HVML_DOM_XPATH_PRIMARY_LITERAL; $$.literal = $2;
                      if (!($$.literal)) { hvml_dom_xpath_primary_cleanup(&($$)); free($2); YYABORT; } }
| INTEGER           { $$ = null_primary; int64_t v; sscanf($1, "%" PRId64 "", &v);
                      $$.primary_type = HVML_DOM_XPATH_PRIMARY_NUMBER; $$.ldbl = v; }
| DOUBLE            { $$ = null_primary; long double v; sscanf($1, "%Lf", &v);
                      $$.primary_type = HVML_DOM_XPATH_PRIMARY_NUMBER; $$.ldbl = v; }
| function_call     { $$ = null_primary;
                      $$.primary_type = HVML_DOM_XPATH_PRIMARY_FUNC; $$.func_call = $1; }
;

function_call:
  function_name '(' ')'             { $$ = null_func; $$.func = $1; }
| function_name '(' arguments ')'   { $$ = null_func; $$.func = $1; $$.args = $3; }
;

function_name:
  qname     { $$ = HVML_DOM_XPATH_PREDEFINED_FUNC_UNSPECIFIED;
              if (!check_func_name($1.prefix, $1.local_part)) {
                hvml_dom_xpath_qname_cleanup(&($1));
                YYABORT;
              } else {
                if ($1.prefix) { hvml_dom_xpath_qname_cleanup(&($1)); YYABORT; }
                if (strcmp($1.local_part, "position")==0) {
                    hvml_dom_xpath_qname_cleanup(&($1));
                    $$ = HVML_DOM_XPATH_PREDEFINED_FUNC_POSITION;
                } else if (strcmp($1.local_part, "last")==0) {
                    hvml_dom_xpath_qname_cleanup(&($1));
                    $$ = HVML_DOM_XPATH_PREDEFINED_FUNC_LAST;
                } else {
                    hvml_dom_xpath_qname_cleanup(&($1));
                    YYABORT;
                }
              } }
;

arguments:
  argument              { $$ = null_exprs;
                          if (hvml_dom_xpath_exprs_append_expr(&($$), &($1))) {
                            hvml_dom_xpath_expr_cleanup(&($1));
                            YYABORT;
                          } }
| arguments ',' expr    { $$ = null_exprs;
                          if (hvml_dom_xpath_exprs_append_expr(&($1), &($3))) {
                            hvml_dom_xpath_exprs_cleanup(&($1));
                            hvml_dom_xpath_expr_cleanup(&($3));
                            YYABORT;
                          }
                          $$ = $1; }
;

argument:
  expr  { $$ = $1; }
;





%%


/* Called by yyparse on error. */
void yyerror(YYLTYPE *yylloc, int param, void *arg, char const *s) {
    (void)param;
    (void)arg;
    (void)yylloc;
    D("%s: (%d,%d)->(%d,%d)", s,
      yylloc->first_line, yylloc->first_column,
      yylloc->last_line, yylloc->last_column);
}

int hvml_dom_xpath_parse(const char *xpath, hvml_dom_xpath_steps_t *steps) {
    A(memcmp(steps, &null_steps, sizeof(null_steps))==0, "internal logic error");
    yyscan_t arg = {0};
    hvml_dom_xpath_yylex_init(&arg);
    // hvml_dom_xpath_yyset_in(in, arg);
    hvml_dom_xpath_yyset_debug(1, arg);
    hvml_dom_xpath_yyset_extra(steps, arg);
    hvml_dom_xpath_yy_scan_string(xpath, arg);
    int p = 0;
    int ret =hvml_dom_xpath_yyparse(p, arg);
    hvml_dom_xpath_yylex_destroy(arg);
    return ret ? 1 : 0;
}

