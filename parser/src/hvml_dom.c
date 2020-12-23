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

#include "hvml_dom_xpath_parser.h"

#include "hvml/hvml_jo.h"
#include "hvml/hvml_json_parser.h"
#include "hvml/hvml_list.h"
#include "hvml/hvml_parser.h"
#include "hvml/hvml_printf.h"
#include "hvml/hvml_string.h"

#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
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

const char *hvml_dom_xpath_eval_type_str(HVML_DOM_XPATH_EVAL_TYPE t) {
    switch (t) {
        case HVML_DOM_XPATH_EVAL_UNKNOWN:    return "HVML_DOM_XPATH_EVAL_UNKNOWN";
        case HVML_DOM_XPATH_EVAL_BOOL:       return "HVML_DOM_XPATH_EVAL_BOOL";
        case HVML_DOM_XPATH_EVAL_NUMBER:     return "HVML_DOM_XPATH_EVAL_NUMBER";
        case HVML_DOM_XPATH_EVAL_STRING:     return "HVML_DOM_XPATH_EVAL_STRING";
        case HVML_DOM_XPATH_EVAL_DOMS:       return "HVML_DOM_XPATH_EVAL_DOMS";
        default: {
            A(0, "internal logic error");
            return ""; // never reached here
        } break;
    }
}

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
    } u;

    DOM_ATTR_MEMBERS();
    DOM_MEMBERS();
};

struct hvml_dom_gen_s {
    hvml_dom_t          *dom;
    hvml_parser_t       *parser;
    hvml_jo_value_t     *jo;
};

const char *hvml_dom_type_str(HVML_DOM_TYPE t) {
    switch (t) {
        case MKDOT(D_ROOT):               return "D_ROOT";
        case MKDOT(D_TAG):                return "D_TAG";
        case MKDOT(D_ATTR):               return "D_ATTR";
        case MKDOT(D_TEXT):               return "D_TEXT";
        case MKDOT(D_JSON):               return "D_JSON";
        default: return "D_UNKNOWN";
    }
}

const hvml_dom_xpath_eval_t      null_eval;

static void hvml_dom_xpath_eval_cleanup(hvml_dom_xpath_eval_t *ev) {
    if (!ev) return;
    switch (ev->et) {
        case HVML_DOM_XPATH_EVAL_UNKNOWN: break;
        case HVML_DOM_XPATH_EVAL_BOOL: {
            ev->u.b = 0;
        } break;
        case HVML_DOM_XPATH_EVAL_NUMBER: {
            ev->u.ldbl = 0;
        } break;
        case HVML_DOM_XPATH_EVAL_STRING: {
            free(ev->u.str);
            ev->u.str = NULL;
        } break;
        case HVML_DOM_XPATH_EVAL_DOMS: {
            hvml_doms_cleanup(&ev->u.doms);
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }
    ev->et = HVML_DOM_XPATH_EVAL_UNKNOWN;
}

const hvml_doms_t null_doms = {0};

int hvml_doms_append_dom(hvml_doms_t *doms, hvml_dom_t *dom) {
    if (!doms) return -1;
    if (!dom) return 0;

    for (size_t i=0; i<doms->ndoms; ++i) {
        if (dom == doms->doms[i]) return 0;
    }

    hvml_dom_t **e = (hvml_dom_t**)realloc(doms->doms, (doms->ndoms+1)*sizeof(*e));
    if (!e) return -1;
    e[doms->ndoms] = dom;
    doms->doms     = e;
    doms->ndoms   += 1;
    return 0;
}

int hvml_doms_append_doms(hvml_doms_t *doms, hvml_doms_t *in) {
    if (!doms) return -1;
    if (!in) return 0;
    if (in->ndoms==0) return 0;

    int r = 0;

    for (size_t i=0; i<in->ndoms; ++i) {
        r = hvml_doms_append_dom(doms, in->doms[i]);
        if (r) break;
    }

    return r;
}

void hvml_doms_cleanup(hvml_doms_t *doms) {
    if (!doms) return;

    free(doms->doms);
    doms->doms     = NULL;
    doms->ndoms    = 0;
}

void hvml_doms_destroy(hvml_doms_t *doms) {
    if (!doms) return;

    hvml_doms_cleanup(doms);
    free(doms);
}

hvml_dom_t* hvml_dom_create() {
    hvml_dom_t *dom = (hvml_dom_t*)calloc(1, sizeof(*dom));
    if (!dom) return NULL;

    return dom;
}

void hvml_dom_destroy(hvml_dom_t *dom) {
    hvml_dom_detach(dom);

    switch (dom->dt) {
        case MKDOT(D_ROOT):
        {
            hvml_dom_t *child = DOM_HEAD(dom);
            if (!child) break;
            A(hvml_dom_type(child)==MKDOT(D_TAG), "internal logic error");
            A(DOM_NEXT(child)==NULL, "internal logic error");
        } break;
        case MKDOT(D_TAG):
        {
            hvml_string_clear(&dom->u.tag.name);
        } break;
        case MKDOT(D_ATTR):
        {
            hvml_string_clear(&dom->u.attr.key);
            hvml_string_clear(&dom->u.attr.val);
        } break;
        case MKDOT(D_TEXT):
        {
            hvml_string_clear(&dom->u.txt.txt);
        } break;
        case MKDOT(D_JSON):
        {
            hvml_jo_value_free(dom->u.jo);
            dom->u.jo = NULL;
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

hvml_dom_t* hvml_dom_make_root(hvml_dom_t *dom) {
    if (!dom) return NULL;

    switch (dom->dt) {
        case MKDOT(D_ROOT): {
            A(DOM_IS_ORPHAN(dom), "internal logic error");
            return dom;
        } break;
        case MKDOT(D_TAG): {
            A(DOM_IS_ORPHAN(dom), "internal logic error");
            hvml_dom_t *root = hvml_dom_create();
            if (!root) return NULL;
            root->dt = MKDOT(D_ROOT);
            DOM_APPEND(root, dom);
            return root;
        } break;
        case MKDOT(D_ATTR): {
            A(0, "internal logic error");
        } break;
        case MKDOT(D_TEXT): {
            A(0, "internal logic error");
        } break;
        case MKDOT(D_JSON): {
            A(0, "internal logic error");
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }
}

hvml_dom_t* hvml_dom_append_attr(hvml_dom_t *dom, const char *key, size_t key_len, const char *val, size_t val_len) {
    A(!dom || dom->dt == MKDOT(D_TAG), "internal logic error");
    hvml_dom_t *v      = hvml_dom_create();
    if (!v) return NULL;
    v->dt              = MKDOT(D_ATTR);
    do {
        int ret = hvml_string_set(&v->u.attr.key, key, key_len);
        if (ret) break;
        if (val) {
            ret = hvml_string_set(&v->u.attr.val, val, val_len);
            if (ret) break;
        }
        if (dom) DOM_ATTR_APPEND(dom, v);
        return v;
    } while (0);
    hvml_dom_destroy(v);
    return NULL;
}

hvml_dom_t* hvml_dom_set_val(hvml_dom_t *dom, const char *val, size_t val_len) {
    A(dom && dom->dt == MKDOT(D_ATTR), "internal logic error");
    A(dom->dt != MKDOT(D_ROOT), "internal logic error");
    do {
        int ret = hvml_string_set(&dom->u.attr.val, val, val_len);
        if (ret) break;
        return dom;
    } while (0);
    return NULL;
}

hvml_dom_t* hvml_dom_append_content(hvml_dom_t *dom, const char *txt, size_t len) {
    A(!dom || dom->dt == MKDOT(D_TAG), "internal logic error");
    A(dom->dt != MKDOT(D_ROOT), "internal logic error");
    hvml_dom_t *v      = hvml_dom_create();
    if (!v) return NULL;
    v->dt              = MKDOT(D_TEXT);
    do {
        int ret = hvml_string_set(&v->u.txt.txt, txt, len);
        if (ret) break;
        if (dom) DOM_APPEND(dom, v);
        return v;
    } while (0);
    hvml_dom_destroy(v);
    return NULL;
}

hvml_dom_t* hvml_dom_add_tag(hvml_dom_t *dom, const char *tag, size_t len) {
    A(!dom || dom->dt == MKDOT(D_TAG) || (dom->dt == MKDOT(D_ROOT) && DOM_HEAD(dom)==NULL), "internal logic error");
    hvml_dom_t *v      = hvml_dom_create();
    if (!v) return NULL;
    v->dt              = MKDOT(D_TAG);
    do {
        int ret = hvml_string_set(&v->u.tag.name, tag, len);
        if (ret) break;
        if (dom) {
            if (dom->dt == MKDOT(D_ROOT)) {
                A(DOM_HEAD(dom)==NULL, "internal logic error");
            }
            DOM_APPEND(dom, v);
        }
        return v;
    } while (0);
    hvml_dom_destroy(v);
    return NULL;
}

hvml_dom_t* hvml_dom_append_json(hvml_dom_t *dom, hvml_jo_value_t *jo) {
    A(dom && dom->dt == MKDOT(D_TAG), "internal logic error");
    A(dom->dt != MKDOT(D_ROOT), "internal logic error");
    A(jo, "internal logic error");
    hvml_dom_t *v      = hvml_dom_create();
    if (!v) return NULL;
    v->dt              = MKDOT(D_JSON);
    if (hvml_jo_value_parent(jo)==NULL) {
        // jo is root, take owner ship
        v->u.jo        = jo;
        DOM_APPEND(dom, v);
        return v;
    }
    // jo is subvalue, clone it
    v->u.jo = hvml_jo_clone(jo);
    if (!v->u.jo) {
        hvml_dom_destroy(v);
        return NULL;
    }
    DOM_APPEND(dom, v);
    return v;
}

hvml_dom_t* hvml_dom_root(hvml_dom_t *dom) {
    while (dom) {
        hvml_dom_t *parent = NULL;
        switch (hvml_dom_type(dom)) {
            case MKDOT(D_ROOT): {
                A(DOM_OWNER(dom)==NULL, "internal logic error");
                return dom;
            } break;
            case MKDOT(D_TAG):
            case MKDOT(D_TEXT):
            case MKDOT(D_JSON): {
                parent = DOM_OWNER(dom);
            } break;
            case MKDOT(D_ATTR): {
                parent = DOM_ATTR_OWNER(dom);
            } break;
            default: {
                A(0, "internal logic error");
            } break;
        }
        if (!parent) break;
        dom = parent;
    }
    return dom;
}

hvml_dom_t* hvml_dom_doc(hvml_dom_t *dom) {
    while (dom) {
        hvml_dom_t *parent = NULL;
        switch (hvml_dom_type(dom)) {
            case MKDOT(D_ROOT): {
                hvml_dom_t *child = DOM_HEAD(dom);
                if (!child) return NULL;
                A(hvml_dom_type(child)==MKDOT(D_TAG), "internal logic error");
                return child;
            } break;
            case MKDOT(D_TAG):
            case MKDOT(D_TEXT):
            case MKDOT(D_JSON): {
                parent = DOM_OWNER(dom);
            } break;
            case MKDOT(D_ATTR): {
                parent = DOM_ATTR_OWNER(dom);
            } break;
            default: {
                A(0, "internal logic error");
            } break;
        }
        if (!parent) break;
        dom = parent;
    }
    return dom;
}

hvml_dom_t* hvml_dom_parent(hvml_dom_t *dom) {
    if (dom->dt==MKDOT(D_ATTR)) {
        dom = DOM_ATTR_OWNER(dom);
        A(dom->dt == MKDOT(D_TAG), "internal logic error");
    } else {
        dom = DOM_OWNER(dom);
    }
    return dom;
}

hvml_dom_t* hvml_dom_next(hvml_dom_t *dom) {
    return DOM_NEXT(dom);
}

hvml_dom_t* hvml_dom_prev(hvml_dom_t *dom) {
    return DOM_PREV(dom);
}

hvml_dom_t* hvml_dom_child(hvml_dom_t *dom) {
    return DOM_HEAD(dom);
}

hvml_dom_t* hvml_dom_attr_head(hvml_dom_t *dom) {
    return DOM_ATTR_HEAD(dom);
}

hvml_dom_t* hvml_dom_attr_next(hvml_dom_t *attr) {
    return DOM_ATTR_NEXT(attr);
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
    (void)dom;
    (void)selector;
}

void hvml_dom_str_serialize_file(const char *str, size_t len, FILE *out) {
    hvml_stream_t *stream = hvml_stream_bind_file(out, 0);
    if (!stream) return;
    hvml_dom_str_serialize(str, len, stream);
    hvml_stream_destroy(stream);
}

void hvml_dom_attr_val_serialize_file(const char *str, size_t len, FILE *out) {
    hvml_stream_t *stream = hvml_stream_bind_file(out, 0);
    if (!stream) return;
    hvml_dom_attr_val_serialize(str, len, stream);
    hvml_stream_destroy(stream);
}

int hvml_dom_str_serialize(const char *str, size_t len, hvml_stream_t *stream) {
    const char *p = str;
    int r = 0;
    for (size_t i=0; i<len; ++i, ++p) {
        const char c = *p;
        switch (c) {
            case '&': {
                r = hvml_stream_printf(stream, "&amp;");
            } break;
            case '<': {
                r = hvml_stream_printf(stream, "&lt;");
            } break;
            default: {
                r = hvml_stream_printf(stream, "%c", c);
            } break;
        }
        if (r<0) break;
    }
    return r<0 ? -1 : 0;
}

int hvml_dom_attr_val_serialize(const char *str, size_t len, hvml_stream_t *stream) {
    const char *p = str;
    int r = 0;
    for (size_t i=0; i<len; ++i, ++p) {
        const char c = *p;
        switch (c) {
            case '&': {
                r = hvml_stream_printf(stream, "&amp;");
            } break;
            case '<': {
                r = hvml_stream_printf(stream, "&lt;");
            } break;
            case '"': {
                r = hvml_stream_printf(stream, "&quot;");
            } break;
            default: {
                r = hvml_stream_printf(stream, "%c", c);
            } break;
        }
        if (r<0) break;
    }
    return r<0 ? -1 : 0;
}

void hvml_dom_attr_set_key(hvml_dom_t *dom, const char *key, size_t key_len) {
    A((dom->dt == MKDOT(D_ATTR)), "internal logic error");
    hvml_string_set(&dom->u.attr.key, key, key_len);
}

void hvml_dom_attr_set_val(hvml_dom_t *dom, const char *val, size_t val_len) {
    A((dom->dt == MKDOT(D_ATTR)), "internal logic error");
    hvml_string_set(&dom->u.attr.val, val, val_len);
}

void hvml_dom_set_text(hvml_dom_t *dom, const char *txt, size_t txt_len) {
    A((dom->dt == MKDOT(D_TEXT)), "internal logic error");
    hvml_string_set(&dom->u.txt.txt, txt, txt_len);
}

HVML_DOM_TYPE hvml_dom_type(hvml_dom_t *dom) {
    return dom->dt;
}

const char* hvml_dom_tag_name(hvml_dom_t *dom) {
    A(dom->dt == MKDOT(D_TAG), "internal logic error");
    return dom->u.tag.name.str;
}

const char* hvml_dom_attr_key(hvml_dom_t *dom) {
    A(dom->dt == MKDOT(D_ATTR), "internal logic error");
    return dom->u.attr.key.str;
}

const char* hvml_dom_attr_val(hvml_dom_t *dom) {
    A(dom->dt == MKDOT(D_ATTR), "internal logic error");
    return dom->u.attr.val.str;
}

const char* hvml_dom_text(hvml_dom_t *dom) {
    A(dom->dt == MKDOT(D_TEXT), "internal logic error");
    return dom->u.txt.txt.str;
}

hvml_jo_value_t* hvml_dom_jo(hvml_dom_t *dom) {
    A(dom->dt == MKDOT(D_JSON), "internal logic error");
    return dom->u.jo;
}

int hvml_dom_context_node_position(hvml_dom_context_node_t *node) {
    A(node, "internal logic error");
    A(node->doms && node->idx<node->doms->ndoms, "internal logic error");
    return node->idx;
}

typedef struct traverse_s          traverse_t;
struct traverse_s {
    void                  *arg;
    hvml_dom_traverse_cb   traverse_cb;
};

static int apply_traverse_callback(hvml_dom_t *dom, int lvl, int tag_open_close, traverse_t *tvs);
static int do_hvml_dom_traverse(hvml_dom_t *dom, traverse_t *tvs);

int hvml_dom_traverse(hvml_dom_t *dom, void *arg, hvml_dom_traverse_cb traverse_cb) {
    traverse_t tvs;
    tvs.arg         = arg;
    tvs.traverse_cb = traverse_cb;
    return do_hvml_dom_traverse(dom, &tvs);
}

typedef struct back_traverse_s          back_traverse_t;
struct back_traverse_s {
    void                       *arg;
    hvml_dom_back_traverse_cb   back_traverse_cb;
};

static int apply_back_traverse_callback(hvml_dom_t *dom, int lvl, back_traverse_t *tvs);
static int do_hvml_dom_back_traverse(hvml_dom_t *dom, back_traverse_t *tvs);

int hvml_dom_back_traverse(hvml_dom_t *dom, void *arg, hvml_dom_back_traverse_cb back_traverse_cb) {
    back_traverse_t tvs;
    tvs.arg         = arg;
    tvs.back_traverse_cb = back_traverse_cb;
    return do_hvml_dom_back_traverse(dom, &tvs);
}

typedef struct dom_clone_s            dom_clone_t;
struct dom_clone_s {
    hvml_dom_t        *dom;
};

static void traverse_for_clone(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout) {
    (void)lvl;
    dom_clone_t *dc = (dom_clone_t*)arg;

    HVML_DOM_TYPE dt = hvml_dom_type(dom);
    hvml_dom_t *v = NULL;
    *breakout = 1;
    switch (dt) {
        case MKDOT(D_ROOT): {
            A(dc->dom==NULL, "internal logic error");
            dc->dom = hvml_dom_create();
            if (!dc->dom) break;
            dc->dom->dt = MKDOT(D_ROOT);
            *breakout = 0;
        } break;
        case MKDOT(D_TAG): {
            switch (tag_open_close) {
                case 1: {
                    const char *s = hvml_dom_tag_name(dom);
                    A(s, "internal logic error");
                    v = hvml_dom_add_tag(dc->dom, s, strlen(s));
                    if (!v) break;
                    A(hvml_dom_type(v)==MKDOT(D_TAG), "internal logic error");
                    dc->dom = v;
                    *breakout = 0;
                } break;
                case 2: {
                    A(dc->dom, "internal logic error");
                    A(hvml_dom_type(dc->dom)==MKDOT(D_TAG), "internal logic error");
                    v = DOM_OWNER(dc->dom);
                    if (v) dc->dom = v;
                    *breakout = 0;
                } break;
                case 3: {
                    A(dc->dom, "internal logic error");
                    A(hvml_dom_type(dc->dom)==MKDOT(D_TAG), "internal logic error");
                    *breakout = 0;
                } break;
                case 4: {
                    A(dc->dom, "internal logic error");
                    A(hvml_dom_type(dc->dom)==MKDOT(D_TAG), "internal logic error");
                    v = DOM_OWNER(dc->dom);
                    if (v) dc->dom = v;
                    *breakout = 0;
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        case MKDOT(D_ATTR): {
            const char *key = hvml_dom_attr_key(dom);
            const char *val = hvml_dom_attr_val(dom);
            A(key, "internal logic error");
            A(hvml_dom_type(dc->dom)==MKDOT(D_TAG), "internal logic error");
            v = hvml_dom_append_attr(dc->dom, key, strlen(key), val, val ? strlen(val) : 0);
            if (!v) break;
            A(hvml_dom_type(v)==MKDOT(D_ATTR), "internal logic error");
            A(DOM_ATTR_OWNER(v)==dc->dom, "internal logic error");
            *breakout = 0;
        } break;
        case MKDOT(D_TEXT): {
            const char *text = hvml_dom_text(dom);
            A(text, "internal logic error");
            A(hvml_dom_type(dc->dom)==MKDOT(D_TAG), "internal logic error");
            v = hvml_dom_append_content(dc->dom, text, strlen(text));
            if (!v) break;
            A(hvml_dom_type(v)==MKDOT(D_TEXT), "internal logic error");
            A(DOM_OWNER(v)==dc->dom, "internal logic error");
            *breakout = 0;
        } break;
        case MKDOT(D_JSON): {
            A(dc->dom, "internal logic error");
            hvml_jo_value_t *jo = hvml_dom_jo(dom);
            A(jo, "internal logic error");
            jo = hvml_jo_clone(jo);
            if (!jo) break;
            A(hvml_dom_type(dc->dom)==MKDOT(D_TAG), "internal logic error");
            v = hvml_dom_append_json(dc->dom, jo);
            if (!v) {
                A(0, "internal logic error");
                hvml_jo_value_free(jo);
                break;
            }
            A(DOM_OWNER(v)==dc->dom, "internal logic error");
            *breakout = 0;
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }
}

hvml_dom_t* hvml_dom_clone(hvml_dom_t *dom) {
    dom_clone_t arg = {0};
    int r = hvml_dom_traverse(dom, &arg, traverse_for_clone);
    if (r) {
        if (arg.dom) {
            free(arg.dom);
            arg.dom = NULL;
        }
        return NULL;
    }
    if (!arg.dom) return NULL;

    if (dom->dt == MKDOT(D_ROOT)) {
        hvml_dom_t *root = hvml_dom_root(arg.dom);
        A(root, "internal logic error");
        A(root->dt == MKDOT(D_ROOT), "internal logic error");
        return root;
    }

    A(arg.dom->dt == dom->dt, "internal logic error");
    return arg.dom;
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
static int on_number(void *arg, const char *origin, long double val);
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
    conf.on_number        = on_number;
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
        hvml_dom_t *root = hvml_dom_doc(gen->dom);
        A(root, "internal logic error");
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

    A(gen->dom, "internal logic error");
    A(gen->dom->dt == MKDOT(D_ROOT), "internal logic error");

    hvml_dom_t *root   = gen->dom;
    gen->dom           = NULL;

    return root;
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

static int do_hvml_dom_check_node_test(hvml_dom_t *dom, HVML_DOM_XPATH_AXIS_TYPE axis, hvml_dom_xpath_node_test_t *node_test, hvml_dom_t **v);

typedef struct collect_relative_s          collect_relative_t;
struct collect_relative_s {
    hvml_doms_t                 *out;
    HVML_DOM_XPATH_AXIS_TYPE     axis;
    hvml_dom_xpath_node_test_t  *node_test;
    hvml_dom_t                  *relative;
    unsigned int                 forward:1;
    unsigned int                 self:1;
    unsigned int                 hit:1;
    unsigned int                 failed:1;
};

static void collect_relative_cb(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout) {
    (void)lvl;
    collect_relative_t *parg = (collect_relative_t*)arg;
    A(parg,               "internal logic error");
    A(parg->out,          "internal logic error");
    A(parg->relative,     "internal logic error");
    A(parg->failed==0,    "internal logic error");

    *breakout = 0;

    switch (hvml_dom_type(dom)) {
        case MKDOT(D_ROOT): break;
        case MKDOT(D_TAG):
        {
            switch (tag_open_close) {
                case 1: break;
                case 2: return;
                case 3: return;
                case 4: return;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        case MKDOT(D_ATTR): break;
        case MKDOT(D_TEXT): break;
        case MKDOT(D_JSON): break;
        default: {
            A(0, "internal logic error");
        } break;
    }

    if (dom==parg->relative) {
        A(!parg->hit, "internal logic error");
        parg->hit = 1;
        do {
            if (parg->self) {
                hvml_dom_t *v = NULL;
                if (parg->axis == HVML_DOM_XPATH_AXIS_ATTRIBUTE) break;
                parg->failed = do_hvml_dom_check_node_test(dom, parg->axis, parg->node_test, &v);
                if (parg->failed) break;
                if (!v) break;
                A(v==dom, "internal logic error");
                parg->failed = hvml_doms_append_dom(parg->out, v);
            }
        } while (0);
        if (parg->failed) {
            *breakout    = 1;
        }
        if (!parg->forward) {
            *breakout    = 1;
        }
        return;
    }

    do {
        if (parg->forward && !parg->hit) break;
        hvml_dom_t *v = NULL;
        parg->failed = do_hvml_dom_check_node_test(dom, parg->axis, parg->node_test, &v);
        if (parg->failed) break;
        if (!v) break;
        A(v==dom, "internal logic error");

        if (parg->axis == HVML_DOM_XPATH_AXIS_FOLLOWING ||
            parg->axis == HVML_DOM_XPATH_AXIS_PRECEDING)
        {
            if (parg->forward) {
                A(parg->hit, "internal logic error");
                hvml_dom_t *p = dom;
                while (p && p!=parg->relative) {
                    p = DOM_OWNER(p);
                }
                if (p==parg->relative) break;
            } else {
                A(parg->hit==0, "internal logic error");
                hvml_dom_t *p = parg->relative;
                while (p && p!=dom) {
                    p = DOM_OWNER(p);
                }
                if (p==dom) break;
            }
        }

        parg->failed = hvml_doms_append_dom(parg->out, v);
        if (parg->failed) break;
    } while (0);

    if (parg->failed) {
        *breakout = 1;
        parg->failed = 1;
    }
}

static int hvml_doms_append_relative(hvml_doms_t *out, HVML_DOM_XPATH_AXIS_TYPE axis, hvml_dom_xpath_node_test_t *node_test, hvml_dom_t *dom) {
    A(out,    "internal logic error");
    A(dom,    "internal logic error");
    collect_relative_t      collect = {0};
    collect.out       = out;
    collect.axis      = axis;
    collect.node_test = node_test;
    collect.relative  = dom;
    collect.forward   = 0;
    collect.self      = 0;
    collect.hit       = 0;
    collect.failed    = 0;

    switch (axis) {
        case HVML_DOM_XPATH_AXIS_UNSPECIFIED:
        case HVML_DOM_XPATH_AXIS_NAMESPACE:
        case HVML_DOM_XPATH_AXIS_SLASH:
        case HVML_DOM_XPATH_AXIS_SELF:
        case HVML_DOM_XPATH_AXIS_PARENT:
        case HVML_DOM_XPATH_AXIS_ATTRIBUTE:
        case HVML_DOM_XPATH_AXIS_ANCESTOR_OR_SELF:
        case HVML_DOM_XPATH_AXIS_ANCESTOR:
        case HVML_DOM_XPATH_AXIS_CHILD:
        case HVML_DOM_XPATH_AXIS_FOLLOWING_SIBLING:
        case HVML_DOM_XPATH_AXIS_PRECEDING_SIBLING: {
            A(0, "internal logic error");
        } break;
        case HVML_DOM_XPATH_AXIS_DESCENDANT_OR_SELF: {
            collect.forward = 1;
            collect.self    = 1;
        } break;
        case HVML_DOM_XPATH_AXIS_DESCENDANT: {
            collect.forward = 1;
        } break;
        case HVML_DOM_XPATH_AXIS_FOLLOWING: {
            collect.forward = 1;
            dom = hvml_dom_root(dom);
            A(dom, "internal logic error");
        } break;
        case HVML_DOM_XPATH_AXIS_PRECEDING: {
            dom = hvml_dom_root(dom);
            A(dom, "internal logic error");
        } break;
        default: {
            A(0, "internal logic error:%d", axis);
        } break;
    }
    hvml_dom_traverse(dom, &collect, collect_relative_cb);
    return collect.failed ? -1 : 0;
}

typedef struct doms_sort_s        doms_sort_t;
struct doms_sort_s {
    hvml_doms_t    *doms;
    hvml_doms_t    *in;
    size_t          count;
    int             failed;
};

static void traverse_for_doms_sort(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout) {
    doms_sort_t *parg = (doms_sort_t*)arg;
    A(parg, "internal logic error");
    A(lvl>=0, "internal logic error");

    *breakout = 0;

    int r = 0;

    switch (hvml_dom_type(dom)) {
        case MKDOT(D_ROOT): break;
        case MKDOT(D_TAG):
        {
            switch (tag_open_close) {
                case 1: break;
                case 2:
                case 3:
                case 4: {
                    dom = NULL;
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        case MKDOT(D_ATTR): break;
        case MKDOT(D_TEXT): break;
        case MKDOT(D_JSON): break;
        default: {
            A(0, "internal logic error");
        } break;
    }
    if (!dom) return;
    for (size_t i=0; i<parg->in->ndoms; ++i) {
        if (parg->in->doms[i] != dom) continue;
        r = hvml_doms_append_dom(parg->doms, dom);
        if (r) break;
        parg->in->doms[i] = NULL;
        parg->count -= 1;
        break;
    }
    if (r) {
        parg->failed = -1;
        *breakout = 1;
        return;
    }
    if (parg->count == 0) {
        *breakout = 1;
    }
}

int hvml_doms_reverse(hvml_doms_t *doms) {
    if (!doms) return 0;
    for (size_t i=0; i<doms->ndoms; ++i) {
        size_t j = doms->ndoms - 1 - i;
        if (i>j) break;
        hvml_dom_t *d    = doms->doms[i];
        doms->doms[i]    = doms->doms[j];
        doms->doms[j]    = d;
    }
    return 0;
}

int hvml_doms_sort(hvml_doms_t *doms, hvml_doms_t *in) {
    A(doms,             "internal logic error");
    A(doms->ndoms==0,   "internal logic error");
    if (!in) return 0;
    if (in->ndoms==0) return 0;
    A(in->doms[0], "internal logic error");

    doms_sort_t parg;
    parg.doms    = doms;
    parg.in      = in;
    parg.count   = in->ndoms;
    parg.failed  = 0;
    hvml_dom_traverse(hvml_dom_root(in->doms[0]), &parg, traverse_for_doms_sort);
    if (parg.failed) {
        hvml_doms_cleanup(doms);
        return -1;
    }
    A(parg.count == 0, "internal logic error");
    return 0;
}

static int do_hvml_dom_eval_location(hvml_dom_t *dom, hvml_dom_xpath_steps_t *steps, hvml_doms_t *out);
static int do_hvml_doms_eval_step(hvml_doms_t *doms, hvml_dom_xpath_step_t *step, hvml_doms_t *out);
static int do_hvml_dom_eval_step(hvml_dom_t *dom, hvml_dom_xpath_step_t *step, hvml_doms_t *out);
static int do_hvml_doms_eval_expr(hvml_doms_t *in, hvml_dom_xpath_expr_t *expr, hvml_doms_t *out);
static int do_hvml_dom_eval_expr(hvml_dom_context_node_t *node, hvml_dom_xpath_expr_t *expr, hvml_dom_xpath_eval_t *ev);
static int do_hvml_dom_eval_union_expr(hvml_dom_context_node_t *node, hvml_dom_xpath_union_expr_t *expr, hvml_dom_xpath_eval_t *ev);
static int do_hvml_dom_eval_path_expr(hvml_dom_context_node_t *node, hvml_dom_xpath_path_expr_t *expr, hvml_dom_xpath_eval_t *ev);
static int do_hvml_dom_eval_filter(hvml_dom_context_node_t *node, hvml_dom_xpath_filter_expr_t *filter, hvml_dom_xpath_eval_t *ev);
static int do_hvml_dom_eval_primary(hvml_dom_context_node_t *node, hvml_dom_xpath_primary_t *primary, hvml_dom_xpath_eval_t *ev);
static int do_hvml_dom_eval_func(hvml_dom_context_node_t *node, hvml_dom_xpath_func_t *func_call, hvml_dom_xpath_eval_t *ev);

static int hvml_dom_xpath_eval_or(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);
static int hvml_dom_xpath_eval_and(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);

static int hvml_dom_xpath_eval_compare(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, HVML_DOM_XPATH_OP_TYPE op, hvml_dom_xpath_eval_t *ev);
static int hvml_dom_xpath_eval_arith(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, HVML_DOM_XPATH_OP_TYPE op, hvml_dom_xpath_eval_t *ev);

static int do_hvml_dom_check_node_test(hvml_dom_t *dom, HVML_DOM_XPATH_AXIS_TYPE axis, hvml_dom_xpath_node_test_t *node_test, hvml_dom_t **v) {
    A(dom,          "internal logic error");
    A(node_test,    "internal logic error");
    A(v,            "internal logic error");
    A(node_test->is_cleanedup==0, "internal logic error");

    *v = NULL;

    int r = 0;
    if (!node_test->is_name_test) {
        switch (node_test->u.node_type) {
            case HVML_DOM_XPATH_NT_UNSPECIFIED: {
                A(0, "internal logic error");
            } break;
            case HVML_DOM_XPATH_NT_COMMENT: {
                break;
            } break;
            case HVML_DOM_XPATH_NT_TEXT: {
                if (dom->dt == MKDOT(D_TEXT)) *v = dom;
            } break;
            case HVML_DOM_XPATH_NT_PROCESSING_INSTRUCTION: {
                A(0, "not supported yet");
            } break;
            case HVML_DOM_XPATH_NT_NODE: {
                *v = dom;
            } break;
            case HVML_DOM_XPATH_NT_JSON: {
                if (dom->dt == MKDOT(D_JSON)) *v = dom;
            } break;
            default: {
                A(0, "internal logic error");
                // never reached here
                return -1;
            } break;
        }
        return r;
    }

    if (dom->dt!=MKDOT(D_TAG) && axis!=HVML_DOM_XPATH_AXIS_ATTRIBUTE) {
        return 0;
    }

    const char *prefix     = node_test->u.name_test.prefix;
    const char *local_part = node_test->u.name_test.local_part;
    A(local_part, "internal logic error");
    const char *tok = NULL;
    if (dom->dt == MKDOT(D_TAG)) {
        tok = hvml_dom_tag_name(dom);
    } else if (dom->dt == MKDOT(D_ATTR)) {
        tok = hvml_dom_attr_key(dom);
    }
    const char *colon = tok ? strchr(tok, ':') : NULL;

    if (strcmp(local_part, "*")==0) {
        if (prefix==NULL) {
            // "*"
            *v = dom;
            return 0;
        }
        // prefix:*
        if (!tok) return 0;
        if (strstr(tok, prefix)!=tok) return 0;
        if (tok[strlen(prefix)]!=':') return 0;
        *v = dom;
        return 0;
    }
    if (prefix && strcmp(prefix, "*")==0) {
        // "*:xxx"
        if (!tok) return 0;
        if (!colon) return 0;
        if (strcmp(colon+1, local_part)) return 0;
        *v = dom;
        return 0;
    }
    if (!prefix) {
        // "xxx"
        if (!tok) return 0;
        if (strcmp(tok, local_part)) return 0;
        *v = dom;
        return 0;
    }
    // "xxx:yyy"
    if (!tok) return 0;
    if (!colon) return 0;
    if (strstr(tok, prefix)!=tok) return 0;
    if (strcmp(colon+1, local_part)) return 0;
    *v = dom;
    return 0;
}

static int do_hvml_dom_eval_union_expr(hvml_dom_context_node_t *node, hvml_dom_xpath_union_expr_t *expr, hvml_dom_xpath_eval_t *ev) {
    A(node,         "internal logic error");
    hvml_dom_t *dom = node->dom;
    A(dom,          "internal logic error");
    A(expr,         "internal logic error");
    A(ev,           "internal logic error");
    A(ev->et==HVML_DOM_XPATH_EVAL_UNKNOWN, "internal logic error");
    A(expr->is_cleanedup==0, "internal logic error");
    A(expr->paths && expr->npaths>0,  "internal logic error");

    /*
    hvml_dom_xpath_path_expr_t    *paths;
    size_t                         npaths;
    int                            uminus;
    */

    if (expr->uminus==1) {
        A(expr->npaths==1, "internal logic error");
    }

    int r = 0;
    hvml_dom_xpath_eval_t e = {0};
    hvml_doms_t doms = {0};
    do {
        hvml_dom_xpath_path_expr_t *path_expr = expr->paths + 0;
        r = do_hvml_dom_eval_path_expr(node, path_expr, &e);
        if (r) break;
        if (expr->uminus) {
            A(expr->npaths==1, "internal logic error");
            A(e.et == HVML_DOM_XPATH_EVAL_NUMBER, "internal logic error");
            ev->et      = e.et;
            ev->u.ldbl  = -e.u.ldbl;
            break;
        }

        if (expr->npaths==1) {
            *ev = e;
            e   = null_eval;
            break;
        }

        A(e.et == HVML_DOM_XPATH_EVAL_DOMS, "internal logic error");
        r = hvml_doms_append_doms(&doms, &e.u.doms);
        if (r) break;
        hvml_dom_xpath_eval_cleanup(&e);

        for (size_t i=1; i<expr->npaths && r==0; ++i) {
            hvml_dom_xpath_path_expr_t *path_expr = expr->paths + i;
            r = do_hvml_dom_eval_path_expr(node, path_expr, &e);
            if (r) break;
            A(e.et == HVML_DOM_XPATH_EVAL_DOMS, "internal logic error");
            r = hvml_doms_append_doms(&doms, &e.u.doms);
            if (r) break;
            hvml_dom_xpath_eval_cleanup(&e);
        }

        if (r) break;
        hvml_dom_xpath_eval_cleanup(&e);
        ev->et       = HVML_DOM_XPATH_EVAL_DOMS;
        ev->u.doms   = doms;
        doms       = null_doms;
    } while (0);

    hvml_doms_cleanup(&doms);
    hvml_dom_xpath_eval_cleanup(&e);

    return r;
}

static int do_hvml_dom_eval_func(hvml_dom_context_node_t *node, hvml_dom_xpath_func_t *func_call, hvml_dom_xpath_eval_t *ev) {
    A(node,         "internal logic error");
    hvml_dom_t *dom = node->dom;
    A(dom,          "internal logic error");
    A(func_call,    "internal logic error");
    A(ev,           "internal logic error");
    A(ev->et==HVML_DOM_XPATH_EVAL_UNKNOWN, "internal logic error");
    A(func_call->is_cleanedup==0, "internal logic error");
    /*
    unsigned int is_cleanedup:1;
    HVML_DOM_XPATH_PREDEFINED_FUNC_TYPE  func;
    hvml_dom_xpath_exprs_t               args;
    */
    switch (func_call->func) {
        case HVML_DOM_XPATH_PREDEFINED_FUNC_UNSPECIFIED: {
            A(0, "internal logic error");
        } break;
        case HVML_DOM_XPATH_PREDEFINED_FUNC_POSITION: {
            A(func_call->args.nexprs==0, "internal logic error");
            int64_t position = hvml_dom_context_node_position(node);
            long double ldbl = (position+1);
            ev->et     = HVML_DOM_XPATH_EVAL_NUMBER;
            ev->u.ldbl = ldbl;
            return 0;
        } break;
        case HVML_DOM_XPATH_PREDEFINED_FUNC_LAST: {
            A(func_call->args.nexprs==0, "internal logic error");
            int64_t position = 0;
            position   = node->doms->ndoms - 1;
            long double ldbl = (position+1);
            ev->et     = HVML_DOM_XPATH_EVAL_NUMBER;
            ev->u.ldbl = ldbl;
            return 0;
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }
}

static int do_hvml_dom_eval_primary(hvml_dom_context_node_t *node, hvml_dom_xpath_primary_t *primary, hvml_dom_xpath_eval_t *ev) {
    A(node,         "internal logic error");
    hvml_dom_t *dom = node->dom;
    A(dom,          "internal logic error");
    A(primary,      "internal logic error");
    A(ev,           "internal logic error");
    A(ev->et==HVML_DOM_XPATH_EVAL_UNKNOWN, "internal logic error");
    A(primary->is_cleanedup==0, "internal logic error");
    /*
    HVML_DOM_XPATH_PRIMARY_TYPE primary_type;
    union {
        hvml_dom_xpath_qname_t  variable;
        hvml_dom_xpath_expr_t   expr;
        int64_t                 i32;
        long double             ldbl;
        char                   *literal;
        hvml_dom_xpath_func_t   func_call;
    } u;
    */
    int r = 0;

    do {
        switch (primary->primary_type) {
            case HVML_DOM_XPATH_PRIMARY_UNSPECIFIED: {
                A(0, "internal logic error");
            } break;
            case HVML_DOM_XPATH_PRIMARY_VARIABLE: {
                A(0, "not implemented yet");
            } break;
            case HVML_DOM_XPATH_PRIMARY_EXPR: {
                r = do_hvml_dom_eval_expr(node, &primary->u.expr, ev);
                return r;
            } break;
            case HVML_DOM_XPATH_PRIMARY_NUMBER: {
                ev->et     = HVML_DOM_XPATH_EVAL_NUMBER;
                ev->u.ldbl = primary->u.ldbl;
                return r;
            } break;
            case HVML_DOM_XPATH_PRIMARY_LITERAL: {
                ev->et    = HVML_DOM_XPATH_EVAL_STRING;
                ev->u.str = strdup(primary->u.literal ? primary->u.literal : "");
                if (!ev->u.str) r = -1;
                return r;
            } break;
            case HVML_DOM_XPATH_PRIMARY_FUNC: {
                r = do_hvml_dom_eval_func(node, &primary->u.func_call, ev);
                return r;
            } break;
            default: {
                A(0, "internal logic error");
            } break;
        }
    } while (0);

    return r;
}

static int do_hvml_dom_eval_filter(hvml_dom_context_node_t *node, hvml_dom_xpath_filter_expr_t *filter, hvml_dom_xpath_eval_t *ev) {
    A(node,         "internal logic error");
    hvml_dom_t *dom = node->dom;
    A(dom,          "internal logic error");
    A(filter,       "internal logic error");
    A(ev,           "internal logic error");
    A(ev->et==HVML_DOM_XPATH_EVAL_UNKNOWN, "internal logic error");
    A(filter->is_cleanedup==0, "internal logic error");

    /*
    hvml_dom_xpath_primary_t  primary;
    hvml_dom_xpath_exprs_t    exprs;
    */

    int r = 0;

    hvml_dom_xpath_eval_t e = {0};
    hvml_doms_t doms = {0};
    do {
        r = do_hvml_dom_eval_primary(node, &filter->primary, &e);
        if (r) break;

        if (filter->exprs.nexprs==0) {
            *ev = e;
            e = null_eval;
            break;
        }

        int ok = 0;
        switch (e.et) {
            case HVML_DOM_XPATH_EVAL_UNKNOWN: {
                A(0, "internal logic error");
            } break;
            case HVML_DOM_XPATH_EVAL_BOOL: {
                ok = e.u.b ? 1 : 0;
                if (!ok) break;
                r = hvml_doms_append_dom(&doms, node->dom);
            } break;
            case HVML_DOM_XPATH_EVAL_NUMBER: {
                int64_t position = hvml_dom_context_node_position(node);
                A(position>=0, "internal logic error");
                if (fabsl(e.u.ldbl-(position+1))<=DBL_EPSILON) {
                    ok = 1;
                }
                if (!ok) break;
                r = hvml_doms_append_dom(&doms, node->dom);
            } break;
            case HVML_DOM_XPATH_EVAL_STRING: {
                if (strlen(e.u.str)>0) {
                    ok = 1;
                }
                if (!ok) break;
                r = hvml_doms_append_dom(&doms, node->dom);
            } break;
            case HVML_DOM_XPATH_EVAL_DOMS: {
                if (e.u.doms.ndoms) {
                    ok = 1;
                }
                if (!ok) break;
                r = hvml_doms_append_doms(&doms, &e.u.doms);
            } break;
            default: {
                A(0, "internal logic error");
            } break;
        }

        if (r) break;

        if (!ok) {
            ev->et   = HVML_DOM_XPATH_EVAL_BOOL;
            ev->u.b  = 0;
            break;
        }

        A(doms.ndoms>0, "internal logic error");
        A(filter->exprs.nexprs>0, "internal logic error");
        A(ok, "internal logic error");

        hvml_doms_t tmp = {0};
        for (size_t i=0; i<filter->exprs.nexprs; ++i) {
            hvml_dom_xpath_expr_t *expr = filter->exprs.exprs + i;
            r = do_hvml_doms_eval_expr(&doms, expr, &tmp);
            if (r) break;
            hvml_doms_cleanup(&doms);
            doms  = tmp;
            tmp   = null_doms;
        }
        if (r==0) {
            ev->et        = HVML_DOM_XPATH_EVAL_BOOL;
            ev->u.b       = doms.ndoms ? 1 : 0;
        }
        hvml_doms_cleanup(&tmp);
    } while (0);

    hvml_doms_cleanup(&doms);
    hvml_dom_xpath_eval_cleanup(&e);

    return r;
}

static int do_hvml_dom_eval_path_expr(hvml_dom_context_node_t *node, hvml_dom_xpath_path_expr_t *expr, hvml_dom_xpath_eval_t *ev) {
    A(node,         "internal logic error");
    hvml_dom_t *dom = node->dom;
    A(dom,          "internal logic error");
    A(expr,         "internal logic error");
    A(ev,           "internal logic error");
    A(ev->et==HVML_DOM_XPATH_EVAL_UNKNOWN, "internal logic error");
    A(expr->is_cleanedup==0, "internal logic error");

    int r = 0;
    hvml_doms_t doms = {0};
    do {
        if (expr->is_location) {
            r = do_hvml_dom_eval_location(dom, &expr->location, &doms);
            if (r) break;
            ev->et      = HVML_DOM_XPATH_EVAL_DOMS;
            ev->u.doms  = doms;
            doms        = null_doms;
        } else {
            r = do_hvml_dom_eval_filter(node, &expr->filter_expr, ev);
            if (r) break;
        }
    } while (0);

    if (r) {
        hvml_dom_xpath_eval_cleanup(ev);
    }

    hvml_doms_cleanup(&doms);

    return r;
}

static int do_hvml_doms_eval_expr(hvml_doms_t *in, hvml_dom_xpath_expr_t *expr, hvml_doms_t *out) {
    A(in,           "internal logic error");
    A(expr,         "internal logic error");
    A(out,          "internal logic error");

    int r = 0;
    hvml_dom_xpath_eval_t ev = {0};
    for (size_t i=0; i<in->ndoms; ++i) {
        hvml_dom_context_node_t node = {0};
        node.doms = in;
        node.dom  = in->doms[i];
        node.idx  = i;
        r = do_hvml_dom_eval_expr(&node, expr, &ev);
        if (r) break;
        switch (ev.et) {
            case HVML_DOM_XPATH_EVAL_UNKNOWN: {
                A(0, "internal logic error");
            } break;
            case HVML_DOM_XPATH_EVAL_BOOL: {
                if (!ev.u.b) break;
                r = hvml_doms_append_dom(out, node.dom);
            } break;
            case HVML_DOM_XPATH_EVAL_NUMBER: {
                int64_t position = hvml_dom_context_node_position(&node);
                A(position>=0, "internal logic error");
                if (fabsl(ev.u.ldbl-(position+1))>DBL_EPSILON) break;
                r = hvml_doms_append_dom(out, node.dom);
            } break;
            case HVML_DOM_XPATH_EVAL_STRING: {
                A(0, "internal logic error");
                if (strlen(ev.u.str)==0) break;
                r = hvml_doms_append_dom(out, node.dom);
            } break;
            case HVML_DOM_XPATH_EVAL_DOMS: {
                if (ev.u.doms.ndoms==0) break;
                r = hvml_doms_append_dom(out, node.dom);
            } break;
            default: {
                A(0, "internal logic error");
            } break;
        }
        hvml_dom_xpath_eval_cleanup(&ev);
    }
    hvml_dom_xpath_eval_cleanup(&ev);
    return r;
}

static int do_hvml_dom_eval_expr(hvml_dom_context_node_t *node, hvml_dom_xpath_expr_t *expr, hvml_dom_xpath_eval_t *ev) {
    A(node,         "internal logic error");
    hvml_dom_t *dom = node->dom;
    A(dom,          "internal logic error");
    A(expr,         "internal logic error");
    A(ev,           "internal logic error");
    A(ev->et==HVML_DOM_XPATH_EVAL_UNKNOWN, "internal logic error");
    /*
    unsigned int is_binary_op:1;

    hvml_dom_xpath_union_expr_t    *unary;

    HVML_DOM_XPATH_OP_TYPE         op;
    hvml_dom_xpath_expr_t          *left;
    hvml_dom_xpath_expr_t          *right;
    */

    int r = 0;

    if (!expr->is_binary_op) {
        r = do_hvml_dom_eval_union_expr(node, expr->unary, ev);
    } else {
        A(expr->left, "internal logic error");
        A(expr->right, "internal logic error");

        hvml_dom_xpath_eval_t left  = {0};
        hvml_dom_xpath_eval_t right = {0};

        A(expr->left,  "internal logic error");
        A(expr->right, "internal logic error");
        do {
            // note: recursive
            r = do_hvml_dom_eval_expr(node, expr->left, &left);
            if (r) break;
            r = do_hvml_dom_eval_expr(node, expr->right, &right);
            if (r) break;
            switch (expr->op) {
                case HVML_DOM_XPATH_OP_UNSPECIFIED: {
                    A(0, "internal logic error");
                } break;
                case HVML_DOM_XPATH_OP_OR: {
                    r = hvml_dom_xpath_eval_or(&left, &right, ev);
                } break;
                case HVML_DOM_XPATH_OP_AND: {
                    r = hvml_dom_xpath_eval_and(&left, &right, ev);
                } break;
                case HVML_DOM_XPATH_OP_EQ:
                case HVML_DOM_XPATH_OP_NEQ: {
                    if (left.et == HVML_DOM_XPATH_EVAL_DOMS) {
                        r = hvml_dom_xpath_eval_compare(&left, &right, expr->op, ev);
                    } else {
                        r = hvml_dom_xpath_eval_compare(&right, &left, expr->op, ev);
                    }
                } break;
                case HVML_DOM_XPATH_OP_LT:
                case HVML_DOM_XPATH_OP_GT:
                case HVML_DOM_XPATH_OP_LTE:
                case HVML_DOM_XPATH_OP_GTE: {
                    if (left.et == HVML_DOM_XPATH_EVAL_DOMS) {
                        r = hvml_dom_xpath_eval_compare(&left, &right, expr->op, ev);
                    } else {
                        r = hvml_dom_xpath_eval_compare(&right, &left, -expr->op, ev);
                    }
                } break;
                case HVML_DOM_XPATH_OP_PLUS:
                case HVML_DOM_XPATH_OP_MINUS:
                case HVML_DOM_XPATH_OP_MULTI:
                case HVML_DOM_XPATH_OP_DIV:
                case HVML_DOM_XPATH_OP_MOD: {
                    r = hvml_dom_xpath_eval_arith(&left, &right, expr->op, ev);
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } while (0);

        hvml_dom_xpath_eval_cleanup(&left);
        hvml_dom_xpath_eval_cleanup(&right);

    }
    return r;
}

static int do_hvml_dom_eval_step(hvml_dom_t *dom, hvml_dom_xpath_step_t *step, hvml_doms_t *out) {
    A(dom,               "internal logic error");
    A(step,              "internal logic error");
    A(out,               "internal logic error");
    A(step->is_cleanedup==0, "internal logic error");
    A(step->node_test.is_cleanedup==0, "internal logic error");

    int r = 0;
    hvml_doms_t in = {0};

    switch (step->axis) {
        case HVML_DOM_XPATH_AXIS_UNSPECIFIED: {
            A(0, "internal logic error");
        } break;
        case HVML_DOM_XPATH_AXIS_NAMESPACE: {
            return 0;
        } break;
        case HVML_DOM_XPATH_AXIS_SLASH: {
            dom = hvml_dom_root(dom);
            A(dom, "internal logic error");
            A(dom->dt == MKDOT(D_ROOT), "internal logic error");
            r = hvml_doms_append_dom(&in, dom);
        } break;
        case HVML_DOM_XPATH_AXIS_SELF: {
            hvml_dom_t *v = NULL;
            r = do_hvml_dom_check_node_test(dom, step->axis, &step->node_test, &v);
            if (r) break;
            if (v) {
                A(v==dom, "internal logic error");
                r = hvml_doms_append_dom(&in, v);
                if (r) break;
            }
        } break;
        case HVML_DOM_XPATH_AXIS_PARENT: {
            if (dom->dt==MKDOT(D_ATTR)) {
                dom = DOM_ATTR_OWNER(dom);
                A(dom->dt == MKDOT(D_TAG), "internal logic error");
            } else {
                dom = DOM_OWNER(dom);
            }
            if (!dom) break;
            hvml_dom_t *v = NULL;
            r = do_hvml_dom_check_node_test(dom, step->axis, &step->node_test, &v);
            if (r) break;
            if (v) {
                A(v==dom, "internal logic error");
                r = hvml_doms_append_dom(&in, v);
                if (r) break;
            }
        } break;
        case HVML_DOM_XPATH_AXIS_ATTRIBUTE: {
            if (dom->dt!=MKDOT(D_TAG)) return 0;
            dom = DOM_ATTR_HEAD(dom);
            while (r==0 && dom) {
                hvml_dom_t *v = NULL;
                r = do_hvml_dom_check_node_test(dom, step->axis, &step->node_test, &v);
                if (r) break;
                if (v) {
                    A(v==dom, "internal logic error");
                    A(v->dt==MKDOT(D_ATTR), "internal logic error");
                    r = hvml_doms_append_dom(&in, v);
                    if (r) break;
                }
                dom = DOM_ATTR_NEXT(dom);
            }
        } break;
        case HVML_DOM_XPATH_AXIS_ANCESTOR_OR_SELF: {
            if (dom->dt!=MKDOT(D_ATTR)) {
                hvml_dom_t *v = NULL;
                r = do_hvml_dom_check_node_test(dom, step->axis, &step->node_test, &v);
                if (r) break;
                if (v) {
                    A(v==dom, "internal logic error");
                    r = hvml_doms_append_dom(&in, v);
                    if (r) break;
                }
            }
        } /* break; */ /* fall through */
        case HVML_DOM_XPATH_AXIS_ANCESTOR: {
            if (dom->dt == MKDOT(D_ATTR)) {
                dom = DOM_ATTR_OWNER(dom);
            } else {
                dom = DOM_OWNER(dom);
            }
            while (r==0 && dom) {
                hvml_dom_t *v = NULL;
                r = do_hvml_dom_check_node_test(dom, step->axis, &step->node_test, &v);
                if (r) break;
                if (v) {
                    A(v==dom, "internal logic error");
                    r = hvml_doms_append_dom(&in, v);
                    if (r) break;
                }
                dom = DOM_OWNER(dom);
            }
        } break;
        case HVML_DOM_XPATH_AXIS_CHILD: {
            if (dom->dt==MKDOT(D_ATTR)) return 0;
            dom = DOM_HEAD(dom);
            while (r==0 && dom) {
                A(dom->dt != MKDOT(D_ATTR), "internal logic error");
                hvml_dom_t *v = NULL;
                r = do_hvml_dom_check_node_test(dom, step->axis, &step->node_test, &v);
                if (r) break;
                if (v) {
                    A(v==dom, "internal logic error");
                    r = hvml_doms_append_dom(&in, v);
                    if (r) break;
                }
                dom = DOM_NEXT(dom);
            }
        } break;
        case HVML_DOM_XPATH_AXIS_FOLLOWING_SIBLING: {
            dom = DOM_NEXT(dom);
            while (r==0 && dom) {
                hvml_dom_t *v = NULL;
                r = do_hvml_dom_check_node_test(dom, step->axis, &step->node_test, &v);
                if (r) break;
                if (v) {
                    A(v==dom, "internal logic error");
                    r = hvml_doms_append_dom(&in, v);
                    if (r) break;
                }
                dom = DOM_NEXT(dom);
            }
        } break;
        case HVML_DOM_XPATH_AXIS_PRECEDING_SIBLING: {
            dom = DOM_PREV(dom);
            while (r==0 && dom) {
                hvml_dom_t *v = NULL;
                r = do_hvml_dom_check_node_test(dom, step->axis, &step->node_test, &v);
                if (r) break;
                if (v) {
                    A(v==dom, "internal logic error");
                    r = hvml_doms_append_dom(&in, v);
                    if (r) break;
                }
                dom = DOM_PREV(dom);
            }
        } break;
        case HVML_DOM_XPATH_AXIS_DESCENDANT_OR_SELF: {
            r = hvml_doms_append_relative(&in, step->axis, &step->node_test, dom);
        } break;
        case HVML_DOM_XPATH_AXIS_DESCENDANT: {
            r = hvml_doms_append_relative(&in, step->axis, &step->node_test, dom);
        } break;
        case HVML_DOM_XPATH_AXIS_FOLLOWING: {
            r = hvml_doms_append_relative(&in, step->axis, &step->node_test, dom);
        } break;
        case HVML_DOM_XPATH_AXIS_PRECEDING: {
            r = hvml_doms_append_relative(&in, step->axis, &step->node_test, dom);
            if (r) break;
            r = hvml_doms_reverse(&in);
        } break;
        default: {
            A(0, "internal logic error:%d", step->axis);
            r = -1;
        } break;
    }

    if (r) {
        hvml_doms_cleanup(&in);
        return r;
    }

    if (step->exprs.nexprs>0) {
        hvml_doms_t tmp = {0};
        for (size_t i=0; i<step->exprs.nexprs; ++i) {
            hvml_dom_xpath_expr_t *expr = step->exprs.exprs + i;
            r = do_hvml_doms_eval_expr(&in, expr, &tmp);
            if (r) break;
            hvml_doms_cleanup(&in);
            in    = tmp;
            tmp   = null_doms;
        }
        hvml_doms_cleanup(&tmp);
    }
    if (r==0) {
        if (out) {
            *out      = in;
            in        = null_doms;
        }
    }
    hvml_doms_cleanup(&in);
    return r;
}

static int do_hvml_doms_eval_step(hvml_doms_t *doms, hvml_dom_xpath_step_t *step, hvml_doms_t *out) {
    A(doms,              "internal logic error");
    A(step,              "internal logic error");
    A(out,               "internal logic error");

    int r = 0;
    hvml_doms_t o = null_doms;
    hvml_doms_t tmp = null_doms;

    for (size_t i=0; i<doms->ndoms; ++i) {
        hvml_dom_t *dom = doms->doms[i];

        r = do_hvml_dom_eval_step(dom, step, &tmp);
        if (r) break;

        r = hvml_doms_append_doms(&o, &tmp);
        if (r) break;

        hvml_doms_cleanup(&tmp);
    }

    hvml_doms_cleanup(&tmp);

    if (r==0) {
        if (out) {
            *out = o;
            o = null_doms;
        }
    }

    hvml_doms_cleanup(&o);

    return r;
}

static int do_hvml_dom_eval_location(hvml_dom_t *dom, hvml_dom_xpath_steps_t *steps, hvml_doms_t *out) {
    A(dom,               "internal logic error");
    A(steps,             "internal logic error");
    A(out,               "internal logic error");

    int r = 0;
    hvml_doms_t in = {0};

    do {
        r = hvml_doms_append_dom(&in, dom);
        if (r) break;

        for (size_t i=0; i<steps->nsteps; ++i) {
            hvml_doms_t tmp = {0};
            r = do_hvml_doms_eval_step(&in, steps->steps+i, &tmp);
            if (r==0) {
                hvml_doms_cleanup(&in);
                in  = tmp;
                tmp = null_doms;
            }
            hvml_doms_cleanup(&tmp);
            if (r) break;
        }
    } while (0);

    if (r==0) {
        if (out) {
            *out = in;
            in = null_doms;
        }
    }

    hvml_doms_cleanup(&in);

    return r;
}

static int hvml_dom_xpath_eval_to_bool(hvml_dom_xpath_eval_t *ev, int *v);
static int hvml_dom_xpath_eval_to_number(hvml_dom_xpath_eval_t *ev, long double *v);
static int hvml_dom_xpath_eval_to_string(hvml_dom_xpath_eval_t *ev, const char **v, int *allocated);


typedef struct collect_string_value_s          collect_string_value_t;
struct collect_string_value_s {
    hvml_dom_t         *dom;
    hvml_string_t       string_value;
    int                 failed;
};

static void collect_string_value_cb(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout) {
    (void)lvl;
    (void)tag_open_close;
    collect_string_value_t *parg = (collect_string_value_t*)arg;

    HVML_DOM_TYPE dt = hvml_dom_type(dom);
    *breakout = 0;

    switch (dt) {
        case MKDOT(D_ROOT):
        case MKDOT(D_TAG):
        case MKDOT(D_ATTR):
        case MKDOT(D_JSON): {
            // ignores
        } break;
        case MKDOT(D_TEXT): {
            const char *text = hvml_dom_text(dom);
            A(text, "internal logic error");
            int r = hvml_string_append(&parg->string_value, text);
            if (r==0) break;
            *breakout = 1;
            parg->failed = 1;
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }
}

int hvml_dom_string_for_xpath(hvml_dom_t *dom, const char **v, int *allocated) {
    A(dom,         "internal logic error");
    A(v,           "internal logic error");
    A(allocated,   "internal logic error");

    *v = NULL;
    *allocated = 0;

    switch (dom->dt) {
        case MKDOT(D_ROOT):
        case MKDOT(D_TAG): {
            collect_string_value_t      collect = {0};
            collect.dom    = dom;
            collect.failed = 0;
            hvml_dom_traverse(dom, &collect, collect_string_value_cb);
            if (collect.failed) {
                hvml_string_clear(&collect.string_value);
            } else {
                *v = hvml_string_str(&collect.string_value);
                if (*v) {
                    *allocated = 1;
                    hvml_string_reset(&collect.string_value);
                } else {
                    *v = "";
                }
            }
            return collect.failed ? -1 : 0;
        } break;
        case MKDOT(D_ATTR): {
            *v = hvml_dom_attr_val(dom);
            return 0;
        } break;
        case MKDOT(D_TEXT): {
            *v = hvml_dom_text(dom);
            return 0;
        } break;
        case MKDOT(D_JSON): {
            W("not implemented yet");    // what to compare? serialized form or not?
            *v = "";
            return 0;
        } break;
        default: {
            A(0, "internal logic error");
            // never reached here
            return -1;
        } break;
    }
}

static int hvml_dom_xpath_eval_to_bool(hvml_dom_xpath_eval_t *ev, int *v) {
    A(v, "internal logic error");
    *v = 0;
    switch (ev->et) {
        case HVML_DOM_XPATH_EVAL_UNKNOWN: {
            A(0, "internal logic error");
        } break;
        case HVML_DOM_XPATH_EVAL_BOOL: {
            if (ev->u.b) *v = 1;
        } break;
        case HVML_DOM_XPATH_EVAL_NUMBER: {
            int fc = fpclassify(ev->u.ldbl);
            if (fc!=FP_NAN && fc!=FP_ZERO) *v = 1;
        } break;
        case HVML_DOM_XPATH_EVAL_STRING: {
            if (strlen(ev->u.str)>0) *v = 1;
        } break;
        case HVML_DOM_XPATH_EVAL_DOMS: {
            if (ev->u.doms.ndoms>0) *v = 1;
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }
    return 0;
}

static int hvml_dom_xpath_eval_to_number(hvml_dom_xpath_eval_t *ev, long double *v) {
    A(v, "internal logic error");
    *v = 0;
    int r = 0;
    switch (ev->et) {
        case HVML_DOM_XPATH_EVAL_UNKNOWN: {
            A(0, "internal logic error");
        } break;
        case HVML_DOM_XPATH_EVAL_BOOL: {
            *v = ev->u.b ? 1 : 0;
        } break;
        case HVML_DOM_XPATH_EVAL_NUMBER: {
            *v = ev->u.ldbl;
        } break;
        case HVML_DOM_XPATH_EVAL_STRING: {
            // check error?
            A(ev->u.str, "internal logic error");
            *v = NAN;
            hvml_string_to_number(ev->u.str, v);
        } break;
        case HVML_DOM_XPATH_EVAL_DOMS: {
            const char *s = NULL;
            int allocated = 0;
            *v = NAN;
            r = hvml_dom_xpath_eval_to_string(ev, &s, &allocated);
            do {
                if (r) break;
                hvml_string_to_number(s, v);
            } while (0);
            if (allocated && s) {
                free((void*)s);
                break;
            }
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }
    return r;
}

static int hvml_dom_xpath_eval_to_string(hvml_dom_xpath_eval_t *ev, const char **v, int *allocated) {
    A(v,         "internal logic error");
    A(allocated, "internal logic error");
    *v = NULL;
    *allocated = 0;

    switch (ev->et) {
        case HVML_DOM_XPATH_EVAL_UNKNOWN: {
            A(0, "internal logic error");
        } break;
        case HVML_DOM_XPATH_EVAL_BOOL: {
            if (ev->u.b) *v = "true";
            else         *v = "false";
            return 0;
        } break;
        case HVML_DOM_XPATH_EVAL_NUMBER: {
            char buf[128];
            int n = snprintf(buf, sizeof(buf), "%.*Lf", (int)(sizeof(buf)/2), ev->u.ldbl);
            A(n>=0 && (size_t)n<sizeof(buf), "internal logic error");
            *v = strdup(buf);
            if (!*v) return -1; // out of memory
            *allocated = 1;
            return 0;
        } break;
        case HVML_DOM_XPATH_EVAL_STRING: {
            *v = ev->u.str;
            if (!*v) *v = "";
            return 0;
        } break;
        case HVML_DOM_XPATH_EVAL_DOMS: {
            if (ev->u.doms.ndoms==0) {
                *v = "";
                return 0;
            }
            A(ev->u.doms.doms, "internal logic error");
            hvml_dom_t *dom = ev->u.doms.doms[0];
            switch (dom->dt) {
                case MKDOT(D_ATTR): {
                    *v = hvml_dom_attr_val(dom);
                    if (!*v) *v = "";
                } break;
                case MKDOT(D_ROOT):
                case MKDOT(D_TAG): {
                    int r = hvml_dom_string_for_xpath(dom, v, allocated);
                    if (r) return -1;
                } break;
                case MKDOT(D_TEXT): {
                    *v = hvml_dom_text(dom);
                    if (!*v) *v = "";
                } break;
                case MKDOT(D_JSON): {
                } break;
            }
            return 0;
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }
    return 0;
}


static int hvml_dom_xpath_eval_or(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev) {
    A(left,  "internal logic error");
    A(right, "internal logic error");
    A(ev,    "internal logic error");
    A(ev->et==HVML_DOM_XPATH_EVAL_UNKNOWN, "internal logic error");

    int r = 0;
    int b = 0;

    ev->et = HVML_DOM_XPATH_EVAL_BOOL;

    r = hvml_dom_xpath_eval_to_bool(left, &b);
    if (r) return r;

    if (b) { ev->u.b = 1; return 0; }

    r = hvml_dom_xpath_eval_to_bool(right, &b);
    if (r) return r;

    ev->u.b = b;

    return 0;
}

static int hvml_dom_xpath_eval_and(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev) {
    A(left,  "internal logic error");
    A(right, "internal logic error");
    A(ev,    "internal logic error");
    A(ev->et==HVML_DOM_XPATH_EVAL_UNKNOWN, "internal logic error");

    int r = 0;
    int b = 0;

    ev->et = HVML_DOM_XPATH_EVAL_BOOL;

    r = hvml_dom_xpath_eval_to_bool(left, &b);
    if (r) return r;

    if (!b) { ev->u.b = 0; return 0; }

    r = hvml_dom_xpath_eval_to_bool(right, &b);
    if (r) return r;

    ev->u.b = b;

    return 0;
}

static int hvml_dom_xpath_to_bool(HVML_DOM_XPATH_OP_TYPE op, int delta) {
    switch(op) {
        case HVML_DOM_XPATH_OP_EQ: {
            return delta==0 ? 1 : 0;
        } break;
        case HVML_DOM_XPATH_OP_NEQ: {
            return delta ? 1 : 0;
        } break;
        case HVML_DOM_XPATH_OP_LT: {
            return delta<0 ? 1 : 0;
        } break;
        case HVML_DOM_XPATH_OP_GT: {
            return delta>0 ? 1 : 0;
        } break;
        case HVML_DOM_XPATH_OP_LTE: {
            return delta<=0 ? 1 : 0;
        } break;
        case HVML_DOM_XPATH_OP_GTE: {
            return delta>=0 ? 1 : 0;
        } break;
        default: {
            A(0, "internal logic error");
            return 0; // never reached here
        } break;
    }
}

static int hvml_dom_xpath_eval_compare(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, HVML_DOM_XPATH_OP_TYPE op, hvml_dom_xpath_eval_t *ev) {
    A(left,  "internal logic error");
    A(right, "internal logic error");
    A(ev,    "internal logic error");
    A(ev->et==HVML_DOM_XPATH_EVAL_UNKNOWN, "internal logic error");

    switch(op) {
        case HVML_DOM_XPATH_OP_EQ:
        case HVML_DOM_XPATH_OP_NEQ:
        case HVML_DOM_XPATH_OP_LT:
        case HVML_DOM_XPATH_OP_GT:
        case HVML_DOM_XPATH_OP_LTE:
        case HVML_DOM_XPATH_OP_GTE: {
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }

    ev->et   = HVML_DOM_XPATH_EVAL_BOOL;
    ev->u.b  = 0;

    HVML_DOM_XPATH_EVAL_TYPE et = HVML_DOM_XPATH_EVAL_UNKNOWN;
    if (left->et != HVML_DOM_XPATH_EVAL_DOMS) {
        A(right->et != HVML_DOM_XPATH_EVAL_DOMS, "internal logic error");
    }

    switch (left->et) {
        case HVML_DOM_XPATH_EVAL_UNKNOWN: {
            A(0, "internal logic error");
        } break;
        case HVML_DOM_XPATH_EVAL_BOOL: {
            switch (right->et) {
                case HVML_DOM_XPATH_EVAL_UNKNOWN: {
                    A(0, "internal logic error");
                } break;
                case HVML_DOM_XPATH_EVAL_BOOL:
                case HVML_DOM_XPATH_EVAL_NUMBER:
                case HVML_DOM_XPATH_EVAL_STRING: {
                    et = left->et;
                } break;
                case HVML_DOM_XPATH_EVAL_DOMS: {
                    et = right->et;
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        case HVML_DOM_XPATH_EVAL_NUMBER: {
            switch (right->et) {
                case HVML_DOM_XPATH_EVAL_UNKNOWN: {
                    A(0, "internal logic error");
                } break;
                case HVML_DOM_XPATH_EVAL_BOOL: {
                    et = right->et;
                } break;
                case HVML_DOM_XPATH_EVAL_NUMBER:
                case HVML_DOM_XPATH_EVAL_STRING: {
                    et = left->et;
                } break;
                case HVML_DOM_XPATH_EVAL_DOMS: {
                    et = right->et;
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        case HVML_DOM_XPATH_EVAL_STRING: {
            switch (right->et) {
                case HVML_DOM_XPATH_EVAL_UNKNOWN: {
                    A(0, "internal logic error");
                } break;
                case HVML_DOM_XPATH_EVAL_BOOL:
                case HVML_DOM_XPATH_EVAL_NUMBER:
                case HVML_DOM_XPATH_EVAL_STRING:
                case HVML_DOM_XPATH_EVAL_DOMS: {
                    et = right->et;
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        case HVML_DOM_XPATH_EVAL_DOMS: {
            switch (right->et) {
                case HVML_DOM_XPATH_EVAL_UNKNOWN: {
                    A(0, "internal logic error");
                } break;
                case HVML_DOM_XPATH_EVAL_BOOL:
                case HVML_DOM_XPATH_EVAL_NUMBER:
                case HVML_DOM_XPATH_EVAL_STRING:
                case HVML_DOM_XPATH_EVAL_DOMS: {
                    et = left->et;
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }
    int r = 0;
    int delta = 0;
    switch (et) {
        case HVML_DOM_XPATH_EVAL_UNKNOWN: {
            A(0, "internal logic error");
        } break;
        case HVML_DOM_XPATH_EVAL_BOOL: {
            int lv = 0, rv = 0;
            r = hvml_dom_xpath_eval_to_bool(left, &lv);
            if (r) break;
            r = hvml_dom_xpath_eval_to_bool(right, &rv);
            if (r) break;
            delta   = lv-rv;
            ev->u.b = hvml_dom_xpath_to_bool(op, delta);
        } break;
        case HVML_DOM_XPATH_EVAL_NUMBER: {
            long double lv = 0, rv = 0;
            r = hvml_dom_xpath_eval_to_number(left, &lv);
            if (r) break;
            r = hvml_dom_xpath_eval_to_number(right, &rv);
            if (r) break;
            int cl = fpclassify(lv);
            int cr = fpclassify(rv);
            if (cl==FP_NAN || cr==FP_NAN) {
                ev->u.b = 0;
                break;
            }
            if (cl==FP_ZERO && cr==FP_ZERO) {
                ev->u.b = hvml_dom_xpath_to_bool(op, 0);
                break;
            }
            long double delta = lv - rv;
            switch (fpclassify(delta)) {
                case FP_ZERO: {
                    ev->u.b = hvml_dom_xpath_to_bool(op, 0);
                } break;
                case FP_NAN: {
                    ev->u.b = 0;
                } break;
                case FP_INFINITE: {
                    ev->u.b = 0;
                } break;
                default: {
                    if (fabsl(delta)<=DBL_EPSILON) {
                        ev->u.b = hvml_dom_xpath_to_bool(op, 0);
                        break;
                    }
                    ev->u.b = hvml_dom_xpath_to_bool(op, signbit(delta) ? -1 : 1);
                } break;
            }
        } break;
        case HVML_DOM_XPATH_EVAL_STRING: {
            const char *lv = NULL, *rv = NULL;
            int llv = 0, rrv = 0;
            do {
                r = hvml_dom_xpath_eval_to_string(left, &lv, &llv);
                if (r) break;
                r = hvml_dom_xpath_eval_to_string(right, &rv, &rrv);
                if (r) break;
                delta   = strcmp(lv, rv);
                ev->u.b = hvml_dom_xpath_to_bool(op, delta);
            } while (0);
            if (lv && llv) free((void*)lv);
            if (rv && rrv) free((void*)rv);
        } break;
        case HVML_DOM_XPATH_EVAL_DOMS: {
            A(left->et == HVML_DOM_XPATH_EVAL_DOMS, "internl logic error");
            for (size_t i=0; i<left->u.doms.ndoms && !ev->u.b && r==0; ++i) {
                hvml_dom_t *ldom = left->u.doms.doms[i];
                A(ldom, "internal logic error");
                const char *lvs = NULL;
                int allocated = 0;
                switch (right->et) {
                    case HVML_DOM_XPATH_EVAL_UNKNOWN: {
                        A(0, "internal logic error");
                    } break;
                    case HVML_DOM_XPATH_EVAL_BOOL: {
                        int rv  = right->u.b ? 1 : 0;
                        delta   = 1-rv;
                        ev->u.b = hvml_dom_xpath_to_bool(op, delta);
                    } break;
                    case HVML_DOM_XPATH_EVAL_NUMBER: {
                        long double rv = right->u.ldbl;
                        if (!lvs) {
                            r = hvml_dom_string_for_xpath(ldom, &lvs, &allocated);
                            if (r) break;
                        }
                        long double lv = 0.;
                        hvml_string_to_number(lvs, &lv);
                        long double delta = lv - rv;
                        int sign = signbit(delta);
                        if (sign) {
                            delta = -1;
                        } else {
                            delta = (fabsl(delta)<=DBL_EPSILON) ? 0 : 1;
                        }
                        ev->u.b = hvml_dom_xpath_to_bool(op, delta);
                    } break;
                    case HVML_DOM_XPATH_EVAL_STRING: {
                        const char *rv = right->u.str;
                        if (!lvs) {
                            r = hvml_dom_string_for_xpath(ldom, &lvs, &allocated);
                            if (r) break;
                        }
                        delta   = strcmp(lvs, rv);
                        ev->u.b = hvml_dom_xpath_to_bool(op, delta);
                    } break;
                    case HVML_DOM_XPATH_EVAL_DOMS: {
                        A(right->u.doms.ndoms && right->u.doms.doms, "internal logic error");
                        char **rvs = NULL;
                        int *rallocates = NULL;
                        for (size_t i=0; i<right->u.doms.ndoms && !ev->u.b; ++i) {
                            hvml_dom_t *rdom = right->u.doms.doms[i];
                            if (ldom == rdom) {
                                delta   = 1-1;
                                ev->u.b = hvml_dom_xpath_to_bool(op, delta);
                                break;
                            }
                            if (!lvs) {
                                r = hvml_dom_string_for_xpath(ldom, &lvs, &allocated);
                                if (r) break;
                            }
                            if (!rvs) {
                                rvs = (char**)calloc(1, right->u.doms.ndoms * sizeof(*rvs));
                                rallocates = (int*)calloc(1, right->u.doms.ndoms * sizeof(*rallocates));
                                if (!rvs || !rallocates) break;
                            }
                            if (!rvs[i]) {
                                const char *s = NULL;
                                int allocated = 0;
                                r = hvml_dom_string_for_xpath(rdom, &s, &allocated);
                                if (r) break;
                                A(s, "internal logic error");
                                rvs[i] = (char*)s;
                                rallocates[i] = allocated;
                            }
                            delta   = strcmp(lvs, rvs[i]);
                            ev->u.b = hvml_dom_xpath_to_bool(op, delta);
                        }
                        for (size_t i=0; i<right->u.doms.ndoms; ++i) {
                            int rallocate = rallocates ? rallocates[i] : 0;
                            if (!rallocate) continue;
                            char *rv = rvs ? rvs[i] : NULL;
                            if (rv) free(rv);
                        }
                        free(rvs);
                        free(rallocates);
                    } break;
                    default: {
                        A(0, "internal logic error");
                    } break;
                }
                if (allocated && lvs) free((void*)lvs);
            }
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }
    return r;
}

static int hvml_dom_xpath_eval_arith(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, HVML_DOM_XPATH_OP_TYPE op, hvml_dom_xpath_eval_t *ev) {
    A(left,  "internal logic error");
    A(right, "internal logic error");
    A(ev,    "internal logic error");
    A(ev->et==HVML_DOM_XPATH_EVAL_UNKNOWN, "internal logic error");

    int r = 0;
    long double ld=0., rd=0.;

    ev->et = HVML_DOM_XPATH_EVAL_NUMBER;

    r = hvml_dom_xpath_eval_to_number(left, &ld);
    if (r) return r;

    r = hvml_dom_xpath_eval_to_number(right, &rd);
    if (r) return r;

    switch(op) {
        case HVML_DOM_XPATH_OP_PLUS: {
            ev->u.ldbl = ld + rd;
        } break;
        case HVML_DOM_XPATH_OP_MINUS: {
            ev->u.ldbl = ld - rd;
        } break;
        case HVML_DOM_XPATH_OP_MULTI: {
            ev->u.ldbl = ld * rd;
        } break;
        case HVML_DOM_XPATH_OP_DIV: {
            ev->u.ldbl = ld / rd;
        } break;
        case HVML_DOM_XPATH_OP_MOD: {
            ev->u.ldbl = fmodl(ld, rd);
        } break;
        default: {
            A(0, "internal logic error");
            return 0; // never reached here
        } break;
    }

    return 0;
}

int hvml_dom_query(hvml_dom_t *dom, const char *path, hvml_doms_t *doms) {
    A(path,   "internal logic error");

    int r = 0;

    hvml_doms_t out = {0};

    hvml_dom_xpath_steps_t steps = null_steps;

    r = hvml_dom_xpath_parse(path, &steps);

    do {
        if (r) break;
        r = do_hvml_dom_eval_location(dom, &steps, &out);
    } while (0);

    if (r==0) {
        if (doms) {
            r = hvml_doms_sort(doms, &out);
            if (r) {
                hvml_doms_cleanup(doms);
            }
        }
    }

    hvml_dom_xpath_steps_cleanup(&steps);
    hvml_doms_cleanup(&out);

    return r ? -1 : 0;
}



static int on_open_tag(void *arg, const char *tag) {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;
    if (!gen->dom) {
        gen->dom = hvml_dom_create();
        if (!gen->dom) return -1;
        gen->dom->dt = MKDOT(D_ROOT);
    }
    A(gen->dom, "internal logic error");
    hvml_dom_t *v       = hvml_dom_create();
    if (!v) return -1;
    v->dt      = MKDOT(D_TAG);
    if (hvml_string_set(&v->u.tag.name, tag, strlen(tag))) {
        hvml_dom_destroy(v);
        return -1;
    }
    switch (gen->dom->dt) {
        case MKDOT(D_ROOT): {
            A(DOM_HEAD(gen->dom)==NULL, "internal logic error");
            DOM_APPEND(gen->dom, v);
        } break;
        case MKDOT(D_TAG): {
            DOM_APPEND(gen->dom, v);
        } break;
        case MKDOT(D_ATTR): {
            DOM_APPEND(DOM_ATTR_OWNER(gen->dom), v);
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }
    gen->dom = v;
    return 0;
}

static int on_attr_key(void *arg, const char *key) {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;
    hvml_dom_t *v       = hvml_dom_create();
    if (!v) return -1;
    v->dt      = MKDOT(D_ATTR);
    if (hvml_string_set(&v->u.attr.key, key, strlen(key))) {
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
    if (hvml_string_set(&gen->dom->u.attr.val, val, strlen(val))) {
        return -1;
    }
    gen->dom = DOM_ATTR_OWNER(gen->dom);
    A(gen->dom, "internal logic error");
    A(gen->dom->dt == MKDOT(D_TAG), "internal logic error");
    return 0;
}

static int on_close_tag(void *arg) {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;
    A(gen->dom, "internal logic error");
    if (gen->dom->dt == MKDOT(D_ATTR)) {
        gen->dom = DOM_ATTR_OWNER(gen->dom);
        A(gen->dom->dt == MKDOT(D_TAG), "internal logic error");
    }
    A(gen->dom->dt == MKDOT(D_TAG), "internal logic error");
    A(DOM_OWNER(gen->dom), "internal logic error");
    gen->dom = DOM_OWNER(gen->dom);
    return 0;
}

static int on_text(void *arg, const char *txt) {
    hvml_dom_gen_t *gen = (hvml_dom_gen_t*)arg;
    A(gen->dom, "internal logic error");
    hvml_dom_t *v       = hvml_dom_create();
    if (!v) return -1;
    v->dt      = MKDOT(D_TEXT);
    if (hvml_string_set(&v->u.txt.txt, txt, strlen(txt))) {
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
    (void)arg;
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

static int on_number(void *arg, const char *origin, long double val) {
    hvml_jo_value_t *jo = hvml_jo_number(val, origin);
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
    A(dom, "internal logic error");
    A(tvs, "internal logic error");
    int lvl = 0;
    int r = 0;
    int pop = 0; // 1:pop from attr, 2:pop from child

    // if (dom->dt == MKDOT(D_ROOT)) {
    //     dom = DOM_HEAD(dom);
    //     if (!dom) return 0;
    //     A(dom->dt == MKDOT(D_TAG), "internal logic error");
    //     A(DOM_NEXT(dom)==NULL, "internal logic error");
    // }

    while (r==0) {
        switch (dom->dt) {
            case MKDOT(D_ROOT):
            {
                A(lvl == 0, "internal logic error");
                r = apply_traverse_callback(dom, lvl, 0, tvs);
                if (r) continue;

                hvml_dom_t *child = DOM_HEAD(dom);
                if (!child) return 0;
                A(child->dt==MKDOT(D_TAG), "=%s=", hvml_dom_type_str(child->dt));

                ++lvl;
                dom = child;
                continue;
            } break;
            case MKDOT(D_TAG):
            {
                if (pop==1) { // pop from attr
                    hvml_dom_t *child = DOM_HEAD(dom);
                    if (child) {
                        A(child->dt==MKDOT(D_TAG) || child->dt==MKDOT(D_TEXT) || child->dt==MKDOT(D_JSON), "=%s=", hvml_dom_type_str(child->dt));
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
                    if (lvl>0) {
                        hvml_dom_t *sibling = DOM_NEXT(dom);
                        if (sibling) {
                            dom = sibling;
                            A(dom->dt==MKDOT(D_TAG) || dom->dt==MKDOT(D_TEXT) || dom->dt==MKDOT(D_JSON), "");
                            pop = 0;
                            continue;
                        }
                    }
                    hvml_dom_t *parent = DOM_OWNER(dom);
                    if (parent) {
                        if (lvl == 0) return 0; // normal end up.
                        if (parent->dt == MKDOT(D_ROOT)) return 0;

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

                r = apply_traverse_callback(dom, lvl, 1, tvs);
                if (r) continue;

                hvml_dom_t *attr = DOM_ATTR_HEAD(dom);
                if (attr) {
                    dom = attr;
                    ++lvl;
                    A(dom->dt==MKDOT(D_ATTR), "internal logic error");
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

                if (lvl==0) return 0;

                hvml_dom_t *attr = DOM_ATTR_NEXT(dom);
                if (attr) {
                    dom = attr;
                    A(dom->dt==MKDOT(D_ATTR), "internal logic error");
                    continue;
                }

                hvml_dom_t *parent = DOM_ATTR_OWNER(dom);
                A(parent, "internal logic error");
                A(parent->dt == MKDOT(D_TAG), "internal logic error");
                A(lvl>0, "internal logic error");
                --lvl;
                dom = parent;
                pop = 1; // pop from attr
                continue;
            } break;
            case MKDOT(D_TEXT):
            {
                A(pop==0, "internal logic error");
                r = apply_traverse_callback(dom, lvl+1, 0, tvs);
                if (r) continue;

                if (lvl==0) return 0;

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
                if (r) continue;

                if (lvl==0) return 0;

                hvml_dom_t *sibling = DOM_NEXT(dom);
                if (sibling) {
                    dom = sibling;
                        A(dom->dt==MKDOT(D_TAG) || dom->dt==MKDOT(D_TEXT) || dom->dt==MKDOT(D_JSON), "");
                    pop = 0;
                    continue;
                }
                if (lvl == 0) return 0;// normal end up.

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

static int do_hvml_dom_back_traverse(hvml_dom_t *dom, back_traverse_t *tvs) {
    A(tvs, "internal logic error");
    int lvl = 0;
    int r = 0;
    // int pop = 0; // 1:pop from attr, 2:pop from child
    while (r==0 && dom) {
        switch (dom->dt) {
            case MKDOT(D_TAG):
            case MKDOT(D_TEXT):
            case MKDOT(D_JSON):
            {
                r = apply_back_traverse_callback(dom, lvl, tvs);
                dom = DOM_OWNER(dom);
                --lvl;
                continue;
            } break;
            case MKDOT(D_ATTR):
            {
                r = apply_back_traverse_callback(dom, lvl, tvs);
                dom = DOM_ATTR_OWNER(dom);
                --lvl;
                continue;
            } break;
            default:
            {
                A(0, "internal logic error");
            } break;
        }
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

static int apply_back_traverse_callback(hvml_dom_t *dom, int lvl, back_traverse_t *tvs) {
    A(dom, "internal logic error");
    A(lvl<=0, "internal logic error");
    A(tvs, "internal logic error");
    if (!tvs->back_traverse_cb) return 0;

    int breakout = 0;
    tvs->back_traverse_cb(dom, lvl, tvs->arg, &breakout);
    return breakout ? -1 : 0;
}

