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

#ifndef _hvml_dom_h_
#define _hvml_dom_h_

#include "hvml/hvml_jo.h"

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MKDOT(type)  HVML_DOM_##type
#define MKDOS(type) "HVML_DOM_"#type

typedef enum {
    MKDOT(D_TAG),
    MKDOT(D_ATTR),
    MKDOT(D_TEXT),
    MKDOT(D_JSON)
} HVML_DOM_TYPE;

typedef struct hvml_dom_s          hvml_dom_t;
typedef struct hvml_dom_gen_s      hvml_dom_gen_t;

typedef struct traverse_callback_s {
    // all callback-funcs just mean as name implies

    // hvml output callbacks
    int  (*out_dom_head)       (FILE *out, char *dom_name);
    int  (*out_attr_separator) (FILE *out);
    int  (*out_simple_close)   (FILE *out);
    int  (*out_tag_close)      (FILE *out);
    int  (*out_dom_close)      (FILE *out, char *dom_name);
    int  (*out_attr_key)       (FILE *out, char *key_name);
    void (*out_attr_val)       (const char *str, size_t len, FILE *out);
    void (*out_dom_string)     (const char *str, size_t len, FILE *out);
    void (*out_json_value)     (hvml_jo_value_t *jo, FILE *out);

} traverse_callback;

hvml_dom_t* hvml_dom_create();
void        hvml_dom_destroy(hvml_dom_t *dom);

hvml_dom_t* hvml_dom_append_attr(hvml_dom_t *dom, const char *key, size_t key_len, const char *val, size_t val_len);
hvml_dom_t* hvml_dom_set_val(hvml_dom_t *dom, const char *val, size_t val_len);
hvml_dom_t* hvml_dom_append_content(hvml_dom_t *dom, const char *txt, size_t len);
hvml_dom_t* hvml_dom_add_tag(hvml_dom_t *dom, const char *tag, size_t len);
hvml_dom_t* hvml_dom_append_json(hvml_dom_t *dom, hvml_jo_value_t *jo);

hvml_dom_t* hvml_dom_root(hvml_dom_t *dom);
hvml_dom_t* hvml_dom_parent(hvml_dom_t *dom);
hvml_dom_t* hvml_dom_next(hvml_dom_t *dom);
hvml_dom_t* hvml_dom_prev(hvml_dom_t *dom);

void        hvml_dom_detach(hvml_dom_t *dom);

hvml_dom_t* hvml_dom_select(hvml_dom_t *dom, const char *selector);

void        hvml_dom_str_serialize(const char *str, size_t len, FILE *out);
void        hvml_dom_attr_val_serialize(const char *str, size_t len, FILE *out);

void        hvml_dom_printf(hvml_dom_t *dom, FILE *out);

void        hvml_dom_traverse(hvml_dom_t *dom, FILE *out, traverse_callback *out_funcs);

hvml_dom_gen_t*   hvml_dom_gen_create();
void              hvml_dom_gen_destroy(hvml_dom_gen_t *gen);

int               hvml_dom_gen_parse_char(hvml_dom_gen_t *gen, const char c);
int               hvml_dom_gen_parse(hvml_dom_gen_t *gen, const char *buf, size_t len);
int               hvml_dom_gen_parse_string(hvml_dom_gen_t *gen, const char *str);
hvml_dom_t*       hvml_dom_gen_parse_end(hvml_dom_gen_t *gen);

hvml_dom_t*       hvml_dom_load_from_stream(FILE *in);

#ifdef __cplusplus
}
#endif

#endif // _hvml_dom_h_

