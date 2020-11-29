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

#include "hvml/hvml_printf.h"

#include "hvml/hvml_log.h"
#include "hvml/hvml_json_parser.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

typedef struct jo_value_printf_s        jo_value_printf_t;
struct jo_value_printf_s {
    int             lvl;
    FILE           *out;
};

static int traverse_for_printf(hvml_jo_value_t *jo, int lvl, int action, void *arg) {
    jo_value_printf_t *parg = (jo_value_printf_t*)arg;
    int breakout = 0;
    hvml_jo_value_t *parent = hvml_jo_value_owner(jo);
    hvml_jo_value_t *prev   = hvml_jo_value_sibling_prev(jo);
    switch (hvml_jo_value_type(jo)) {
        case MKJOT(J_TRUE): {
            A(action==0, "internal logic error");
            if (parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT_KV)) {
                fprintf(parg->out, ":");
            } else if (prev) {
                fprintf(parg->out, ",");
            }
            fprintf(parg->out, "true");
        } break;
        case MKJOT(J_FALSE): {
            A(action==0, "internal logic error");
            if (parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT_KV)) {
                fprintf(parg->out, ":");
            } else if (prev) {
                fprintf(parg->out, ",");
            }
            fprintf(parg->out, "false");
        } break;
        case MKJOT(J_NULL): {
            A(action==0, "internal logic error");
            if (parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT_KV)) {
                fprintf(parg->out, ":");
            } else if (prev) {
                fprintf(parg->out, ",");
            }
            fprintf(parg->out, "null");
        } break;
        case MKJOT(J_NUMBER): {
            A(action==0, "internal logic error");
            if (parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT_KV)) {
                fprintf(parg->out, ":");
            } else if (prev) {
                fprintf(parg->out, ",");
            }
            long double d;
            const char *s;
            A(0==hvml_jo_number_get(jo, &d, &s), "internal logic error");
            int prec = strlen(s);
            fprintf(parg->out, "%.*Lg", prec, d);
        } break;
        case MKJOT(J_STRING): {
            A(action==0, "internal logic error");
            if (parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT_KV)) {
                fprintf(parg->out, ":");
            } else if (prev) {
                fprintf(parg->out, ",");
            }
            const char *s;
            if (!hvml_jo_string_get(jo, &s)) {
                hvml_json_str_printf(parg->out, s, s ? strlen(s) : 0);
            }
        } break;
        case MKJOT(J_OBJECT): {
            switch (action) {
                case 1: {
                    if (parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT_KV)) {
                        fprintf(parg->out, ":");
                    } else if (prev) {
                        fprintf(parg->out, ",");
                    }
                    fprintf(parg->out, "{"); // "}"
                } break;
                case -1: {
                    // "{"
                    fprintf(parg->out, "}");
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        case MKJOT(J_OBJECT_KV): {
            switch (action) {
                case 1: {
                    if (prev) {
                        fprintf(parg->out, ",");
                    }
                    const char      *key;
                    A(0==hvml_jo_kv_get(jo, &key, NULL), "internal logic error");
                    A(key, "internal logic error");
                    A(parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT), "internal logic error");
                    hvml_json_str_printf(parg->out, key, strlen(key));
                } break;
                case -1: {
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        case MKJOT(J_ARRAY): {
            switch (action) {
                case 1: {
                    if (parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT_KV)) {
                        fprintf(parg->out, ":");
                    } else if (prev) {
                        fprintf(parg->out, ",");
                    }
                    fprintf(parg->out, "["); // "]";
                } break;
                case -1: {
                    // "["
                    fprintf(parg->out, "]");
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        default: {
            A(0, "print json type [%d]: not implemented yet", hvml_jo_value_type(jo));
        } break;
    }
    return 0;
}

void hvml_jo_value_printf(hvml_jo_value_t *jo, FILE *out) {
    jo_value_printf_t parg;
    parg.lvl = -1;
    parg.out = out;
    hvml_jo_value_traverse(jo, &parg, traverse_for_printf);
}


