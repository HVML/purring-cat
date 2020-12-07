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

#include "hvml_dom_xpath_parser.h"

#include "hvml/hvml_log.h"

const char *hvml_dom_xpath_dot  = ".";
const char *hvml_dom_xpath_dot2 = "..";
const hvml_dom_xpath_step_t            null_step                 = {0};
const hvml_dom_xpath_steps_t           null_steps                = {0};
const hvml_dom_xpath_node_test_t       null_node_test            = {0};
const hvml_dom_xpath_qname_t           null_qname                = {0};
const hvml_dom_xpath_primary_t         null_primary              = {0};
const hvml_dom_xpath_filter_expr_t     null_filter_expr          = {0};
const hvml_dom_xpath_path_expr_t       null_path_expr            = {0};
const hvml_dom_xpath_union_expr_t      null_union_expr           = {0};
const hvml_dom_xpath_func_t            null_func                 = {0};
const hvml_dom_xpath_expr_t            null_expr                 = {0};
const hvml_dom_xpath_exprs_t           null_exprs                = {0};

void hvml_dom_xpath_qname_cleanup(hvml_dom_xpath_qname_t *qname) {
    if (!qname) return;

    if (qname->prefix) {
        free(qname->prefix);
        qname->prefix = NULL;
    }
    if (qname->local_part) {
        free(qname->local_part);
        qname->local_part = NULL;
    }
}

void hvml_dom_xpath_node_test_cleanup(hvml_dom_xpath_node_test_t *node_test) {
    if (!node_test) return;
    if (node_test->is_cleanedup) return;

    if (node_test->is_name_test) {
        hvml_dom_xpath_qname_cleanup(&node_test->name_test);
    }

    node_test->is_cleanedup = 1;
}

void hvml_dom_xpath_step_cleanup(hvml_dom_xpath_step_t *step) {
    if (!step) return;
    if (step->is_cleanedup) return;

    hvml_dom_xpath_node_test_cleanup(&step->node_test);
    hvml_dom_xpath_exprs_cleanup(&step->exprs);

    step->is_cleanedup = 1;
}

void hvml_dom_xpath_steps_cleanup(hvml_dom_xpath_steps_t *steps) {
    if (!steps) return;

    for (size_t i=0; i<steps->nsteps; ++i) {
        hvml_dom_xpath_step_cleanup(steps->steps + i);
    }
    free(steps->steps);
    steps->steps   = NULL;
    steps->nsteps  = 0;
}

void hvml_dom_xpath_func_cleanup(hvml_dom_xpath_func_t *func) {
    if (!func) return;
    if (func->is_cleanedup) return;

    hvml_dom_xpath_exprs_cleanup(&func->args);

    func->is_cleanedup = 1;
}

void hvml_dom_xpath_expr_cleanup(hvml_dom_xpath_expr_t *expr) {
    if (!expr) return;

    if (expr->is_binary_op) {
        hvml_dom_xpath_expr_destroy(expr->left);  expr->left   = NULL;
        hvml_dom_xpath_expr_destroy(expr->right); expr->right  = NULL;
    } else {
        hvml_dom_xpath_union_expr_destroy(expr->unary);
        expr->unary = NULL;
    }
}

void hvml_dom_xpath_primary_cleanup(hvml_dom_xpath_primary_t *primary) {
    if (!primary) return;
    if (primary->is_cleanedup) return;

    switch (primary->primary_type) {
        case HVML_DOM_XPATH_PRIMARY_UNSPECIFIED: break;
        case HVML_DOM_XPATH_PRIMARY_VARIABLE: {
            hvml_dom_xpath_qname_cleanup(&primary->variable);
        } break;
        case HVML_DOM_XPATH_PRIMARY_EXPR: {
            hvml_dom_xpath_expr_cleanup(&primary->expr);
        } break;
        case HVML_DOM_XPATH_PRIMARY_NUMBER:  break;
        case HVML_DOM_XPATH_PRIMARY_LITERAL: {
            free(primary->literal);
            primary->literal = NULL;
        } break;
        case HVML_DOM_XPATH_PRIMARY_FUNC: {
            hvml_dom_xpath_func_cleanup(&primary->func_call);
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }

    primary->is_cleanedup = 1;
}

void hvml_dom_xpath_filter_expr_cleanup(hvml_dom_xpath_filter_expr_t *filter_expr) {
    if (!filter_expr) return;
    if (filter_expr->is_cleanedup) return;

    hvml_dom_xpath_primary_cleanup(&filter_expr->primary);
    hvml_dom_xpath_exprs_cleanup(&filter_expr->exprs);

    filter_expr->is_cleanedup = 1;
}

void hvml_dom_xpath_path_expr_cleanup(hvml_dom_xpath_path_expr_t *path_expr) {
    if (!path_expr) return;
    if (path_expr->is_cleanedup) return;

    hvml_dom_xpath_filter_expr_cleanup(&path_expr->filter_expr);
    hvml_dom_xpath_steps_cleanup(&path_expr->location);

    path_expr->is_cleanedup = 1;
}

void hvml_dom_xpath_union_expr_cleanup(hvml_dom_xpath_union_expr_t *union_expr) {
    if (!union_expr) return;
    if (union_expr->is_cleanedup) return;

    for (size_t i=0; i<union_expr->npaths; ++i) {
        hvml_dom_xpath_path_expr_cleanup(union_expr->paths + i);
    }
    free(union_expr->paths);
    union_expr->paths = NULL;
    union_expr->npaths = 0;

    union_expr->is_cleanedup = 1;
}

void hvml_dom_xpath_exprs_cleanup(hvml_dom_xpath_exprs_t *exprs) {
    if (!exprs) return;

    for (size_t i=0; i<exprs->nexprs; ++i) {
        hvml_dom_xpath_expr_cleanup(exprs->exprs + i);
    }

    free(exprs->exprs);
    exprs->exprs   = NULL;
    exprs->nexprs  = 0;
}

void hvml_dom_xpath_qname_destroy(hvml_dom_xpath_qname_t *qname) {
    if (!qname) return;

    hvml_dom_xpath_qname_cleanup(qname);

    free(qname);
}

void hvml_dom_xpath_node_test_destroy(hvml_dom_xpath_node_test_t *node_test) {
    if (!node_test) return;

    hvml_dom_xpath_node_test_cleanup(node_test);

    free(node_test);
}

void hvml_dom_xpath_step_destroy(hvml_dom_xpath_step_t *step) {
    if (!step) return;

    hvml_dom_xpath_step_cleanup(step);

    free(step);
}

void hvml_dom_xpath_steps_destroy(hvml_dom_xpath_steps_t *steps) {
    if (!steps) return;

    hvml_dom_xpath_steps_cleanup(steps);

    free(steps);
}

void hvml_dom_xpath_func_destroy(hvml_dom_xpath_func_t *func) {
    if (!func) return;

    hvml_dom_xpath_func_cleanup(func);

    free(func);
}

void hvml_dom_xpath_expr_destroy(hvml_dom_xpath_expr_t *expr) {
    if (!expr) return;

    hvml_dom_xpath_expr_cleanup(expr);

    free(expr);
}

void hvml_dom_xpath_primary_destroy(hvml_dom_xpath_primary_t *primary) {
    if (!primary) return;

    hvml_dom_xpath_primary_cleanup(primary);

    free(primary);
}

void hvml_dom_xpath_filter_expr_destroy(hvml_dom_xpath_filter_expr_t *filter_expr) {
    if (!filter_expr) return;

    hvml_dom_xpath_filter_expr_cleanup(filter_expr);

    free(filter_expr);
}

void hvml_dom_xpath_path_expr_destroy(hvml_dom_xpath_path_expr_t *path_expr) {
    if (!path_expr) return;

    hvml_dom_xpath_path_expr_cleanup(path_expr);

    free(path_expr);
}

void hvml_dom_xpath_union_expr_destroy(hvml_dom_xpath_union_expr_t *union_expr) {
    if (!union_expr) return;

    hvml_dom_xpath_union_expr_cleanup(union_expr);

    free(union_expr);
}

void hvml_dom_xpath_exprs_destroy(hvml_dom_xpath_exprs_t *exprs) {
    if (!exprs) return;

    hvml_dom_xpath_exprs_cleanup(exprs);

    free(exprs);
}

int hvml_dom_xpath_exprs_append_expr(hvml_dom_xpath_exprs_t *exprs, hvml_dom_xpath_expr_t *expr) {
    A(exprs && expr, "internal logic error");

    hvml_dom_xpath_expr_t *e = (hvml_dom_xpath_expr_t*)exprs->exprs;
    size_t                 n = exprs->nexprs;
    e = (hvml_dom_xpath_expr_t*)realloc(e, (n+1)*sizeof(*e));
    if (!e) return -1;
    e[n]           = *expr;
    exprs->exprs   = e;
    exprs->nexprs += 1;
    *expr          = null_expr;
    return 0;
}

