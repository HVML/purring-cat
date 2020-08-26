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

#ifndef _hvml_jo_h_
#define _hvml_jo_h_

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MKJOT(type)  HVML_JO_##type
#define MKJOS(type) "HVML_JO_"#type

typedef enum {
    MKJOT(J_UNDEFINED),
    MKJOT(J_TRUE),
    MKJOT(J_FALSE),
    MKJOT(J_NULL),
    MKJOT(J_NUMBER),
    MKJOT(J_STRING),
    MKJOT(J_OBJECT),
    MKJOT(J_ARRAY),
    MKJOT(J_OBJECT_KV)
} HVML_JO_TYPE;

typedef struct hvml_jo_value_s         hvml_jo_value_t;
typedef struct hvml_jo_gen_s           hvml_jo_gen_t;

hvml_jo_value_t* hvml_jo_undefined();
hvml_jo_value_t* hvml_jo_true();
hvml_jo_value_t* hvml_jo_false();
hvml_jo_value_t* hvml_jo_null();
hvml_jo_value_t* hvml_jo_integer(const int64_t v);
hvml_jo_value_t* hvml_jo_double(const double v);
hvml_jo_value_t* hvml_jo_string(const char *v);
hvml_jo_value_t* hvml_jo_object();
hvml_jo_value_t* hvml_jo_array();

int              hvml_jo_object_set_kv(hvml_jo_value_t *jo, const char *key, hvml_jo_value_t *val);
int              hvml_jo_array_append(hvml_jo_value_t *jo, hvml_jo_value_t *val);

void             hvml_jo_value_detach(hvml_jo_value_t *jo);
void             hvml_jo_value_free(hvml_jo_value_t *jo);

HVML_JO_TYPE     hvml_jo_value_type(hvml_jo_value_t *jo);
const char*      hvml_jo_value_type_str(hvml_jo_value_t *jo);

hvml_jo_value_t* hvml_jo_value_parent(hvml_jo_value_t *jo);
hvml_jo_value_t* hvml_jo_value_root(hvml_jo_value_t *jo);

size_t           hvml_jo_value_children(hvml_jo_value_t *jo);

const char*      hvml_jo_object_key(hvml_jo_value_t *jo, size_t idx);

void             hvml_jo_value_printf(hvml_jo_value_t *jo, int escaping, FILE *out);


hvml_jo_gen_t*   hvml_jo_gen_create();
void             hvml_jo_gen_destroy(hvml_jo_gen_t *gen);

int              hvml_jo_gen_parse_char(hvml_jo_gen_t *gen, const char c);
int              hvml_jo_gen_parse(hvml_jo_gen_t *gen, const char *buf, size_t len);
int              hvml_jo_gen_parse_string(hvml_jo_gen_t *gen, const char *str);
hvml_jo_value_t* hvml_jo_gen_parse_end(hvml_jo_gen_t *gen);

hvml_jo_value_t* hvml_jo_value_load_from_stream(FILE *in);

#ifdef __cplusplus
}
#endif

#endif // _hvml_jo_h_

