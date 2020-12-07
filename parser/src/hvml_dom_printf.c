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
#include "hvml/hvml_string.h"

#include <stdio.h>
#include <string.h>

typedef struct dom_printf_s        dom_printf_t;
struct dom_printf_s {
    unsigned int    rooted:1;
    unsigned int    failed:1;
    int             lvl;
    hvml_stream_t  *stream;
};

static void traverse_for_printf(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout);

int hvml_dom_serialize(hvml_dom_t *dom, hvml_stream_t *stream) {
    dom_printf_t parg;
    parg.rooted     = 0;
    parg.failed     = 0;
    parg.lvl        = -1;
    parg.stream     = stream;
    if (!parg.stream) return -1;

    hvml_dom_traverse(dom, &parg, traverse_for_printf);

    return parg.failed ? -1 : 0;
}

int hvml_dom_printf(hvml_dom_t *dom, FILE *out) {
    hvml_stream_t *stream = hvml_stream_bind_file(out, 0);
    if (!stream) return -1;

    int r = hvml_dom_serialize(dom, stream);

    hvml_stream_destroy(stream);

    return r;
}

static void traverse_for_printf(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout) {
    dom_printf_t *parg = (dom_printf_t*)arg;
    A(parg, "internal logic error");
    A(lvl>=0, "internal logic error");

    *breakout = 0;
    parg->lvl = lvl;

    int r = 0;
    switch (hvml_dom_type(dom)) {
        case MKDOT(D_ROOT):
        {
            A(parg->rooted==0, "internal logic error");
            parg->rooted = 1;
        } break;
        case MKDOT(D_TAG):
        {
            switch (tag_open_close) {
                case 1: {
                    r = hvml_stream_printf(parg->stream, "<%s", hvml_dom_tag_name(dom));
                } break;
                case 2: {
                    r = hvml_stream_printf(parg->stream, "/>");
                } break;
                case 3: {
                    r = hvml_stream_printf(parg->stream, ">");
                } break;
                case 4: {
                    r = hvml_stream_printf(parg->stream, "</%s>", hvml_dom_tag_name(dom));
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        case MKDOT(D_ATTR):
        {
            A(parg->lvl >= parg->rooted ? 1 : 0, "internal logic error");
            const char *key = hvml_dom_attr_key(dom);
            const char *val = hvml_dom_attr_val(dom);
            r = hvml_stream_printf(parg->stream, " ");
            if (r<0) break;
            r = hvml_stream_printf(parg->stream, "%s", key);
            if (r<0) break;
            if (val) {
                r = hvml_stream_printf(parg->stream, "=\"");
                if (r<0) break;
                r = hvml_dom_attr_val_serialize(val, strlen(val), parg->stream);
                if (r<0) break;
                r = hvml_stream_printf(parg->stream, "\"");
            }
        } break;
        case MKDOT(D_TEXT):
        {
            const char *text = hvml_dom_text(dom);
            r = hvml_dom_str_serialize(text, strlen(text), parg->stream);
        } break;
        case MKDOT(D_JSON):
        {
            r = hvml_jo_value_serialize(hvml_dom_jo(dom), parg->stream);
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }

    if (r<0) {
        parg->failed = 1;
        *breakout = 1;
    }
}

