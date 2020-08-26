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

#include "hvml/hvml_jo.h"

#include "hvml/hvml_json_parser.h"
#include "hvml/hvml_list.h"
#include "hvml/hvml_log.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// for easy coding
#define OBJ_MEMBERS()                HLIST_MEMBERS(hvml_jo_object_kv_t, hvml_jo_object_t, kv)
#define OBJ_KV_MEMBERS()             HNODE_MEMBERS(hvml_jo_object_kv_t, hvml_jo_object_t, kv)
#define OBJ_NEXT(obj_kv)             (obj_kv)->MKM(hvml_jo_object_kv_t, hvml_jo_object_t, kv, next)
#define OBJ_PREV(obj_kv)             (obj_kv)->MKM(hvml_jo_object_kv_t, hvml_jo_object_t, kv, prev)
#define OBJ_OWNER(obj_kv)            (obj_kv)->MKM(hvml_jo_object_kv_t, hvml_jo_object_t, kv, owner)
#define OBJ_HEAD(obj)                (obj)->MKM(hvml_jo_object_kv_t, hvml_jo_object_t, kv, head)
#define OBJ_TAIL(obj)                (obj)->MKM(hvml_jo_object_kv_t, hvml_jo_object_t, kv, tail)
#define OBJ_COUNT(obj)               (obj)->MKM(hvml_jo_object_kv_t, hvml_jo_object_t, kv, count)
#define OBJ_IS_ORPHAN(obj_kv)        HNODE_IS_ORPHAN(hvml_jo_object_kv_t, hvml_jo_object_t, kv, obj_kv)
#define OBJ_IS_EMPTY(obj)            HLIST_IS_EMPTY(hvml_jo_object_kv_t, hvml_jo_object_t, kv, obj)
#define OBJ_APPEND(obj,obj_kv)       HLIST_APPEND(hvml_jo_object_kv_t, hvml_jo_object_t, kv, obj, obj_kv)
#define OBJ_REMOVE(obj_kv)           HLIST_REMOVE(hvml_jo_object_kv_t, hvml_jo_object_t, kv, obj_kv)

#define VAL_MEMBERS() \
    HLIST_MEMBERS(hvml_jo_value_t, hvml_jo_value_t, _val_); \
    HNODE_MEMBERS(hvml_jo_value_t, hvml_jo_value_t, _val_)
#define VAL_NEXT(v)          (v)->MKM(hvml_jo_value_t, hvml_jo_value_t, _val_, next)
#define VAL_PREV(v)          (v)->MKM(hvml_jo_value_t, hvml_jo_value_t, _val_, prev)
#define VAL_OWNER(v)         (v)->MKM(hvml_jo_value_t, hvml_jo_value_t, _val_, owner)
#define VAL_HEAD(v)          (v)->MKM(hvml_jo_value_t, hvml_jo_value_t, _val_, head)
#define VAL_TAIL(v)          (v)->MKM(hvml_jo_value_t, hvml_jo_value_t, _val_, tail)
#define VAL_COUNT(v)         (v)->MKM(hvml_jo_value_t, hvml_jo_value_t, _val_, count)
#define VAL_IS_ORPHAN(v)     HNODE_IS_ORPHAN(hvml_jo_value_t, hvml_jo_value_t, _val_, v)
#define VAL_IS_EMPTY(v)      HLIST_IS_EMPTY(hvml_jo_value_t, hvml_jo_value_t, _val_, v)
#define VAL_APPEND(ov,v)     HLIST_APPEND(hvml_jo_value_t, hvml_jo_value_t, _val_, ov, v)
#define VAL_REMOVE(v)        HLIST_REMOVE(hvml_jo_value_t, hvml_jo_value_t, _val_, v)

struct hvml_jo_gen_s {
    hvml_jo_value_t          *jo;
    size_t                    idx;
    hvml_json_parser_t       *parser;
};

typedef struct hvml_jo_true_s         hvml_jo_true_t;
typedef struct hvml_jo_false_s        hvml_jo_false_t;
typedef struct hvml_jo_null_s         hvml_jo_null_t;
typedef struct hvml_jo_number_s       hvml_jo_number_t;
typedef struct hvml_jo_string_s       hvml_jo_string_t;
typedef struct hvml_jo_object_s       hvml_jo_object_t;
typedef struct hvml_jo_array_s        hvml_jo_array_t;

// internal
typedef struct hvml_jo_object_kv_s    hvml_jo_object_kv_t;
static hvml_jo_value_t* hvml_jo_object_kv(const char *key);

struct hvml_jo_true_s { };

struct hvml_jo_false_s { };

struct hvml_jo_null_s { };

struct hvml_jo_number_s {
    unsigned int   integer:1;
    union {
        int64_t    v_i;
        double     v_d;
    };
};

struct hvml_jo_string_s {
    char *str;
};

struct hvml_jo_object_s {
    OBJ_MEMBERS();
};

struct hvml_jo_array_s {
};

struct hvml_jo_object_kv_s {
    char                    *key;
    hvml_jo_value_t         *val;

    OBJ_KV_MEMBERS();
};

struct hvml_jo_value_s {
    HVML_JO_TYPE            jot;

    union {
        hvml_jo_string_t    jstr;
        hvml_jo_number_t    jnum;
        hvml_jo_true_t      jtrue;
        hvml_jo_false_t     jfalse;
        hvml_jo_null_t      jnull;
        hvml_jo_object_t    jobject;
        hvml_jo_array_t     jarray;

        hvml_jo_object_kv_t jkv;
    };

    VAL_MEMBERS();
};


#define hvml_jo_value_from_union(ptr) \
    ptr ? (hvml_jo_value_t*)(((char*)ptr)-offsetof(hvml_jo_value_t, jstr)) : NULL

hvml_jo_value_t* hvml_jo_undefined() {
    static hvml_jo_value_t undefined = { MKJOT(J_UNDEFINED) };
    return &undefined;
}

hvml_jo_value_t* hvml_jo_true() {
    hvml_jo_value_t *jo = (hvml_jo_value_t*)calloc(1, sizeof(*jo));
    if (!jo) return NULL;

    jo->jot = MKJOT(J_TRUE);

    return jo;
}

hvml_jo_value_t* hvml_jo_false() {
    hvml_jo_value_t *jo = (hvml_jo_value_t*)calloc(1, sizeof(*jo));
    if (!jo) return NULL;

    jo->jot = MKJOT(J_FALSE);

    return jo;
}

hvml_jo_value_t* hvml_jo_null() {
    hvml_jo_value_t *jo = (hvml_jo_value_t*)calloc(1, sizeof(*jo));
    if (!jo) return NULL;

    jo->jot = MKJOT(J_NULL);

    return jo;
}

hvml_jo_value_t* hvml_jo_integer(const int64_t v) {
    hvml_jo_value_t *jo = (hvml_jo_value_t*)calloc(1, sizeof(*jo));
    if (!jo) return NULL;

    jo->jot           = MKJOT(J_NUMBER);
    jo->jnum.integer  = 1;
    jo->jnum.v_i      = v;

    return jo;
}

hvml_jo_value_t* hvml_jo_double(const double v) {
    hvml_jo_value_t *jo = (hvml_jo_value_t*)calloc(1, sizeof(*jo));
    if (!jo) return NULL;

    jo->jot           = MKJOT(J_NUMBER);
    jo->jnum.integer  = 0;
    jo->jnum.v_d      = v;

    return jo;
}

hvml_jo_value_t* hvml_jo_string(const char *v) {
    hvml_jo_value_t *jo = (hvml_jo_value_t*)calloc(1, sizeof(*jo));
    if (!jo) return NULL;

    jo->jot = MKJOT(J_STRING);
    jo->jstr.str = strdup(v);
    if (!jo->jstr.str) {
        free(jo);
        return NULL;
    }

    return jo;
}

hvml_jo_value_t* hvml_jo_object() {
    hvml_jo_value_t *jo = (hvml_jo_value_t*)calloc(1, sizeof(*jo));
    if (!jo) return NULL;

    jo->jot = MKJOT(J_OBJECT);

    return jo;
}

hvml_jo_value_t* hvml_jo_array() {
    hvml_jo_value_t *jo = (hvml_jo_value_t*)calloc(1, sizeof(*jo));
    if (!jo) return NULL;

    jo->jot = MKJOT(J_ARRAY);

    return jo;
}

static hvml_jo_value_t* hvml_jo_object_kv(const char *key) {
    hvml_jo_value_t *jo = (hvml_jo_value_t*)calloc(1, sizeof(*jo));
    if (!jo) return NULL;

    jo->jot = MKJOT(J_OBJECT_KV);
    jo->jkv.key = strdup(key);
    if (!jo->jkv.key) {
        free(jo);
        return NULL;
    }

    return jo;
}

hvml_jo_value_t* hvml_jo_object_get_kv_by_key(hvml_jo_value_t *jo, const char *key) {
    A(jo->jot == MKJOT(J_OBJECT), "internal logic error");

    hvml_jo_object_kv_t *kv = OBJ_HEAD((&jo->jobject));
    while (kv) {
        if (strcmp(kv->key, key)==0) break;
        kv = OBJ_NEXT(kv);
    }

    if (!kv) return NULL;

    hvml_jo_value_t *v = hvml_jo_value_from_union(kv);
    A(v->jot == MKJOT(J_OBJECT_KV), "internal logic error");
    return v;
}

int hvml_jo_object_set_kv(hvml_jo_value_t *jo, const char *key, hvml_jo_value_t *val) {
    A(jo->jot == MKJOT(J_OBJECT), "internal logic error");

    hvml_jo_value_t *njo = hvml_jo_object_get_kv_by_key(jo, key);
    if (!njo) {
        njo = hvml_jo_object_kv(key);
        if (!njo) return -1;
    }
    A(njo->jot == MKJOT(J_OBJECT_KV), "internal logic error");
    if (njo->jkv.val!=val) {
        if (njo->jkv.val) hvml_jo_value_free(njo->jkv.val); 
        njo->jkv.val = val;
    }
    return 0;
}

int hvml_jo_array_append(hvml_jo_value_t *jo, hvml_jo_value_t *val) {
    if (jo->jot != MKJOT(J_ARRAY)) {
        E("jo[%p/%s] is NOT array", jo, hvml_jo_value_type_str(jo));
        return -1;
    }
    if (!VAL_IS_ORPHAN(val)) {
        E("val[%p/%s] is NOT orphan", val, hvml_jo_value_type_str(val));
        return -1;
    }
    if (jo==val) {
        E("can NOT append to itself");
        return -1;
    }

    size_t count = VAL_COUNT(jo);

    VAL_APPEND(jo, val);

    A(count+1 == VAL_COUNT(jo), "internal logic error");
    A(VAL_OWNER(val) == jo, "internal logic error");

    return 0;
}

void hvml_jo_value_detach(hvml_jo_value_t *jo) {
    hvml_jo_value_t *owner = VAL_OWNER(jo);
    if (!owner) return;

    A(!VAL_IS_ORPHAN(jo), "internal logic error");
    A(!VAL_IS_EMPTY(owner), "internal logic error");

    VAL_REMOVE(jo);

    A(VAL_IS_ORPHAN(jo), "internal logic error");
    A(VAL_OWNER(jo)==NULL, "internal logic error");

    if (owner->jot == MKJOT(J_OBJECT)) {
        A(jo->jot == MKJOT(J_OBJECT_KV), "internal logic error");

        hvml_jo_object_kv_t *kv = &jo->jkv;
        OBJ_REMOVE(kv);
    }
}

void hvml_jo_value_free(hvml_jo_value_t *jo) {
    if (jo->jot == MKJOT(J_UNDEFINED)) return;

    hvml_jo_value_detach(jo);

    switch (jo->jot) {
        case MKJOT(J_UNDEFINED):
        case MKJOT(J_TRUE):
        case MKJOT(J_FALSE):
        case MKJOT(J_NULL):
        case MKJOT(J_NUMBER): {
        } break;
        case MKJOT(J_STRING): {
            free(jo->jstr.str);
            jo->jstr.str = NULL;
        } break;
        case MKJOT(J_OBJECT): {
            while (OBJ_COUNT((&jo->jobject))>0) {
                size_t count = OBJ_COUNT((&jo->jobject));
                hvml_jo_object_kv_t *kv = OBJ_TAIL((&jo->jobject));
                A(kv, "internal logic error");
                hvml_jo_value_t *v = hvml_jo_value_from_union(kv);
                A(v, "internal logic error");

                hvml_jo_value_free(v);
                A(count - 1 == OBJ_COUNT((&jo->jobject)), "internal logic error");
            }
        } break;
        case MKJOT(J_ARRAY): {
            while (VAL_COUNT(jo)>0) {
                size_t count = VAL_COUNT(jo);
                hvml_jo_value_t *v = VAL_TAIL(jo);
                A(v, "internal logic error");

                hvml_jo_value_free(v);
                A(count - 1 == VAL_COUNT(jo), "internal logic error");
            }
        } break;
        case MKJOT(J_OBJECT_KV): {
            if (jo->jkv.key) {
                free(jo->jkv.key);
                jo->jkv.key = NULL;
            }
            if (jo->jkv.val) {
                hvml_jo_value_free(jo->jkv.val);
                jo->jkv.val = NULL;
            }
        } break;
        default: {
            A(0, "internal logic error, unknown JOT: [%d]", jo->jot);
            return;
        } break;
    }

    free(jo);
}

HVML_JO_TYPE hvml_jo_value_type(hvml_jo_value_t *jo) {
    return jo->jot;
}

const char* hvml_jo_value_type_str(hvml_jo_value_t *jo) {
    switch (jo->jot) {
        case MKJOT(J_UNDEFINED): { return MKJOS(J_UNDEFINED); break; }
        case MKJOT(J_TRUE):      { return MKJOS(J_TRUE);      break; }
        case MKJOT(J_FALSE):     { return MKJOS(J_FALSE);     break; }
        case MKJOT(J_NULL):      { return MKJOS(J_NULL);      break; }
        case MKJOT(J_NUMBER):    { return MKJOS(J_NUMBER);    break; }
        case MKJOT(J_STRING):    { return MKJOS(J_STRING);    break; }
        case MKJOT(J_OBJECT):    { return MKJOS(J_OBJECT);    break; }
        case MKJOT(J_ARRAY):     { return MKJOS(J_ARRAY);     break; }
        case MKJOT(J_OBJECT_KV): { return MKJOS(J_OBJECT_KV); break; }
        default: {
            A(0, "internal logic error, unknown JOT: [%d]", jo->jot);
            return ""; // never return
        } break;
    }
}

hvml_jo_value_t* hvml_jo_value_parent(hvml_jo_value_t *jo) {
    if (jo == NULL) return NULL;

    hvml_jo_value_t *owner = VAL_OWNER(jo);
    if (!owner) return NULL;

    if (owner->jot != MKJOT(J_OBJECT_KV)) return owner;

    return VAL_OWNER(owner);
}

hvml_jo_value_t* hvml_jo_value_root(hvml_jo_value_t *jo) {
    if (jo == NULL) return NULL;

    while (VAL_OWNER(jo)) {
        jo = hvml_jo_value_parent(jo);
    }

    return jo;
}

size_t hvml_jo_value_children(hvml_jo_value_t *jo) {
    return VAL_COUNT(jo);
}

#define fprintf_string(out, escaping, str)      \
do {                                            \
    if (!escaping) {                            \
        fprintf(out, "\"%s\"", str);            \
    } else {                                    \
        fprintf(out, "\"");                     \
        printf_escape(out, str);                \
        fprintf(out, "\"");                     \
    }                                           \
} while (0)

static void printf_escape(FILE *out, const char *s);

void hvml_jo_value_printf(hvml_jo_value_t *jo, int escaping, FILE *out) {
    switch (jo->jot) {
        case MKJOT(J_UNDEFINED): {
            fprintf(out, "undefined");
        } break;
        case MKJOT(J_TRUE): {
            fprintf(out, "true");
        } break;
        case MKJOT(J_FALSE): {
            fprintf(out, "false");
        } break;
        case MKJOT(J_NULL): {
            fprintf(out, "null");
        } break;
        case MKJOT(J_NUMBER): {
            if (jo->jnum.integer) {
                fprintf(out, "%"PRId64"", jo->jnum.v_i);
            } else {
                fprintf(out, "%g", jo->jnum.v_d);
            }
        } break;
        case MKJOT(J_STRING): {
            fprintf_string(out, escaping, jo->jstr.str);
        } break;
        case MKJOT(J_OBJECT): {
            fprintf(out, "{"); // "}"
            hvml_jo_object_kv_t *kv = OBJ_HEAD((&jo->jobject));
            while (kv) {
                hvml_jo_value_t *v = hvml_jo_value_from_union(kv);
                // attention: recursive call
                hvml_jo_value_printf(v, escaping, out);
                kv = OBJ_NEXT(kv);
                if (!kv) break;
                fprintf(out, ",");
            }
            // "{"
            fprintf(out, "}");
        } break;
        case MKJOT(J_OBJECT_KV): {
            A(jo->jkv.key, "internal logic error");
            fprintf_string(out, escaping, jo->jkv.key);
            if (jo->jkv.val) {
                fprintf(out, ":");
                // attention: recursive call
                hvml_jo_value_printf(jo->jkv.val, escaping, out);
            }
        } break;
        case MKJOT(J_ARRAY): {
            fprintf(out, "[");  // "]"
            hvml_jo_value_t *v = VAL_HEAD(jo);
            while (v) {
                // attention: recursive call
                hvml_jo_value_printf(v, escaping, out);
                v = VAL_NEXT(v);
                if (!v) break;
                fprintf(out, ",");
            }
            // "["
            fprintf(out, "]");
        } break;
        default: {
            A(0, "print json type [%d]: not implemented yet", jo->jot);
        } break;
    }
}

static int on_begin(void *arg);
static int on_open_array(void *arg);
static int on_close_array(void *arg);
static int on_open_obj(void *arg);
static int on_close_obj(void *arg);
static int on_key(void *arg, const char *key);
static int on_true(void *arg);
static int on_false(void *arg);
static int on_null(void *arg);
static int on_string(void *arg, const char *val);
static int on_integer(void *arg, const char *origin, int64_t val);
static int on_double(void *arg, const char *origin, double val);
static int on_end(void *arg);

static int gen_on_value(hvml_jo_gen_t *gen, hvml_jo_value_t *jo);

hvml_jo_gen_t* hvml_jo_gen_create() {
    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)calloc(1, sizeof(*gen));
    if (!gen) return NULL;

    gen->jo = hvml_jo_undefined();

    hvml_json_parser_conf_t conf = {0};
    conf.on_begin               = on_begin;
    conf.on_open_array          = on_open_array;
    conf.on_close_array         = on_close_array;
    conf.on_open_obj            = on_open_obj;
    conf.on_close_obj           = on_close_obj;
    conf.on_key                 = on_key;
    conf.on_true                = on_true;
    conf.on_false               = on_false;
    conf.on_null                = on_null;
    conf.on_string              = on_string;
    conf.on_integer             = on_integer;
    conf.on_double              = on_double;
    conf.on_end                 = on_end;

    conf.arg                    = gen;

    gen->parser = hvml_json_parser_create(conf);
    if (!gen->parser) {
        hvml_jo_gen_destroy(gen);
        return NULL;
    }

    return gen;
}

void hvml_jo_gen_destroy(hvml_jo_gen_t *gen) {
    do {
        if (!gen->jo) break;
        hvml_jo_value_t *root = hvml_jo_value_root(gen->jo);
        gen->jo = root;
        if (gen->jo->jot == MKJOT(J_UNDEFINED)) {
            gen->jo = NULL;
            break;
        }

        hvml_jo_value_free(gen->jo);
        gen->jo = NULL;
    } while (0);

    if (gen->parser) {
        hvml_json_parser_destroy(gen->parser);
        gen->parser = NULL;
    }

    free(gen);
}

int hvml_jo_gen_parse_char(hvml_jo_gen_t *gen, const char c) {
    return hvml_json_parser_parse_char(gen->parser, c);
}

int hvml_jo_gen_parse(hvml_jo_gen_t *gen, const char *buf, size_t len) {
    return hvml_json_parser_parse(gen->parser, buf, len);
}

int hvml_jo_gen_parse_string(hvml_jo_gen_t *gen, const char *str) {
    return hvml_json_parser_parse_string(gen->parser, str);
}

hvml_jo_value_t* hvml_jo_gen_parse_end(hvml_jo_gen_t *gen) {
    if (hvml_json_parser_parse_end(gen->parser)) {
        return NULL;
    }
    hvml_jo_value_t *jo   = gen->jo;
    gen->jo               = NULL;

    return jo;
}

hvml_jo_value_t* hvml_jo_value_load_from_stream(FILE *in) {
    hvml_jo_gen_t *gen = hvml_jo_gen_create();
    if (!gen) return NULL;

    char buf[4096] = {0};
    int  n         = 0;
    int  ret       = 0;

    while ( (n=fread(buf, 1, sizeof(buf), in))>0) {
        ret = hvml_jo_gen_parse(gen, buf, n);
        if (ret) break;
    }
    hvml_jo_value_t *jo = hvml_jo_gen_parse_end(gen);
    hvml_jo_gen_destroy(gen);

    if (ret==0) {
        return jo;
    }
    A(0, "internal logic error");
    hvml_jo_value_free(jo);
    return NULL;
}













static void printf_escape(FILE *out, const char *s) {
    while (*s) {
        const char c = *s;
        switch (c) {
            case '"':  { fprintf(out, "\\\"");  break; }
            case '\\': { fprintf(out, "\\\\");  break; }
            case '\b': { fprintf(out, "\\b");   break; }
            case '\t': { fprintf(out, "\\t");   break; }
            case '\f': { fprintf(out, "\\f");   break; }
            case '\r': { fprintf(out, "\\r");   break; }
            case '\n': { fprintf(out, "\\n");   break; }
            default:   { fprintf(out, "%c", c); break; }
        }
        ++s;
    }
}

static int on_begin(void *arg) {
	return 0;
}

static int on_open_array(void *arg) {
    hvml_jo_value_t *jo = hvml_jo_array();
    if (!jo) return -1;

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;
    A(gen, "internal logic error");

    if (gen->jo->jot == MKJOT(J_UNDEFINED)) {
        gen->jo = jo;
        return 0;
    }

    if (gen_on_value(gen, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    A(hvml_jo_value_parent(jo) == gen->jo, "internal logic error");
    gen->jo = jo;

	return 0;
}

static int on_close_array(void *arg) {
    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;
    A(gen->jo, "internal logic error");
    A(gen->jo->jot == MKJOT(J_ARRAY), "internal logic error");
    hvml_jo_value_t *parent = hvml_jo_value_parent(gen->jo);
    if (parent) {
        gen->jo = parent;
    }

	return 0;
}

static int on_open_obj(void *arg) {
    hvml_jo_value_t *jo = hvml_jo_object();
    if (!jo) return -1;

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;
    A(gen, "internal logic error");

    if (gen->jo->jot == MKJOT(J_UNDEFINED)) {
        gen->jo = jo;
        return 0;
    }

    if (gen_on_value(gen, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    A(hvml_jo_value_parent(jo) == gen->jo, "internal logic error");
    gen->jo = jo;

	return 0;
}

static int on_close_obj(void *arg) {
    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;
    A(gen->jo, "internal logic error");
    A(gen->jo->jot == MKJOT(J_OBJECT), "internal logic error");
    hvml_jo_value_t *parent = hvml_jo_value_parent(gen->jo);
    if (parent) {
        gen->jo = parent;
    }
	return 0;
}

static int on_key(void *arg, const char *key) {
    hvml_jo_gen_t   *gen    = (hvml_jo_gen_t*)arg;
    hvml_jo_value_t *parent = gen->jo;
    A(parent->jot == MKJOT(J_OBJECT), "internal logic error");

    hvml_jo_value_t *jo = hvml_jo_object_kv(key);
    if (!jo) return -1;
    A(jo->jot == MKJOT(J_OBJECT_KV), "internal logic error");

    if (gen_on_value(gen, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;
    A(jo->jot == MKJOT(J_OBJECT_KV), "internal logic error");

	return 0;
}

static int on_true(void *arg) {
    hvml_jo_value_t *jo = hvml_jo_true();
    if (!jo) return -1;

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    if (gen->jo->jot == MKJOT(J_UNDEFINED)) {
        gen->jo = jo;
        return 0;
    }

    if (gen_on_value(gen, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    A(hvml_jo_value_parent(jo) == gen->jo, "internal logic error");

	return 0;
}

static int on_false(void *arg) {
    hvml_jo_value_t *jo = hvml_jo_false();
    if (!jo) return -1;

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    if (gen->jo->jot == MKJOT(J_UNDEFINED)) {
        gen->jo = jo;
        return 0;
    }

    if (gen_on_value(gen, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    A(hvml_jo_value_parent(jo) == gen->jo, "internal logic error");

	return 0;
}

static int on_null(void *arg) {
    hvml_jo_value_t *jo = hvml_jo_null();
    if (!jo) return -1;

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    if (gen->jo->jot == MKJOT(J_UNDEFINED)) {
        gen->jo = jo;
        return 0;
    }

    if (gen_on_value(gen, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    A(hvml_jo_value_parent(jo) == gen->jo, "internal logic error");

	return 0;
}

static int on_string(void *arg, const char *val) {
    hvml_jo_value_t *jo = hvml_jo_string(val);
    if (!jo) return -1;

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    if (gen->jo->jot == MKJOT(J_UNDEFINED)) {
        gen->jo = jo;
        return 0;
    }

    if (gen_on_value(gen, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    A(hvml_jo_value_parent(jo) == gen->jo, "internal logic error");

	return 0;
}

static int on_integer(void *arg, const char *origin, int64_t val) {
    hvml_jo_value_t *jo = hvml_jo_integer(val);
    if (!jo) return -1;

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    if (gen->jo->jot == MKJOT(J_UNDEFINED)) {
        gen->jo = jo;
        return 0;
    }

    if (gen_on_value(gen, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    A(hvml_jo_value_parent(jo) == gen->jo, "internal logic error");

	return 0;
}

static int on_double(void *arg, const char *origin, double val) {
    hvml_jo_value_t *jo = hvml_jo_double(val);
    if (!jo) return -1;

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    if (gen->jo->jot == MKJOT(J_UNDEFINED)) {
        gen->jo = jo;
        return 0;
    }

    if (gen_on_value(gen, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    A(hvml_jo_value_parent(jo) == gen->jo, "internal logic error");

	return 0;
}

static int on_end(void *arg) {
	return 0;
}

static int gen_on_value(hvml_jo_gen_t *gen, hvml_jo_value_t *jo) {
    hvml_jo_value_t *parent = gen->jo;
    A(parent, "internal logic error");
    switch (hvml_jo_value_type(parent)) {
        case MKJOT(J_ARRAY): {
            VAL_APPEND(parent, jo);
        } break;
        case MKJOT(J_OBJECT): {
            A(hvml_jo_value_type(jo)==MKJOT(J_OBJECT_KV), "internal logic error");
            VAL_APPEND(parent, jo);
            OBJ_APPEND((&parent->jobject), (&jo->jkv));
        } break;
        case MKJOT(J_OBJECT_KV): {
            VAL_APPEND(parent, jo);
            A(parent->jkv.val == NULL, "internal logic error");
            parent->jkv.val = jo;
            hvml_jo_value_t *np = VAL_OWNER(parent);
            A(np, "internal logic error");
            A(hvml_jo_value_type(np)==MKJOT(J_OBJECT), "internal logic error");
            gen->jo = np;
        } break;
        default: {
            A(0, "internal logic error");
            return -1;
        } break;
    }

    return 0;
}

