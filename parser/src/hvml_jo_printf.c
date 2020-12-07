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
#include "hvml/hvml_string.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

typedef struct jo_value_printf_s        jo_value_printf_t;
struct jo_value_printf_s {
    unsigned int    failed:1;
    int             lvl;
    hvml_stream_t  *stream;
};

static int traverse_for_printf(hvml_jo_value_t *jo, int lvl, int action, void *arg) {
    jo_value_printf_t *parg = (jo_value_printf_t*)arg;
    if (parg->failed) return -1;

    hvml_jo_value_t *parent = hvml_jo_value_owner(jo);
    hvml_jo_value_t *prev   = hvml_jo_value_sibling_prev(jo);
    int r = 0;
    switch (hvml_jo_value_type(jo)) {
        case MKJOT(J_TRUE): {
            A(action==0, "internal logic error");
            if (parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT_KV)) {
                r = hvml_stream_printf(parg->stream, ":");
            } else if (prev) {
                r = hvml_stream_printf(parg->stream, ",");
            }
            if (r<0) break;
            r = hvml_stream_printf(parg->stream, "true");
        } break;
        case MKJOT(J_FALSE): {
            A(action==0, "internal logic error");
            if (parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT_KV)) {
                r = hvml_stream_printf(parg->stream, ":");
            } else if (prev) {
                r = hvml_stream_printf(parg->stream, ",");
            }
            if (r<0) break;
            r = hvml_stream_printf(parg->stream, "false");
        } break;
        case MKJOT(J_NULL): {
            A(action==0, "internal logic error");
            if (parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT_KV)) {
                r = hvml_stream_printf(parg->stream, ":");
            } else if (prev) {
                r = hvml_stream_printf(parg->stream, ",");
            }
            if (r<0) break;
            r = hvml_stream_printf(parg->stream, "null");
        } break;
        case MKJOT(J_NUMBER): {
            A(action==0, "internal logic error");
            if (parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT_KV)) {
                r = hvml_stream_printf(parg->stream, ":");
            } else if (prev) {
                r = hvml_stream_printf(parg->stream, ",");
            }
            if (r<0) break;
            long double d;
            const char *s;
            A(0==hvml_jo_number_get(jo, &d, &s), "internal logic error");
            int prec = strlen(s);
            r = hvml_stream_printf(parg->stream, "%.*Lg", prec, d);
        } break;
        case MKJOT(J_STRING): {
            A(action==0, "internal logic error");
            if (parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT_KV)) {
                r = hvml_stream_printf(parg->stream, ":");
            } else if (prev) {
                r = hvml_stream_printf(parg->stream, ",");
            }
            if (r<0) break;
            const char *s;
            if (!hvml_jo_string_get(jo, &s)) {
                hvml_json_str_serialize(parg->stream, s, s ? strlen(s) : 0);
            }
        } break;
        case MKJOT(J_OBJECT): {
            switch (action) {
                case 1: {
                    if (parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT_KV)) {
                        r = hvml_stream_printf(parg->stream, ":");
                    } else if (prev) {
                        r = hvml_stream_printf(parg->stream, ",");
                    }
                    if (r<0) break;
                    r = hvml_stream_printf(parg->stream, "{"); // "}"
                } break;
                case -1: {
                    // "{"
                    r = hvml_stream_printf(parg->stream, "}");
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
                        r = hvml_stream_printf(parg->stream, ",");
                        if (r<0) break;
                    }
                    const char      *key;
                    A(0==hvml_jo_kv_get(jo, &key, NULL), "internal logic error");
                    A(key, "internal logic error");
                    A(parent && hvml_jo_value_type(parent)==MKJOT(J_OBJECT), "internal logic error");
                    r = hvml_json_str_serialize(parg->stream, key, strlen(key));
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
                        r = hvml_stream_printf(parg->stream, ":");
                    } else if (prev) {
                        r = hvml_stream_printf(parg->stream, ",");
                    }
                    if (r<0) break;
                    r = hvml_stream_printf(parg->stream, "["); // "]";
                } break;
                case -1: {
                    // "["
                    r = hvml_stream_printf(parg->stream, "]");
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
    if (r<0) parg->failed = 1;
    return r<0 ? -1 : 0;
}

int hvml_jo_value_serialize(hvml_jo_value_t *jo, hvml_stream_t *stream) {
    jo_value_printf_t parg;
    parg.failed    = 0;
    parg.lvl       = -1;
    parg.stream    = stream;
    if (!parg.stream) return -1;

    hvml_jo_value_traverse(jo, &parg, traverse_for_printf);

    return parg.failed ? -1 : 0;
}

int hvml_jo_value_printf(hvml_jo_value_t *jo, FILE *out) {
    hvml_stream_t *stream = hvml_stream_bind_file(out, 0);
    if (!stream) return -1;

    int r = hvml_jo_value_serialize(jo, stream);

    hvml_stream_destroy(stream);

    return r;
}

int hvml_jo_value_serialize_string(hvml_jo_value_t *jo, hvml_string_t *str) {
    jo_value_printf_t parg;
    parg.failed    = 0;
    parg.lvl       = -1;
    parg.stream    = hvml_stream_bind_string(str);
    if (!parg.stream) return -1;

    hvml_jo_value_traverse(jo, &parg, traverse_for_printf);

    hvml_stream_destroy(parg.stream);

    return parg.failed ? -1 : 0;
}

