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

#include "hvml/hvml_dom.h"

#include "hvml/hvml_jo.h"
#include "hvml/hvml_json_parser.h"
#include "hvml/hvml_list.h"
#include "hvml/hvml_parser.h"
#include "hvml/hvml_string.h"

#include <ctype.h>
#include <string.h>

// for easy coding
#define DOM_MEMBERS() \
    HLIST_MEMBERS(hvml_dom_t, hvml_dom_t, _dom_); \
    HNODE_MEMBERS(hvml_dom_t, hvml_dom_t, _dom_)
#define DOM_NEXT(v)          (v)->MKM(hvml_dom_t, hvml_dom_t, _dom_, next)
#define DOM_PREV(v)          (v)->MKM(hvml_dom_t, hvml_dom_t, _dom_, prev)
#define DOM_OWNER(v)         (v)->MKM(hvml_dom_t, hvml_dom_t, _dom_, owner)
#define DOM_HEAD(v)          (v)->MKM(hvml_dom_t, hvml_dom_t, _dom_, head)
#define DOM_TAIL(v)          (v)->MKM(hvml_dom_t, hvml_dom_t, _dom_, tail)
#define DOM_COUNT(v)         (v)->MKM(hvml_dom_t, hvml_dom_t, _dom_, count)
#define DOM_IS_ORPHAN(v)     HNODE_IS_ORPHAN(hvml_dom_t, hvml_dom_t, _dom_, v)
#define DOM_IS_EMPTY(v)      HLIST_IS_EMPTY(hvml_dom_t, hvml_dom_t, _dom_, v)
#define DOM_APPEND(ov,v)     HLIST_APPEND(hvml_dom_t, hvml_dom_t, _dom_, ov, v)
#define DOM_REMOVE(v)        HLIST_REMOVE(hvml_dom_t, hvml_dom_t, _dom_, v)

#define DOM_ATTR_MEMBERS() \
    HLIST_MEMBERS(hvml_dom_t, hvml_dom_t, _attr_); \
    HNODE_MEMBERS(hvml_dom_t, hvml_dom_t, _attr_)
#define DOM_ATTR_NEXT(v)          (v)->MKM(hvml_dom_t, hvml_dom_t, _attr_, next)
#define DOM_ATTR_PREV(v)          (v)->MKM(hvml_dom_t, hvml_dom_t, _attr_, prev)
#define DOM_ATTR_OWNER(v)         (v)->MKM(hvml_dom_t, hvml_dom_t, _attr_, owner)
#define DOM_ATTR_HEAD(v)          (v)->MKM(hvml_dom_t, hvml_dom_t, _attr_, head)
#define DOM_ATTR_TAIL(v)          (v)->MKM(hvml_dom_t, hvml_dom_t, _attr_, tail)
#define DOM_ATTR_COUNT(v)         (v)->MKM(hvml_dom_t, hvml_dom_t, _attr_, count)
#define DOM_ATTR_IS_ORPHAN(v)     HNODE_IS_ORPHAN(hvml_dom_t, hvml_dom_t, _attr_, v)
#define DOM_ATTR_IS_EMPTY(v)      HLIST_IS_EMPTY(hvml_dom_t, hvml_dom_t, _attr_, v)
#define DOM_ATTR_APPEND(ov,v)     HLIST_APPEND(hvml_dom_t, hvml_dom_t, _attr_, ov, v)
#define DOM_ATTR_REMOVE(v)        HLIST_REMOVE(hvml_dom_t, hvml_dom_t, _attr_, v)

typedef struct hvml_dom_tag_s               hvml_dom_tag_t;
typedef struct hvml_dom_attr_s              hvml_dom_attr_t;
typedef struct hvml_dom_text_s              hvml_dom_text_t;

struct hvml_dom_tag_s {
    hvml_string_t       name;
};

struct hvml_dom_attr_s {
    hvml_string_t       key;
    hvml_string_t       val;
};

struct hvml_dom_text_s {
    hvml_string_t       txt;
};

struct hvml_dom_s {
    HVML_DOM_TYPE       dt;

    union {
        hvml_dom_tag_t    tag;
        hvml_dom_attr_t   attr;
        hvml_dom_text_t   txt;
        hvml_jo_value_t  *jo;
    };

    DOM_ATTR_MEMBERS();
    DOM_MEMBERS();
};

struct hvml_dom_gen_s {
    hvml_dom_t          *dom;
    hvml_dom_t          *root;
    hvml_parser_t       *parser;
    hvml_jo_value_t     *jo;
};

hvml_dom_t* hvml_dom_create() {
    hvml_dom_t *dom = (hvml_dom_t*)calloc(1, sizeof(*dom));
    if (!dom) return NULL;

    return dom;
}

void hvml_dom_destroy(hvml_dom_t *dom) {
    hvml_dom_detach(dom);

    switch (dom->dt) {
        case MKDOT(D_TAG):
        {
            hvml_string_clear(&dom->tag.name);
        } break;
        case MKDOT(D_ATTR):
        {
            hvml_string_clear(&dom->attr.key);
            hvml_string_clear(&dom->attr.val);
        } break;
        case MKDOT(D_TEXT):
        {
            hvml_string_clear(&dom->txt.txt);
        } break;
        case MKDOT(D_JSON):
        {
            hvml_jo_value_free(dom->jo);
            dom->jo = NULL;
        } break;
        default:
        {
            A(0, "internal logic error");
        } break;
    }

    hvml_dom_t *d = DOM_HEAD(dom);
    while (d) {
        hvml_dom_destroy(d);
        d = DOM_HEAD(dom);
    }

    d = DOM_ATTR_HEAD(dom);
    while (d) {
        hvml_dom_destroy(d);
        d = DOM_ATTR_HEAD(dom);
    }

    free(dom);
}

hvml_dom_t* hvml_dom_append_attr(hvml_dom_t *dom, const char *key, size_t key_len, const char *val, size_t val_len) {
    switch (dom->dt) {
        case MKDOT(D_TAG):
        {
            hvml_dom_t *v      = hvml_dom_create();
            if (!v) return NULL;
            v->dt              = MKDOT(D_ATTR);
            do {
                int ret = hvml_string_set(&v->attr.key, key, key_len);
                if (ret) break;
                if (val) {
                    ret = hvml_string_set(&v->attr.val, val, val_len);
                    if (ret) break;
                }
                DOM_APPEND(dom, v);
                return v;
            } while (0);
            hvml_dom_destroy(v);
            return NULL;
        } break;
        case MKDOT(D_ATTR):
        {
            E("not allowed for attribute node");
            return NULL;
        } break;
        case MKDOT(D_TEXT):
        {
            E("not allowed for content node");
            return NULL;
        } break;
        default:
        {
            A(0, "internal logic error");
            return NULL;
        } break;
    }
}

hvml_dom_t* hvml_dom_set_val(hvml_dom_t *dom, const char *val, size_t val_len) {
    switch (dom->dt) {
        case MKDOT(D_TAG):
        {
            E("not allowed for tag node");
            return NULL;
        } break;
        case MKDOT(D_ATTR):
        {
            do {
                int ret = hvml_string_set(&dom->attr.val, val, val_len);
                if (ret) break;
                return dom;
            } while (0);
            return NULL;
        } break;
        case MKDOT(D_TEXT):
        {
            E("not allowed for content node");
            return NULL;
        } break;
        default:
        {
            A(0, "internal logic error");
            return NULL;
        } break;
    }
}

hvml_dom_t* hvml_dom_append_content(hvml_dom_t *dom, const char *txt, size_t len) {
    switch (dom->dt) {
        case MKDOT(D_TAG):
        {
            hvml_dom_t *v      = hvml_dom_create();
            if (!v) return NULL;
            v->dt              = MKDOT(D_TEXT);
            do {
                int ret = hvml_string_set(&v->txt.txt, txt, len);
                if (ret) break;
                DOM_APPEND(dom, v);
                return v;
            } while (0);
            hvml_dom_destroy(v);
            return NULL;
        } break;
        case MKDOT(D_ATTR):
        {
            E("not allowed for attribute node");
            return NULL;
        } break;
        case MKDOT(D_TEXT):
        {
            E("not allowed for content node");
            return NULL;
        } break;
        default:
        {
            A(0, "internal logic error");
            return NULL;
        } break;
    }
}

hvml_dom_t* hvml_dom_add_tag(hvml_dom_t *dom, const char *tag, size_t len) {
    switch (dom->dt) {
        case MKDOT(D_TAG):
        {
            hvml_dom_t *v      = hvml_dom_create();
            if (!v) return NULL;
            v->dt              = MKDOT(D_TAG);
            do {
                int ret = hvml_string_set(&v->tag.name, tag, len);
                if (ret) break;
                DOM_APPEND(dom, v);
                return v;
            } while (0);
            hvml_dom_destroy(v);
            return NULL;
        } break;
        case MKDOT(D_ATTR):
        {
            E("not allowed for attribute node");
            return NULL;
        } break;
        case MKDOT(D_TEXT):
        {
            E("not allowed for content node");
            return NULL;
        } break;
        default:
        {
            A(0, "internal logic error");
            return NULL;
        } break;
    }
}

hvml_dom_t* hvml_dom_append_json(hvml_dom_t *dom, hvml_jo_value_t *jo) {
    switch (dom->dt) {
        case MKDOT(D_TAG):
        {
            A(hvml_jo_value_parent(jo)==NULL, "internal logic error");
            hvml_dom_t *v      = hvml_dom_create();
            if (!v) return NULL;
            v->dt              = MKDOT(D_JSON);
            v->jo              = jo;
            DOM_APPEND(dom, v);
            return NULL;
        } break;
        case MKDOT(D_ATTR):
        {
            E("not allowed for attribute node");
            A(0, ".");
            return NULL;
        } break;
        case MKDOT(D_TEXT):
        {
            E("not allowed for content node");
            A(0, ".");
            return NULL;
        } break;
        default:
        {
            A(0, "internal logic error");
            return NULL;
        } break;
    }
}

hvml_dom_t* hvml_dom_root(hvml_dom_t *dom) {
    while (DOM_OWNER(dom)) {
        dom = DOM_OWNER(dom);
    }
    return dom;
}

hvml_dom_t* hvml_dom_parent(hvml_dom_t *dom) {
    return DOM_OWNER(dom);
}

hvml_dom_t* hvml_dom_next(hvml_dom_t *dom) {
    return DOM_NEXT(dom);
}

hvml_dom_t* hvml_dom_prev(hvml_dom_t *dom) {
    return DOM_PREV(dom);
}

void hvml_dom_detach(hvml_dom_t *dom) {
    if (DOM_OWNER(dom)) {
        DOM_REMOVE(dom);
    }
    if (DOM_ATTR_OWNER(dom)) {
        DOM_ATTR_REMOVE(dom);
    }
}

hvml_dom_t* hvml_dom_select(hvml_dom_t *dom, const char *selector) {
    A(0, "not implemented yet");
}

void hvml_dom_str_serialize(const char *str, size_t len, FILE *out) {
    const char *p = str;
    for (size_t i=0; i<len; ++i, ++p) {
        const char c = *p;
        switch (c) {
            case '&':  { fprintf(out, "&amp;");   break; }
            case '<':  {
                if (i<len-1 && !isspace(p[1])) {
                    fprintf(out, "&lt;");
                } else {
                    fprintf(out, "%c", c);
                }
            } break;
            default:   { fprintf(out, "%c", c);   break; }
        }
    }
}

void hvml_dom_attr_val_serialize(const char *str, size_t len, FILE *out) {
    const char *p = str;
    for (size_t i=0; i<len; ++i, ++p) {
        const char c = *p;
        switch (c) {
            case '&':  { fprintf(out, "&amp;");   break; }
            case '"':  { fprintf(out, "&quot;");  break; }
            default:   { fprintf(out, "%c", c);   break; }
        }
    }
}

HVML_DOM_TYPE hvml_dom_type(hvml_dom_t *dom) {
    return dom->dt;
}

const char* hvml_dom_tag_name(hvml_dom_t *dom) {
    if (dom->dt != MKDOT(D_TAG)) return NULL;
    return dom->tag.name.str;
}

const char* hvml_dom_attr_key(hvml_dom_t *dom) {
    if (dom->dt != MKDOT(D_ATTR)) return NULL;
    return dom->attr.key.str;
}

const char* hvml_dom_attr_val(hvml_dom_t *dom) {
    if (dom->dt != MKDOT(D_ATTR)) return NULL;
    return dom->attr.val.str;
}

const char* hvml_dom_text(hvml_dom_t *dom) {
    if (dom->dt != MKDOT(D_TEXT)) return NULL;
    return dom->txt.txt.str;
}

hvml_jo_value_t* hvml_dom_jo(hvml_dom_t *dom) {
    if (dom->dt != MKDOT(D_JSON)) return NULL;
    return dom->jo;
}


typedef struct traverse_s          traverse_t;
struct traverse_s {
    void      *arg;
    void     (*traverse_cb)(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout);
};

static int apply_traverse_callback(hvml_dom_t *dom, int lvl, int tag_open_close, traverse_t *tvs);
static int do_hvml_dom_traverse(hvml_dom_t *dom, traverse_t *tvs);

int hvml_dom_traverse(hvml_dom_t *dom, void *arg, void (*traverse_cb)(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout)) {
    traverse_t tvs;
    tvs.arg         = arg;
    tvs.traverse_cb = traverse_cb;
    return do_hvml_dom_traverse(dom, &tvs);
}

static int on_open_tag(void *arg, const char *tag);
static int on_attr_key(void *arg, const char *key);
static int on_attr_val(void *arg, const char *val);
static int on_close_tag(void *arg);
static int on_text(void *arg, const char *txt);

static int on_begin(void *arg);
static int on_open_array(void *arg);
static int on_close_array(void *arg);
static int on_open_obj(void *arg);
static int on_close_obj(void *arg);
static int on_key(void *arg, const char *key, size_t len);
static int on_true(void *arg);
static int on_false(void *arg);
static int on_null(void *arg);
static int on_string(void *arg, const char *val, size_t len);
static int on_integer(void *arg, const char *origin, int64_t val);
static int on_double(void *arg, const char *origin, double val);
static int on_end(void *arg);

hvml_dom_gen_t* hvml_dom_gen_create() {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)calloc(1, sizeof(*gen));
    if (!gen) return NULL;

    hvml_parser_conf_t conf = {0};

    conf.on_open_tag      = on_open_tag;
    conf.on_attr_key      = on_attr_key;
    conf.on_attr_val      = on_attr_val;
    conf.on_close_tag     = on_close_tag;
    conf.on_text          = on_text;

    conf.on_begin         = on_begin;
    conf.on_open_array    = on_open_array;
    conf.on_close_array   = on_close_array;
    conf.on_open_obj      = on_open_obj;
    conf.on_close_obj     = on_close_obj;
    conf.on_key           = on_key;
    conf.on_true          = on_true;
    conf.on_false         = on_false;
    conf.on_null          = on_null;
    conf.on_string        = on_string;
    conf.on_integer       = on_integer;
    conf.on_double        = on_double;
    conf.on_end           = on_end;

    conf.arg                    = gen;


    gen->parser = hvml_parser_create(conf);
    if (!gen->parser) {
        hvml_dom_gen_destroy(gen);
        return NULL;
    }

    return gen;
}

void hvml_dom_gen_destroy(hvml_dom_gen_t *gen) {
    if (gen->dom) {
        hvml_dom_t *root = hvml_dom_root(gen->dom);
        hvml_dom_destroy(root);
        gen->dom = NULL;
    }

    if (gen->parser) {
        hvml_parser_destroy(gen->parser);
        gen->parser = NULL;
    }

    if (gen->jo) {
        hvml_jo_value_free(gen->jo);
        gen->jo = NULL;
    }

    free(gen);
}

int hvml_dom_gen_parse_char(hvml_dom_gen_t *gen, const char c) {
    return hvml_parser_parse_char(gen->parser, c);
}

int hvml_dom_gen_parse(hvml_dom_gen_t *gen, const char *buf, size_t len) {
    return hvml_parser_parse(gen->parser, buf, len);
}

int hvml_dom_gen_parse_string(hvml_dom_gen_t *gen, const char *str) {
    return hvml_parser_parse_string(gen->parser, str);
}

hvml_dom_t* hvml_dom_gen_parse_end(hvml_dom_gen_t *gen) {
    if (hvml_parser_parse_end(gen->parser)) {
        return NULL;
    }
    hvml_dom_t *dom   = gen->dom;
    gen->dom          = NULL;

    return dom;
}

hvml_dom_t* hvml_dom_load_from_stream(FILE *in) {
    hvml_dom_gen_t *gen = hvml_dom_gen_create();
    if (!gen) return NULL;

    char buf[4096] = {0};
    int  n         = 0;
    int  ret       = 0;

    while ( (n=fread(buf, 1, sizeof(buf), in))>0) {
        ret = hvml_dom_gen_parse(gen, buf, n);
        if (ret) break;
    }
    hvml_dom_t *dom = hvml_dom_gen_parse_end(gen);
    hvml_dom_gen_destroy(gen);

    if (ret==0) {
        return dom;
    }
    if (dom) hvml_dom_destroy(dom);
    return NULL;
}

static void traverse_for_clone(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout);

hvml_dom_t* hvml_dom_clone(hvml_dom_t *dom) {
    hvml_dom_t* new_dom;
    hvml_dom_traverse(dom, &new_dom, traverse_for_clone);
    return new_dom;
}

static void traverse_for_clone(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout) {

    hvml_dom_t **p_cur_dom = (hvml_dom_t**)arg;
    A(p_cur_dom, "internal logic error");
    A(*p_cur_dom, "internal logic error");

    *breakout = 0;
    hvml_dom_t *cur_dom = NULL;

    switch (hvml_dom_type(dom)) {
        case MKDOT(D_TAG):
        {
            switch (tag_open_close) {
                case 1: {
                    hvml_dom_t *v = hvml_dom_create();
                    if (! v) {
                        A(0, "internal logic error");
                        return;
                    }
                    const char *tag = hvml_dom_tag_name(dom);
                    int ret = hvml_string_set(&v->tag.name, tag, strlen(tag));
                    if (ret) {
                        A(0, "internal logic error");
                        hvml_dom_destroy(v);
                        return;
                    }
                    *p_cur_dom = v;
                } break;

                case 2:
                case 4: {
                    cur_dom = hvml_dom_parent(cur_dom);
                    if (cur_dom) *p_cur_dom = cur_dom;
                } break;

                case 3: {
                } break;

                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;

        case MKDOT(D_ATTR):
        {
            hvml_dom_t *v = hvml_dom_create();
            if (! v) {
                A(0, "internal logic error");
                return;
            }
            const char *key = hvml_dom_attr_key(dom);
            const char *val = hvml_dom_attr_val(dom);
            v->dt = MKDOT(D_ATTR);
            int ret = hvml_string_set(&v->attr.key, key, strlen(key));
            if (ret) {
                A(0, "internal logic error");
                hvml_dom_destroy(v);
                return;
            }
            if (val) {
                int ret = hvml_string_set(&v->attr.val, val, strlen(val));
                if (ret) {
                    A(0, "internal logic error");
                    hvml_dom_destroy(v);
                    return;
                }
            }
            DOM_ATTR_APPEND(*p_cur_dom, v);
        } break;

        case MKDOT(D_TEXT):
        {
            hvml_dom_t *v = hvml_dom_create();
            if (! v) {
                A(0, "internal logic error");
                return;
            }
            const char *text = hvml_dom_text(dom);
            v->dt = MKDOT(D_TEXT);
            int ret = hvml_string_set(&v->txt.txt, text, strlen(text));
            if (ret) {
                A(0, "internal logic error");
                hvml_dom_destroy(v);
                return;
            }
            DOM_APPEND(*p_cur_dom, v);
        } break;

        case MKDOT(D_JSON):
        {
            hvml_dom_t *v = hvml_dom_create(dom);
            if (! v) {
                A(0, "internal logic error");
                return;
            }
            v->dt = MKDOT(D_JSON);
            v->jo = hvml_jo_clone(hvml_dom_jo(dom));
            if (! v->jo) {
                A(0, "internal logic error");
                hvml_dom_destroy(v);
                return;
            }
            DOM_APPEND(*p_cur_dom, v);
        } break;

        default: {
            A(0, "internal logic error");
        } break;
    }
}



static int on_open_tag(void *arg, const char *tag) {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;
    hvml_dom_t *v       = hvml_dom_create();
    if (!v) return -1;
    v->dt      = MKDOT(D_TAG);
    if (hvml_string_set(&v->tag.name, tag, strlen(tag))) {
        hvml_dom_destroy(v);
        return -1;
    }
    if (!gen->dom) {
        gen->dom = v;
        return 0;
    }
    if (gen->dom->dt == MKDOT(D_ATTR)) {
        DOM_APPEND(DOM_ATTR_OWNER(gen->dom), v);
    } else {
        A(gen->dom->dt == MKDOT(D_TAG), "internal logic error");
        DOM_APPEND(gen->dom, v);
    }
    gen->dom = v;
    return 0;
}

static int on_attr_key(void *arg, const char *key) {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;
    hvml_dom_t *v       = hvml_dom_create();
    if (!v) return -1;
    v->dt      = MKDOT(D_ATTR);
    if (hvml_string_set(&v->attr.key, key, strlen(key))) {
        hvml_dom_destroy(v);
        return -1;
    }
    A(gen->dom, "internal logic error");
    if (gen->dom->dt == MKDOT(D_ATTR)) {
        DOM_ATTR_APPEND(DOM_ATTR_OWNER(gen->dom), v);
    } else {
        A(gen->dom->dt == MKDOT(D_TAG), "internal logic error");
        DOM_ATTR_APPEND(gen->dom, v);
    }
    gen->dom = v;
    A(gen->dom->dt == MKDOT(D_ATTR), "internal logic error");
    return 0;
}

static int on_attr_val(void *arg, const char *val) {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;
    A(gen->dom, "internal logic error");
    A(gen->dom->dt == MKDOT(D_ATTR), "internal logic error");
    if (hvml_string_set(&gen->dom->attr.val, val, strlen(val))) {
        return -1;
    }
    gen->dom = DOM_ATTR_OWNER(gen->dom);
    A(gen->dom, "internal logic error");
    A(gen->dom->dt == MKDOT(D_TAG), "internal logic error");
    return 0;
}

static int on_close_tag(void *arg) {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;
    if (gen->dom->dt == MKDOT(D_ATTR)) {
        gen->dom = DOM_ATTR_OWNER(gen->dom);
        A(gen->dom->dt == MKDOT(D_TAG), "internal logic error");
    }
    A(gen->dom->dt == MKDOT(D_TAG), "internal logic error");
    if (DOM_OWNER(gen->dom)) {
        gen->dom = DOM_OWNER(gen->dom);
        A(gen->dom->dt == MKDOT(D_TAG), "internal logic error");
        return 0;
    }
    gen->root = gen->dom;
    return 0;
}

static int on_text(void *arg, const char *txt) {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;
    A(gen->root == NULL, "internal logic error");
    hvml_dom_t *v       = hvml_dom_create();
    if (!v) return -1;
    v->dt      = MKDOT(D_TEXT);
    if (hvml_string_set(&v->txt.txt, txt, strlen(txt))) {
        hvml_dom_destroy(v);
        return -1;
    }
    if (gen->dom->dt == MKDOT(D_ATTR)) {
        DOM_APPEND(DOM_ATTR_OWNER(gen->dom), v);
    } else {
        A(gen->dom->dt == MKDOT(D_TAG), "internal logic error");
        DOM_APPEND(gen->dom, v);
    }
    gen->dom = DOM_OWNER(v);
    return 0;
}

static int on_begin(void *arg) {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;
    return 0;
}

static int on_open_array(void *arg) {
    hvml_jo_value_t *jo = hvml_jo_array();
    if (!jo) return -1;

    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;
    A(gen, "internal logic error");

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    return 0;
}

static int on_close_array(void *arg) {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;

    hvml_jo_value_t *parent = hvml_jo_value_parent(gen->jo);
    if (!parent) return 0;

    gen->jo = parent;

    return 0;
}

static int on_open_obj(void *arg) {
    hvml_jo_value_t *jo = hvml_jo_object();
    if (!jo) return -1;

    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;
    A(gen, "internal logic error");

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    return 0;
}

static int on_close_obj(void *arg) {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;

    hvml_jo_value_t *parent = hvml_jo_value_parent(gen->jo);
    if (!parent) return 0;

    gen->jo = parent;

    return 0;
}

static int on_key(void *arg, const char *key, size_t len) {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;
    A(hvml_jo_value_type(gen->jo) == MKJOT(J_OBJECT), "internal logic error");

    hvml_jo_value_t *jo = hvml_jo_object_kv(key, len);
    if (!jo) return -1;

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    return 0;
}

static int on_true(void *arg) {
    hvml_jo_value_t *jo = hvml_jo_true();
    if (!jo) return -1;

    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    hvml_jo_value_t *parent = hvml_jo_value_parent(gen->jo);
    if (!parent) return 0;

    gen->jo = parent;

    return 0;
}

static int on_false(void *arg) {
    hvml_jo_value_t *jo = hvml_jo_false();
    if (!jo) return -1;

    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    hvml_jo_value_t *parent = hvml_jo_value_parent(gen->jo);
    if (!parent) return 0;

    gen->jo = parent;

    return 0;
}

static int on_null(void *arg) {
    hvml_jo_value_t *jo = hvml_jo_null();
    if (!jo) return -1;

    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    hvml_jo_value_t *parent = hvml_jo_value_parent(gen->jo);
    if (!parent) return 0;

    gen->jo = parent;

    return 0;
}

static int on_string(void *arg, const char *val, size_t len) {
    hvml_jo_value_t *jo = hvml_jo_string(val, len);
    if (!jo) return -1;

    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    hvml_jo_value_t *parent = hvml_jo_value_parent(gen->jo);
    if (!parent) return 0;

    gen->jo = parent;

    return 0;
}

static int on_integer(void *arg, const char *origin, int64_t val) {
    hvml_jo_value_t *jo = hvml_jo_integer(val, origin);
    if (!jo) return -1;

    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    hvml_jo_value_t *parent = hvml_jo_value_parent(gen->jo);
    if (!parent) return 0;

    gen->jo = parent;

    return 0;
}

static int on_double(void *arg, const char *origin, double val) {
    hvml_jo_value_t *jo = hvml_jo_double(val, origin);
    if (!jo) return -1;

    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    hvml_jo_value_t *parent = hvml_jo_value_parent(gen->jo);
    if (!parent) return 0;

    gen->jo = parent;

    return 0;
}

static int on_end(void *arg) {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;
    hvml_jo_value_t *val = hvml_jo_value_root(gen->jo);
    gen->jo = NULL;
    // hvml_jo_value_printf(val, stdout);
    // fprintf(stdout, "\n");
    // hvml_jo_value_free(val);
    hvml_dom_append_json(gen->dom, val);
    return 0;
}

static int do_hvml_dom_traverse(hvml_dom_t *dom, traverse_t *tvs) {
    A(tvs, "internal logic error");
    int lvl = 0;
    int r = 0;
    int pop = 0; // 1:pop from attr, 2:pop from child
    while (r==0) {
        switch (dom->dt) {
            case MKDOT(D_TAG):
            {
                if (pop==1) { // pop from attr
                    hvml_dom_t *child = DOM_HEAD(dom);
                    if (child) {
                        A(child->dt==MKDOT(D_TAG) || child->dt==MKDOT(D_TEXT) || child->dt==MKDOT(D_JSON), "");
                        // half close dom
                        r = apply_traverse_callback(dom, lvl, 3, tvs);
                        if (r) continue;

                        ++lvl;
                        dom = child;
                        pop = 0;
                        continue;
                    }
                    // single close dom
                    r = apply_traverse_callback(dom, lvl, 2, tvs);
                    if (r) continue;
                    pop = 2; // mock pop from child
                }
                if (pop==2) { // pop from child
                    hvml_dom_t *sibling = DOM_NEXT(dom);
                    if (sibling) {
                        dom = sibling;
                        A(dom->dt==MKDOT(D_TAG) || dom->dt==MKDOT(D_TEXT) || dom->dt==MKDOT(D_JSON), "");
                        pop = 0;
                        continue;
                    }
                    hvml_dom_t *parent = DOM_OWNER(dom);
                    if (parent) {
                        A(parent->dt == MKDOT(D_TAG), "internal logic error");
                        --lvl;
                        dom = parent;
                        // close dom
                        r = apply_traverse_callback(dom, lvl, 4, tvs);
                        if (r) continue;
                        pop = 2; // pop from child
                        continue;
                    }
                    A(lvl==0, "internal logic error:%d", lvl);
                    return 0; // short cut
                }

                A(pop==0, "internal logic error");
                    hvml_dom_t *child = DOM_HEAD(dom);

                r = apply_traverse_callback(dom, lvl, 1, tvs);
                if (r) continue;

                hvml_dom_t *attr = DOM_ATTR_HEAD(dom);
                if (attr) {
                    dom = attr;
                    A(dom->dt==MKDOT(D_ATTR), "");
                    continue;
                }

                pop = 1; // mock pop from attr

                continue;
            } break;
            case MKDOT(D_ATTR):
            {
                A(pop==0, "internal logic error");
                r = apply_traverse_callback(dom, lvl, 0, tvs);
                if (r) continue;

                hvml_dom_t *attr = DOM_ATTR_NEXT(dom);
                if (attr) {
                    dom = attr;
                    A(dom->dt==MKDOT(D_ATTR), "");
                    continue;
                }

                hvml_dom_t *parent = DOM_ATTR_OWNER(dom);
                A(parent, "internal logic error");
                A(parent->dt == MKDOT(D_TAG), "internal logic error");
                dom = parent;
                pop = 1; // pop from attr
                continue;
            } break;
            case MKDOT(D_TEXT):
            {
                A(pop==0, "internal logic error");
                r = apply_traverse_callback(dom, lvl+1, 0, tvs);
                if (r) continue;

                hvml_dom_t *sibling = DOM_NEXT(dom);
                if (sibling) {
                    dom = sibling;
                        A(dom->dt==MKDOT(D_TAG) || dom->dt==MKDOT(D_TEXT) || dom->dt==MKDOT(D_JSON), "");
                    pop = 0;
                    continue;
                }
                hvml_dom_t *parent = DOM_OWNER(dom);
                A(parent, "internal logic error");
                A(parent->dt == MKDOT(D_TAG), "internal logic error");
                --lvl;
                dom = parent;
                // close dom
                r = apply_traverse_callback(dom, lvl, 4, tvs);
                if (r) continue;
                pop = 2; // pop from child
                continue;
            } break;
            case MKDOT(D_JSON):
            {
                A(pop==0, "internal logic error");
                r = apply_traverse_callback(dom, lvl+1, 0, tvs);

                hvml_dom_t *sibling = DOM_NEXT(dom);
                if (sibling) {
                    dom = sibling;
                        A(dom->dt==MKDOT(D_TAG) || dom->dt==MKDOT(D_TEXT) || dom->dt==MKDOT(D_JSON), "");
                    pop = 0;
                    continue;
                }
                hvml_dom_t *parent = DOM_OWNER(dom);
                A(parent, "internal logic error");
                A(parent->dt == MKDOT(D_TAG), "internal logic error");
                --lvl;
                dom = parent;
                // close dom
                r = apply_traverse_callback(dom, lvl, 4, tvs);
                if (r) continue;
                pop = 2; // pop from child
                continue;
            } break;
            default:
            {
                A(0, "internal logic error");
            } break;
        }
        if (r) break;
    }
    return r ? -1 : 0;
}

static int apply_traverse_callback(hvml_dom_t *dom, int lvl, int tag_open_close, traverse_t *tvs) {
    A(dom, "internal logic error");
    A(lvl>=0, "internal logic error");
    A(tvs, "internal logic error");
    if (!tvs->traverse_cb) return 0;

    int breakout = 0;
    tvs->traverse_cb(dom, lvl, tag_open_close, tvs->arg, &breakout);
    return breakout ? -1 : 0;
}

