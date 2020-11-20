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

#ifndef _hvml_dom_xpath_parser_h_
#define _hvml_dom_xpath_parser_h_

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char *hvml_dom_xpath_dot;
extern const char *hvml_dom_xpath_dot2;

typedef enum {
    HVML_DOM_XPATH_OP_UNSPECIFIED,
    HVML_DOM_XPATH_OP_OR,
    HVML_DOM_XPATH_OP_AND,
    HVML_DOM_XPATH_OP_EQ,
    HVML_DOM_XPATH_OP_NEQ,
    HVML_DOM_XPATH_OP_LT,
    HVML_DOM_XPATH_OP_GT,
    HVML_DOM_XPATH_OP_LTE,
    HVML_DOM_XPATH_OP_GTE,
    HVML_DOM_XPATH_OP_PLUS,
    HVML_DOM_XPATH_OP_MINUS,
    HVML_DOM_XPATH_OP_MULTI,
    HVML_DOM_XPATH_OP_DIV,
    HVML_DOM_XPATH_OP_MOD,
} HVML_DOM_XPATH_OP_TYPE;

typedef enum {
    HVML_DOM_XPATH_AXIS_UNSPECIFIED,
    HVML_DOM_XPATH_AXIS_ANCESTOR,
    HVML_DOM_XPATH_AXIS_ANCESTOR_OR_SELF,
    HVML_DOM_XPATH_AXIS_ATTRIBUTE,
    HVML_DOM_XPATH_AXIS_CHILD,
    HVML_DOM_XPATH_AXIS_DESCENDANT,
    HVML_DOM_XPATH_AXIS_DESCENDANT_OR_SELF,
    HVML_DOM_XPATH_AXIS_FOLLOWING,
    HVML_DOM_XPATH_AXIS_FOLLOWING_SIBLING,
    HVML_DOM_XPATH_AXIS_NAMESPACE,
    HVML_DOM_XPATH_AXIS_PARENT,
    HVML_DOM_XPATH_AXIS_PRECEDING,
    HVML_DOM_XPATH_AXIS_PRECEDING_SIBLING,
    HVML_DOM_XPATH_AXIS_SELF,
    // extension
    HVML_DOM_XPATH_AXIS_SLASH,  /* "/" */
    HVML_DOM_XPATH_AXIS_SLASH2  /* "//" */
} HVML_DOM_XPATH_AXIS_TYPE;

typedef enum {
    HVML_DOM_XPATH_NT_UNSPECIFIED,
    HVML_DOM_XPATH_NT_COMMENT,
    HVML_DOM_XPATH_NT_TEXT,
    HVML_DOM_XPATH_NT_PROCESSING_INSTRUCTION,
    HVML_DOM_XPATH_NT_NODE,
    HVML_DOM_XPATH_NT_JSON,     /* extension */
} HVML_DOM_XPATH_NT_TYPE;

typedef enum {
    HVML_DOM_XPATH_AS_UNSPECIFIED,
    HVML_DOM_XPATH_AS_PARENT,
    HVML_DOM_XPATH_AS_GRANDPARENT
} HVML_DOM_XPATH_AS_TYPE;

typedef enum {
    HVML_DOM_XPATH_PRIMARY_UNSPECIFIED,
    HVML_DOM_XPATH_PRIMARY_VARIABLE,
    HVML_DOM_XPATH_PRIMARY_EXPR,
    HVML_DOM_XPATH_PRIMARY_INTEGER,
    HVML_DOM_XPATH_PRIMARY_DOUBLE,
    HVML_DOM_XPATH_PRIMARY_LITERAL,
    HVML_DOM_XPATH_PRIMARY_FUNC
} HVML_DOM_XPATH_PRIMARY_TYPE;

typedef enum {
    HVML_DOM_XPATH_PREDEFINED_FUNC_UNSPECIFIED,
    HVML_DOM_XPATH_PREDEFINED_FUNC_POSITION,
    HVML_DOM_XPATH_PREDEFINED_FUNC_LAST
} HVML_DOM_XPATH_PREDEFINED_FUNC_TYPE;

typedef struct hvml_dom_xpath_qname_s          hvml_dom_xpath_qname_t;
typedef struct hvml_dom_xpath_node_test_s      hvml_dom_xpath_node_test_t;
typedef struct hvml_dom_xpath_step_axis_s      hvml_dom_xpath_step_axis_t;
typedef struct hvml_dom_xpath_step_s           hvml_dom_xpath_step_t;
typedef struct hvml_dom_xpath_steps_s          hvml_dom_xpath_steps_t;
typedef struct hvml_dom_xpath_func_s           hvml_dom_xpath_func_t;
typedef struct hvml_dom_xpath_expr_s           hvml_dom_xpath_expr_t;
typedef struct hvml_dom_xpath_primary_s        hvml_dom_xpath_primary_t;
typedef struct hvml_dom_xpath_filter_expr_s    hvml_dom_xpath_filter_expr_t;
typedef struct hvml_dom_xpath_path_expr_s      hvml_dom_xpath_path_expr_t;
typedef struct hvml_dom_xpath_union_expr_s     hvml_dom_xpath_union_expr_t;
typedef struct hvml_dom_xpath_exprs_s          hvml_dom_xpath_exprs_t;

struct hvml_dom_xpath_qname_s {
    char           *prefix;
    char           *local_part;
};

struct hvml_dom_xpath_node_test_s {
    unsigned int is_cleanedup:1;
    unsigned int is_name_test:1;
    union {
        hvml_dom_xpath_qname_t          name_test;
        HVML_DOM_XPATH_NT_TYPE          node_type;
    };
};

struct hvml_dom_xpath_exprs_s {
    hvml_dom_xpath_expr_t          *exprs;
    size_t                          nexprs;
};

struct hvml_dom_xpath_step_axis_s {
    unsigned int is_cleanedup:1;
    HVML_DOM_XPATH_AXIS_TYPE          axis;
    hvml_dom_xpath_node_test_t        node_test;
    hvml_dom_xpath_exprs_t            exprs;
};

struct hvml_dom_xpath_step_s {
    unsigned int is_cleanedup:1;
    unsigned int is_axis:1;
    union {
        hvml_dom_xpath_step_axis_t                 axis;
        HVML_DOM_XPATH_AS_TYPE                     abbre;
    };
};

struct hvml_dom_xpath_steps_s {
    hvml_dom_xpath_step_t           *steps;
    size_t                           nsteps;
};

struct hvml_dom_xpath_func_s {
    unsigned int is_cleanedup:1;
    HVML_DOM_XPATH_PREDEFINED_FUNC_TYPE  func;
    hvml_dom_xpath_exprs_t               args;
};

struct hvml_dom_xpath_expr_s {
    unsigned int is_binary_op:1;

    hvml_dom_xpath_union_expr_t    *unary;

    HVML_DOM_XPATH_OP_TYPE         op_type;
    hvml_dom_xpath_expr_t          *left;
    hvml_dom_xpath_expr_t          *right;
};

struct hvml_dom_xpath_primary_s {
    unsigned int is_cleanedup:1;
    HVML_DOM_XPATH_PRIMARY_TYPE primary_type;
    union {
        hvml_dom_xpath_qname_t  variable;
        hvml_dom_xpath_expr_t   expr;
        int64_t                 integer;
        double                  dbl;
        char                   *literal;
        hvml_dom_xpath_func_t   func_call;
    };
};

struct hvml_dom_xpath_filter_expr_s {
    unsigned int is_cleanedup:1;
    hvml_dom_xpath_primary_t  primary;
    hvml_dom_xpath_exprs_t    exprs;
};

struct hvml_dom_xpath_path_expr_s {
    unsigned int is_cleanedup:1;
    unsigned int is_location:1;
    hvml_dom_xpath_filter_expr_t    filter_expr;
    hvml_dom_xpath_steps_t          location;
};

struct hvml_dom_xpath_union_expr_s {
    unsigned int is_cleanedup:1;
    hvml_dom_xpath_path_expr_t    *paths;
    size_t                         npaths;
    int                            uminus;
};

extern const hvml_dom_xpath_step_t            null_step;
extern const hvml_dom_xpath_steps_t           null_steps;
extern const hvml_dom_xpath_node_test_t       null_node_test;
extern const hvml_dom_xpath_qname_t           null_qname;
extern const hvml_dom_xpath_primary_t         null_primary;
extern const hvml_dom_xpath_filter_expr_t     null_filter_expr;
extern const hvml_dom_xpath_path_expr_t       null_path_expr;
extern const hvml_dom_xpath_union_expr_t      null_union_expr;
extern const hvml_dom_xpath_func_t            null_func;
extern const hvml_dom_xpath_expr_t            null_expr;
extern const hvml_dom_xpath_exprs_t           null_exprs;

void hvml_dom_xpath_qname_cleanup(hvml_dom_xpath_qname_t *qname);
void hvml_dom_xpath_node_test_cleanup(hvml_dom_xpath_node_test_t *node_test);
void hvml_dom_xpath_step_axis_cleanup(hvml_dom_xpath_step_axis_t *step);
void hvml_dom_xpath_step_cleanup(hvml_dom_xpath_step_t *step);
void hvml_dom_xpath_steps_cleanup(hvml_dom_xpath_steps_t *steps);
void hvml_dom_xpath_func_cleanup(hvml_dom_xpath_func_t *func);
void hvml_dom_xpath_expr_cleanup(hvml_dom_xpath_expr_t *expr);
void hvml_dom_xpath_primary_cleanup(hvml_dom_xpath_primary_t *primary);
void hvml_dom_xpath_filter_expr_cleanup(hvml_dom_xpath_filter_expr_t *filter_expr);
void hvml_dom_xpath_path_expr_cleanup(hvml_dom_xpath_path_expr_t *path_expr);
void hvml_dom_xpath_union_expr_cleanup(hvml_dom_xpath_union_expr_t *union_expr);
void hvml_dom_xpath_exprs_cleanup(hvml_dom_xpath_exprs_t *exprs);

void hvml_dom_xpath_qname_destroy(hvml_dom_xpath_qname_t *qname);
void hvml_dom_xpath_node_test_destroy(hvml_dom_xpath_node_test_t *node_test);
void hvml_dom_xpath_step_axis_destroy(hvml_dom_xpath_step_axis_t *step);
void hvml_dom_xpath_step_destroy(hvml_dom_xpath_step_t *step);
void hvml_dom_xpath_steps_destroy(hvml_dom_xpath_steps_t *steps);
void hvml_dom_xpath_func_destroy(hvml_dom_xpath_func_t *func);
void hvml_dom_xpath_expr_destroy(hvml_dom_xpath_expr_t *expr);
void hvml_dom_xpath_primary_destroy(hvml_dom_xpath_primary_t *primary);
void hvml_dom_xpath_filter_expr_destroy(hvml_dom_xpath_filter_expr_t *filter_expr);
void hvml_dom_xpath_path_expr_destroy(hvml_dom_xpath_path_expr_t *path_expr);
void hvml_dom_xpath_union_expr_destroy(hvml_dom_xpath_union_expr_t *union_expr);
void hvml_dom_xpath_exprs_destroy(hvml_dom_xpath_exprs_t *exprs);

int hvml_dom_xpath_parse(const char *xpath, hvml_dom_xpath_steps_t *steps);


#ifdef __cplusplus
}
#endif

#endif // _hvml_dom_xpath_parser_h_

