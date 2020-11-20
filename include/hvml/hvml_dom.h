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
    MKDOT(D_ROOT),
    MKDOT(D_TAG),
    MKDOT(D_ATTR),
    MKDOT(D_TEXT),
    MKDOT(D_JSON)
} HVML_DOM_TYPE;

const char *hvml_dom_type_str(HVML_DOM_TYPE t);

typedef struct hvml_dom_s          hvml_dom_t;
typedef struct hvml_dom_gen_s      hvml_dom_gen_t;

hvml_dom_t* hvml_dom_create();
void        hvml_dom_destroy(hvml_dom_t *dom);

hvml_dom_t* hvml_dom_make_root(hvml_dom_t *dom);

hvml_dom_t* hvml_dom_append_attr(hvml_dom_t *dom, const char *key, size_t key_len, const char *val, size_t val_len);
hvml_dom_t* hvml_dom_set_val(hvml_dom_t *dom, const char *val, size_t val_len);
hvml_dom_t* hvml_dom_append_content(hvml_dom_t *dom, const char *txt, size_t len);
hvml_dom_t* hvml_dom_add_tag(hvml_dom_t *dom, const char *tag, size_t len);
hvml_dom_t* hvml_dom_append_json(hvml_dom_t *dom, hvml_jo_value_t *jo);

hvml_dom_t* hvml_dom_root(hvml_dom_t *dom);
hvml_dom_t* hvml_dom_doc(hvml_dom_t *dom);
hvml_dom_t* hvml_dom_parent(hvml_dom_t *dom);
hvml_dom_t* hvml_dom_next(hvml_dom_t *dom);
hvml_dom_t* hvml_dom_prev(hvml_dom_t *dom);
hvml_dom_t* hvml_dom_child(hvml_dom_t *dom);
hvml_dom_t* hvml_dom_attr_head(hvml_dom_t *dom);
hvml_dom_t* hvml_dom_attr_next(hvml_dom_t *attr);

void        hvml_dom_detach(hvml_dom_t *dom);

hvml_dom_t* hvml_dom_select(hvml_dom_t *dom, const char *selector);

void        hvml_dom_str_serialize(const char *str, size_t len, FILE *out);
void        hvml_dom_attr_val_serialize(const char *str, size_t len, FILE *out);
void        hvml_dom_attr_set_key(hvml_dom_t *dom, const char *key, size_t key_len);
void        hvml_dom_attr_set_val(hvml_dom_t *dom, const char *val, size_t val_len);
void        hvml_dom_set_text(hvml_dom_t *dom, const char *txt, size_t txt_len);

HVML_DOM_TYPE hvml_dom_type(hvml_dom_t *dom);
const char*   hvml_dom_tag_name(hvml_dom_t *dom);      // tag's name
const char*   hvml_dom_attr_key(hvml_dom_t *dom);      // attr's key
const char*   hvml_dom_attr_val(hvml_dom_t *dom);      // attr's val
const char*   hvml_dom_text(hvml_dom_t *dom);          // elementText
hvml_jo_value_t* hvml_dom_jo(hvml_dom_t *dom);         // elementJson


// tag_open_close: 1-open, 2-single-close, 3-half-close, 4-close
int         hvml_dom_traverse(hvml_dom_t *dom, void *arg, void (*traverse_cb)(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout));

hvml_dom_t* hvml_dom_clone(hvml_dom_t *dom);

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

