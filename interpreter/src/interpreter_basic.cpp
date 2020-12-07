// This file is a part of Purring Cat, a reference implementation of HVML.
//
// Copyright (C) 2020, <liuxinouc@126.com>.
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

#include "interpreter/interpreter_basic.h"
#include <string.h>

Interpreter_Basic::Interpreter_Basic(FILE *out)
{
    m_dom_prtt.lvl = -1;
    m_dom_prtt.out = out;
}

void Interpreter_Basic::GetOutput(hvml_dom_t *input_dom, FILE *out)
{
    Interpreter_Basic ipter(out);
    hvml_dom_traverse(input_dom, &ipter.m_dom_prtt, ipter.traverse_for_printf);
}

// tag_open_close: 1-open, 2-single-close, 3-half-close, 4-close
//int hvml_dom_traverse(hvml_dom_t *dom,
//                      void *arg,
//                      void (*traverse_cb)(hvml_dom_t *dom,
//                                          int lvl,
//                                          int tag_open_close,
//                                          void *arg,
//                                          int *breakout));

void Interpreter_Basic::traverse_for_printf(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout) {
    dom_printf_t *parg = (dom_printf_t*)arg;
    A(parg, "internal logic error");

    *breakout = 0;

    switch (hvml_dom_type(dom)) {
        case MKDOT(D_TAG):
        {
            switch (tag_open_close) {
                case 1: {
                    fprintf(parg->out, "<%s", hvml_dom_tag_name(dom));
                } break;
                case 2: {
                    fprintf(parg->out, "/>");
                } break;
                case 3: {
                    fprintf(parg->out, ">");
                } break;
                case 4: {
                    fprintf(parg->out, "</%s>", hvml_dom_tag_name(dom));
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
            parg->lvl = lvl;
        } break;
        case MKDOT(D_ATTR):
        {
            A(parg->lvl >= 0, "internal logic error");
            const char *key = hvml_dom_attr_key(dom);
            const char *val = hvml_dom_attr_val(dom);
            fprintf(parg->out, " ");
            fprintf(parg->out, "%s", key);
            if (val) {
                fprintf(parg->out, "=\"");
                hvml_dom_attr_val_serialize_file(val, strlen(val), parg->out);
                fprintf(parg->out, "\"");
            }
        } break;
        case MKDOT(D_TEXT):
        {
            const char *text = hvml_dom_text(dom);
            hvml_dom_str_serialize_file(text, strlen(text), parg->out);
            parg->lvl = lvl;
        } break;
        case MKDOT(D_JSON):
        {
            hvml_jo_value_printf(hvml_dom_jo(dom), parg->out);
            parg->lvl = lvl;
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }
}
