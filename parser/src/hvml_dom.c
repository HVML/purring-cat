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

typedef enum {
    HVML_DOM_XPATH_EVAL_UNKNOWN,
    HVML_DOM_XPATH_EVAL_BOOL,
    HVML_DOM_XPATH_EVAL_INTEGER,
    HVML_DOM_XPATH_EVAL_DOUBLE,
    HVML_DOM_XPATH_EVAL_LITERAL,
    HVML_DOM_XPATH_EVAL_DOMS
} HVML_DOM_XPATH_EVAL_TYPE;

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

typedef struct hvml_dom_xpath_eval_s           hvml_dom_xpath_eval_t;

struct hvml_dom_xpath_eval_s {
    HVML_DOM_XPATH_EVAL_TYPE     et;
    union {
        int              b;
        int64_t          i64;
        double           dbl;
        const char      *literal;
        hvml_doms_t      doms;
    };
};

static void hvml_dom_xpath_eval_cleanup(hvml_dom_xpath_eval_t *ev) {
    if (!ev) return;
    switch (ev->et) {
        case HVML_DOM_XPATH_EVAL_UNKNOWN: break;
        case HVML_DOM_XPATH_EVAL_BOOL: {
            ev->b = 0;
        } break;
        case HVML_DOM_XPATH_EVAL_INTEGER: {
            ev->i64 = 0;
        } break;
        case HVML_DOM_XPATH_EVAL_DOUBLE: {
            ev->dbl = 0;
        } break;
        case HVML_DOM_XPATH_EVAL_LITERAL: {
            ev->literal = NULL;
        } break;
        case HVML_DOM_XPATH_EVAL_DOMS: {
            hvml_doms_cleanup(&ev->doms);
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }
    ev->et = HVML_DOM_XPATH_OP_UNSPECIFIED;
}

const hvml_doms_t null_doms = {0};

int hvml_doms_append_dom(hvml_doms_t *doms, hvml_dom_t *dom) {
    if (!doms) return -1;
    if (!dom) return 0;
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
    size_t n = doms->ndoms + in->ndoms;
    hvml_dom_t **e = (hvml_dom_t**)realloc(doms->doms, (n)*sizeof(*e));
    if (!e) return -1;
    memcpy(e + doms->ndoms, in->doms, in->ndoms * sizeof(*e));
    doms->doms     = e;
    doms->ndoms    = n;

    return r;
}

void hvml_doms_cleanup(hvml_doms_t *doms) {
    if (!doms) return;

    free(doms->doms);
    doms->doms     = NULL;
    doms->ndoms    = 0;
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
        int ret = hvml_string_set(&v->attr.key, key, key_len);
        if (ret) break;
        if (val) {
            ret = hvml_string_set(&v->attr.val, val, val_len);
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
        int ret = hvml_string_set(&dom->attr.val, val, val_len);
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
        int ret = hvml_string_set(&v->txt.txt, txt, len);
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
        int ret = hvml_string_set(&v->tag.name, tag, len);
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
        v->jo              = jo;
        DOM_APPEND(dom, v);
        return v;
    }
    // jo is subvalue, clone it
    v->jo = hvml_jo_clone(jo);
    if (!v->jo) {
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
    return DOM_OWNER(dom);
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

void hvml_dom_attr_set_key(hvml_dom_t *dom, const char *key, size_t key_len) {
    A((dom->dt == MKDOT(D_ATTR)), "internal logic error");
    hvml_string_set(&dom->attr.key, key, key_len);
}

void hvml_dom_attr_set_val(hvml_dom_t *dom, const char *val, size_t val_len) {
    A((dom->dt == MKDOT(D_ATTR)), "internal logic error");
    hvml_string_set(&dom->attr.val, val, val_len);
}

void hvml_dom_set_text(hvml_dom_t *dom, const char *txt, size_t txt_len) {
    A((dom->dt == MKDOT(D_TEXT)), "internal logic error");
    hvml_string_set(&dom->txt.txt, txt, txt_len);
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

typedef struct collect_descentant_s          collect_descentant_t;
struct collect_descentant_s {
    hvml_doms_t        *out;
    int                 failed;
};

static void collect_descentant_cb(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout) {
    collect_descentant_t *parg = (collect_descentant_t*)arg;
    A(parg, "internal logic error");

    *breakout = 0;

    int r = 0;
    switch (hvml_dom_type(dom)) {
        case MKDOT(D_TAG):
        {
            switch (tag_open_close) {
                case 1: {
                    r = hvml_doms_append_dom(parg->out, dom);
                } break;
                case 2: break;
                case 3: break;
                case 4: break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        case MKDOT(D_ATTR): break;
        case MKDOT(D_TEXT): {
            r = hvml_doms_append_dom(parg->out, dom);
        } break;
        case MKDOT(D_JSON): {
            r = hvml_doms_append_dom(parg->out, dom);
        } break;
        default: {
            A(0, "internal logic error");
        } break;
    }

    if (r) {
        *breakout = 1;
        parg->failed = 1;
    }
}

static int hvml_doms_append_descentant(hvml_doms_t *out, hvml_dom_t *dom) {
    A(out,    "internal logic error");
    A(dom,    "internal logic error");
    collect_descentant_t      collect = {0};
    collect.out = out;
    collect.failed = 0;
    hvml_dom_traverse(dom, &collect, collect_descentant_cb);
    return collect.failed ? -1 : 0;
}


typedef struct collect_relative_s          collect_relative_t;
struct collect_relative_s {
    hvml_doms_t        *out;
    int                 following;
    hvml_dom_t         *relative;
    int                 hit;
    int                 failed;
};

static void collect_relative_cb(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout) {
    collect_relative_t *parg = (collect_relative_t*)arg;
    A(parg, "internal logic error");

    *breakout = 0;

    int r = 0;
    switch (hvml_dom_type(dom)) {
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
        case MKDOT(D_ATTR): return;
        case MKDOT(D_TEXT): break;
        case MKDOT(D_JSON): break;
        default: {
            A(0, "internal logic error");
        } break;
    }
    if (parg->following) {
        if (!parg->hit) {
            if (dom == parg->relative) {
                parg->hit = 1;
            }
            return;
        }
    } else {
        A(!parg->hit, "internal logic error");
        if (dom == parg->relative) {
            parg->hit = 1;
            *breakout = 1;
            return;
        }
    }

    r = hvml_doms_append_dom(parg->out, dom);

    if (r) {
        *breakout = 1;
        parg->failed = 1;
    }
}

static int hvml_doms_append_relative(hvml_doms_t *out, int following, hvml_dom_t *dom) {
    A(out,    "internal logic error");
    A(dom,    "internal logic error");
    collect_relative_t      collect = {0};
    collect.out       = out;
    collect.following = following;
    collect.relative  = dom;
    collect.hit       = 0;
    collect.failed    = 0;
    hvml_dom_t *root = hvml_dom_root(dom);
    A(root, "internal logic error");
    hvml_dom_traverse(root, &collect, collect_relative_cb);
    return collect.failed ? -1 : 0;
}


static hvml_dom_t* do_hvml_dom_check_node_test(hvml_dom_t *dom, hvml_dom_xpath_node_test_t *node_test, int *r);

static int do_hvml_dom_eval_location(hvml_dom_t *dom, hvml_dom_xpath_steps_t *steps, hvml_doms_t *out);
static int do_hvml_doms_eval_step(hvml_doms_t *doms, hvml_dom_xpath_step_t *step, hvml_doms_t *out);
static int do_hvml_dom_eval_step(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_step_t *step, hvml_doms_t *out);
static int do_hvml_dom_eval_axis(hvml_dom_t *dom, hvml_dom_xpath_step_axis_t *axis, hvml_doms_t *out);
static int do_hvml_dom_eval_node_test(hvml_dom_t *dom, hvml_dom_xpath_node_test_t *node_test, hvml_dom_t **v);
static int do_hvml_dom_eval_exprs(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_exprs_t *exprs, hvml_dom_t **v);
static int do_hvml_dom_eval_expr(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_expr_t *expr, hvml_dom_xpath_eval_t *ev);
static int do_hvml_dom_eval_union_expr(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_union_expr_t *expr, hvml_dom_t **v);
static int do_hvml_dom_eval_path_expr(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_path_expr_t *expr, hvml_dom_t **v);
static int do_hvml_dom_eval_filter(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_filter_expr_t *filter, hvml_doms_t *doms);
static int do_hvml_dom_eval_primary(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_primary_t *primary, hvml_dom_xpath_eval_t *ev);
static int do_hvml_dom_eval_func(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_func_t *func_call, hvml_dom_xpath_eval_t *ev);

static int hvml_dom_xpath_eval_or(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);
static int hvml_dom_xpath_eval_and(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);
static int hvml_dom_xpath_eval_eq(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);
static int hvml_dom_xpath_eval_neq(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);
static int hvml_dom_xpath_eval_lt(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);
static int hvml_dom_xpath_eval_gt(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);
static int hvml_dom_xpath_eval_lte(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);
static int hvml_dom_xpath_eval_gt3(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);
static int hvml_dom_xpath_eval_plus(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);
static int hvml_dom_xpath_eval_minus(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);
static int hvml_dom_xpath_eval_multi(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);
static int hvml_dom_xpath_eval_div(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);
static int hvml_dom_xpath_eval_mod(hvml_dom_xpath_eval_t *left, hvml_dom_xpath_eval_t *right, hvml_dom_xpath_eval_t *ev);

static hvml_dom_t* do_hvml_dom_check_node_test(hvml_dom_t *dom, hvml_dom_xpath_node_test_t *node_test, int *r) {
    A(dom,          "internal logic error");
    A(node_test,    "internal logic error");
    A(r,            "internal logic error");
    A(node_test->is_cleanedup==0, "internal logic error");

    if (!node_test->is_name_test) {
        switch (node_test->node_type) {
            case HVML_DOM_XPATH_NT_UNSPECIFIED: {
            } break;
            case HVML_DOM_XPATH_NT_COMMENT: {
                dom = NULL;
            } break;
            case HVML_DOM_XPATH_NT_TEXT: {
                if (dom->dt != MKDOT(D_TEXT)) dom = NULL;
            } break;
            case HVML_DOM_XPATH_NT_PROCESSING_INSTRUCTION: {
                dom = NULL;
            } break;
            case HVML_DOM_XPATH_NT_NODE: {
                if (dom->dt != MKDOT(D_TAG)) dom = NULL;
            } break;
            case HVML_DOM_XPATH_NT_JSON: {
                if (dom->dt != MKDOT(D_JSON)) dom = NULL;
            } break;
            default: {
                A(0, "internal logic error");
                dom = NULL; // never reached here
            } break;
        }
        return dom;
    }

    const char *prefix     = node_test->name_test.prefix;
    const char *local_part = node_test->name_test.local_part;
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
            return dom;
        }
        // prefix:*
        if (!tok) return NULL;
        if (strstr(tok, prefix)!=tok) return NULL;
        if (tok[strlen(prefix)]!=':') return NULL;
        return dom;
    }
    if (prefix && strcmp(prefix, "*")==0) {
        // "*:xxx"
        if (!tok) return NULL;
        if (!colon) return NULL;
        if (strcmp(colon+1, local_part)==0) return dom;
        return NULL;
    }
    if (!prefix) {
        // "xxx"
        if (!tok) return NULL;
        if (strcmp(tok, local_part)==0) return dom;
        return NULL;
    }
    // "xxx:yyy"
    if (!tok) return NULL;
    if (!colon) return NULL;
    if (strstr(tok, prefix)!=tok) return NULL;
    if (strcmp(colon+1, local_part)==0) return dom;
    return NULL;
}

static int do_hvml_dom_eval_union_expr(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_union_expr_t *expr, hvml_dom_t **v) {
    A(dom,          "internal logic error");
    A(expr,         "internal logic error");
    A(v,            "internal logic error");
    A(expr->is_cleanedup==0, "internal logic error");
    A(expr->paths && expr->npaths>0,  "internal logic error");

    /*
    hvml_dom_xpath_path_expr_t    *paths;
    size_t                         npaths;
    int                            uminus;
    */

    *v = NULL;

    int r = 0;
    for (size_t i=0; i<expr->npaths && r==0; ++i) {
        hvml_dom_xpath_path_expr_t *path_expr = expr->paths + i;
        hvml_dom_t *d = NULL;
        r = do_hvml_dom_eval_path_expr(dom, position, range, path_expr, &d);
        if (r) return r;
        if (d) {
            A(dom==d, "internal logic error");
            return 0;
        }
    }

    return 0;
}

static int do_hvml_dom_eval_primary(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_primary_t *primary, hvml_dom_xpath_eval_t *ev) {
    A(dom,          "internal logic error");
    A(primary,      "internal logic error");
    A(ev,           "internal logic error");
    A(primary->is_cleanedup==0, "internal logic error");
    /*
    HVML_DOM_XPATH_PRIMARY_TYPE primary_type;
    union {
        hvml_dom_xpath_qname_t  variable;
        hvml_dom_xpath_expr_t   expr;
        int64_t                 integer;
        double                  dbl;
        char                   *literal;
        hvml_dom_xpath_func_t   func_call;
    };
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
                r = do_hvml_dom_eval_expr(dom, position, range, &primary->expr, ev);
                return r;
			} break;
            case HVML_DOM_XPATH_PRIMARY_INTEGER: {
                ev->et = HVML_DOM_XPATH_EVAL_INTEGER;
                ev->i64 = primary->integer;
                return r;
			} break;
            case HVML_DOM_XPATH_PRIMARY_DOUBLE: {
                ev->et = HVML_DOM_XPATH_EVAL_DOUBLE;
                ev->dbl = primary->integer;
                return r;
			} break;
            case HVML_DOM_XPATH_PRIMARY_LITERAL: {
                A(0, "not implemented yet");
			} break;
            case HVML_DOM_XPATH_PRIMARY_FUNC: {
                r = do_hvml_dom_eval_func(dom, position, range, &primary->func_call, ev);
			} break;
            default: {
                A(0, "internal logic error");
            } break;
        }
    } while (0);

    return r;
}

static int do_hvml_dom_eval_filter(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_filter_expr_t *filter, hvml_doms_t *doms) {
    A(dom,          "internal logic error");
    A(filter,       "internal logic error");
    A(doms,         "internal logic error");
    A(filter->is_cleanedup==0, "internal logic error");
    /*
    hvml_dom_xpath_primary_t  primary;
    hvml_dom_xpath_exprs_t    exprs;
    */

    int r = 0;

    hvml_doms_t out = {0};

    do {
        hvml_dom_xpath_eval_t ev = {0};
        r = do_hvml_dom_eval_primary(dom, position, range, &filter->primary, &ev);
        if (r) break;

        hvml_dom_t *e = NULL;
        r = do_hvml_dom_eval_exprs(dom, position, range, &filter->exprs, &e);
        if (r) break;
        if (e==NULL) break;
        A(dom==e, "internal logic error");

        r = hvml_doms_append_dom(&out, e);
        if (r) break;
    } while (0);

    if (r==0) {
        if (doms) {
            *doms = out;
        }
    }
    return r;
}

static int do_hvml_dom_eval_path_expr(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_path_expr_t *expr, hvml_dom_t **v) {
    A(dom,          "internal logic error");
    A(expr,         "internal logic error");
    A(v,            "internal logic error");
    A(expr->is_cleanedup==0, "internal logic error");

    int r = 0;
    hvml_dom_t *d = NULL;
    hvml_doms_t doms = {0};
    do {
        if (expr->is_location) {
            r = do_hvml_dom_eval_location(dom, &expr->location, &doms);
            if (r) break;
            if (doms.ndoms>0) d = dom;
        } else {
            r = do_hvml_dom_eval_filter(dom, position, range, &expr->filter_expr, &d);
            if (r) break;
            if (!d) break;
            A(d==dom, "internal logic error");
        }
    } while (0);

    hvml_doms_cleanup(&doms);

    if (r==0) {
        *v = d;
    }

    return r;
}

static int do_hvml_dom_eval_expr(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_expr_t *expr, hvml_dom_xpath_eval_t *ev) {
    A(dom,          "internal logic error");
    A(expr,         "internal logic error");
    A(ev,           "internal logic error");
    /*
    unsigned int is_binary_op:1;

    hvml_dom_xpath_union_expr_t    *unary;

    HVML_DOM_XPATH_OP_TYPE         op_type;
    hvml_dom_xpath_expr_t          *left;
    hvml_dom_xpath_expr_t          *right;
    */

    int r = 0;

    if (!expr->is_binary_op) {
        hvml_dom_t *v = NULL;
        r = do_hvml_dom_eval_union_expr(dom, position, range, expr->unary, &v);
        if (r) return r;
        ev->et = HVML_DOM_XPATH_EVAL_DOMS;
        ev->dom = v;
        return 0;
    } else {
        hvml_dom_xpath_eval_t left  = {0};
        hvml_dom_xpath_eval_t right = {0};

        hvml_dom_xpath_eval_t r = {0};

        A(expr->left, "internal logic error");
        do {
            // note: recursive
            r = do_hvml_dom_eval_expr(dom, position, range, expr->left, &left);
            if (r) break;
            if (expr->right) {
                r = do_hvml_dom_eval_expr(dom, position, range, expr->right, &right);
                if (r) break;
            }
            switch (expr->op_type) {
                case HVML_DOM_XPATH_OP_UNSPECIFIED: {
                    A(0, "internal logic error");
                } break;
                case HVML_DOM_XPATH_OP_OR: {
                    r = hvml_dom_xpath_eval_or(&left, &right, ev);
				} break;
                case HVML_DOM_XPATH_OP_AND: {
                    r = hvml_dom_xpath_eval_and(&left, &right, ev);
				} break;
                case HVML_DOM_XPATH_OP_EQ: {
                    r = hvml_dom_xpath_eval_eq(&left, &right, ev);
				} break;
                case HVML_DOM_XPATH_OP_NEQ: {
                    r = hvml_dom_xpath_eval_neq(&left, &right, ev);
				} break;
                case HVML_DOM_XPATH_OP_LT: {
                    r = hvml_dom_xpath_eval_lt(&left, &right, ev);
				} break;
                case HVML_DOM_XPATH_OP_GT: {
                    r = hvml_dom_xpath_eval_gt(&left, &right, ev);
				} break;
                case HVML_DOM_XPATH_OP_LTE: {
                    r = hvml_dom_xpath_eval_lte(&left, &right, ev);
				} break;
                case HVML_DOM_XPATH_OP_GTE: {
                    r = hvml_dom_xpath_eval_gte(&left, &right, ev);
				} break;
                case HVML_DOM_XPATH_OP_PLUS: {
                    r = hvml_dom_xpath_eval_plus(&left, &right, ev);
				} break;
                case HVML_DOM_XPATH_OP_MINUS: {
                    r = hvml_dom_xpath_eval_minus(&left, &right, ev);
				} break;
                case HVML_DOM_XPATH_OP_MULTI: {
                    r = hvml_dom_xpath_eval_multi(&left, &right, ev);
				} break;
                case HVML_DOM_XPATH_OP_DIV: {
                    r = hvml_dom_xpath_eval_div(&left, &right, ev);
				} break;
                case HVML_DOM_XPATH_OP_MOD: {
                    r = hvml_dom_xpath_eval_mod(&left, &right, ev);
				} break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } while (0);

        hvml_dom_xpath_eval_cleanup(&left);
        hvml_dom_xpath_eval_cleanup(&right);

        return r;
    }
}

static int do_hvml_dom_eval_exprs(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_exprs_t *exprs, hvml_dom_t **v) {
    A(dom,    "internal logic error");
    A(exprs,  "internal logic error");
    A(v,      "internal logic error");

    *v = NULL;

    int r = 0;

    *v = NULL;

    for (size_t i=0; i<exprs->nexprs; ++i) {
        hvml_dom_xpath_expr_t *expr = exprs->exprs + i;
        hvml_dom_xpath_eval_t ev = {0};
        r = do_hvml_dom_eval_expr(dom, position, range, expr, &ev);
        if (r) return r;
        switch (ev.et) {
            case HVML_DOM_XPATH_EVAL_UNKNOWN: {
                A(0, "internal logic error");
			} break;
            case HVML_DOM_XPATH_EVAL_BOOL: {
                if (!ev.b) return 0;
			} break;
            case HVML_DOM_XPATH_EVAL_INTEGER: {
                if (ev.i64!=position) return 0;
			} break;
            case HVML_DOM_XPATH_EVAL_DOUBLE: {
                A(0, "internal logic error");
			} break;
            case HVML_DOM_XPATH_EVAL_LITERAL: {
                A(0, "internal logic error");
			} break;
            case HVML_DOM_XPATH_EVAL_DOMS: {
                if (ev.doms.ndoms==0) return 0;
			} break;
            default: {
                A(0, "internal logic error");
            } break;
        }
    }

    *v = dom;

    return 0;
}

static int do_hvml_dom_eval_node_test(hvml_dom_t *dom, hvml_dom_xpath_node_test_t *node_test, hvml_dom_t **v) {
    A(dom,               "internal logic error");
    A(node_test,         "internal logic error");
    A(v,                 "internal logic error");
    A(node_test->is_cleanedup==0, "internal logic error");

    int r = 0;

    *v = NULL;

    if (!node_test->is_name_test) {
        switch (node_test->node_type) {
            case HVML_DOM_XPATH_NT_UNSPECIFIED: {
                A(0, "internal logic error");
            } break;
            case HVML_DOM_XPATH_NT_COMMENT: {
            } break;
            case HVML_DOM_XPATH_NT_TEXT: {
                if (dom->dt == MKDOT(D_TEXT)) *v = dom;
            } break;
            case HVML_DOM_XPATH_NT_PROCESSING_INSTRUCTION: {
                A(0, "not implemented yet");
            } break;
            case HVML_DOM_XPATH_NT_NODE: {
                if (dom->dt == MKDOT(D_TAG)) *v = dom;
            } break;
            case HVML_DOM_XPATH_NT_JSON: {
                if (dom->dt == MKDOT(D_JSON)) *v = dom;
            } break;
            default: {
                A(0, "internal logic error");
                // never reached here
            } break;
        }

        return r;
    }

    const char *prefix     = node_test->name_test.prefix;
    const char *local_part = node_test->name_test.local_part;
    A(local_part, "internal logic error");
    const char *tok = NULL;
    if (dom->dt == MKDOT(D_TAG)) {
        tok = hvml_dom_tag_name(dom);
    } else if (dom->dt == MKDOT(D_ATTR)) {
        tok = hvml_dom_attr_key(dom);
    }
    const char *colon = tok ? strchr(tok, ':') : NULL;

    do {
        if (strcmp(local_part, "*")==0) {
            if (prefix==NULL) {
                // "*"
                *v = dom;
                break;
            }
            // prefix:*
            if (!tok) break;
            if (strstr(tok, prefix)!=tok) break;
            if (tok[strlen(prefix)]!=':') break;
            *v = dom;
            break;
        }
        if (prefix && strcmp(prefix, "*")==0) {
            // "*:xxx"
            if (!tok) break;
            if (!colon) break;
            if (strcmp(colon+1, local_part)) break;
            *v = dom;
            break;
        }
        if (!prefix) {
            // "xxx"
            if (!tok) break;
            if (strcmp(tok, local_part)) break;
            *v = dom;
            break;
        }
        // "xxx:yyy"
        if (!tok) break;
        if (!colon) break;
        if (strstr(tok, prefix)!=tok) break;
        if (strcmp(colon+1, local_part)) break;
        *v = dom;
    } while (0);

    return r;
}

static int do_hvml_dom_eval_axis(hvml_dom_t *dom, hvml_dom_xpath_step_axis_t *axis, hvml_doms_t *out) {
    A(dom,               "internal logic error");
    A(axis,              "internal logic error");
    A(out,               "internal logic error");
    A(axis->is_cleanedup==0, "internal logic error");

    /*
    unsigned int is_cleanedup:1;
    HVML_DOM_XPATH_AXIS_TYPE          axis;
    hvml_dom_xpath_node_test_t        node_test;
    hvml_dom_xpath_exprs_t            exprs;
    */

    int r = 0;
    hvml_doms_t in = {0};

    switch (axis->axis) {
        case HVML_DOM_XPATH_AXIS_UNSPECIFIED: {
            r = hvml_doms_append_dom(&in, dom);
        } break;
        case HVML_DOM_XPATH_AXIS_ANCESTOR: {
            if (dom->dt==MKDOT(D_ATTR)) break;
            dom = DOM_OWNER(dom);
            while (r==0 && dom) {
                r = hvml_doms_append_dom(&in, dom);
                if (r) break;
                dom = DOM_OWNER(dom);
            }
        } break;
        case HVML_DOM_XPATH_AXIS_ANCESTOR_OR_SELF: {
            if (dom->dt==MKDOT(D_ATTR)) break;
            while (r==0 && dom) {
                r = hvml_doms_append_dom(&in, dom);
                if (r) break;
                dom = DOM_OWNER(dom);
            }
        } break;
        case HVML_DOM_XPATH_AXIS_ATTRIBUTE: {
            if (dom->dt!=MKDOT(D_TAG)) break;
            dom = DOM_ATTR_HEAD(dom);
            while (r==0 && dom) {
                r = hvml_doms_append_dom(&in, dom);
                if (r) break;
                dom = DOM_ATTR_NEXT(dom);
            }
        } break;
        case HVML_DOM_XPATH_AXIS_CHILD: {
            if (dom->dt!=MKDOT(D_TAG)) break;
            dom = DOM_HEAD(dom);
            while (r==0 && dom) {
                r = hvml_doms_append_dom(&in, dom);
                if (r) break;
                dom = DOM_NEXT(dom);
            }
        } break;
        case HVML_DOM_XPATH_AXIS_DESCENDANT: {
            if (dom->dt!=MKDOT(D_TAG)) break;
            r = hvml_doms_append_descentant(&in, dom);
        } break;
        case HVML_DOM_XPATH_AXIS_DESCENDANT_OR_SELF: {
            if (dom->dt!=MKDOT(D_TAG)) break;
            r = hvml_doms_append_dom(&in, dom);
            if (r) break;
            r = hvml_doms_append_descentant(&in, dom);
        } break;
        case HVML_DOM_XPATH_AXIS_FOLLOWING: {
            if (dom->dt==MKDOT(D_ATTR)) break;
            r = hvml_doms_append_relative(&in, 1, dom);
        } break;
        case HVML_DOM_XPATH_AXIS_FOLLOWING_SIBLING: {
            if (dom->dt==MKDOT(D_ATTR)) break;
            dom = DOM_NEXT(dom);
            while (r==0 && dom) {
                r = hvml_doms_append_dom(&in, dom);
                if (r) break;
                dom = DOM_NEXT(dom);
            }
        } break;
        case HVML_DOM_XPATH_AXIS_NAMESPACE: {
            if (dom->dt==MKDOT(D_ATTR)) break;
            r = hvml_doms_append_relative(&in, 0, dom);
        } break;
        case HVML_DOM_XPATH_AXIS_PARENT: {
            if (dom->dt==MKDOT(D_ATTR)) break;
            dom = DOM_OWNER(dom);
            r = hvml_doms_append_dom(&in, dom);
        } break;
        case HVML_DOM_XPATH_AXIS_PRECEDING: {
            if (dom->dt==MKDOT(D_ATTR)) break;
            A(0, "not implemented yet");
        } break;
        case HVML_DOM_XPATH_AXIS_PRECEDING_SIBLING: {
            if (dom->dt==MKDOT(D_ATTR)) break;
            dom = DOM_PREV(dom);
            while (r==0 && dom) {
                r = hvml_doms_append_dom(&in, dom);
                if (r) break;
                dom = DOM_PREV(dom);
            }
        } break;
        case HVML_DOM_XPATH_AXIS_SELF: {
            r = hvml_doms_append_dom(&in, dom);
        } break;
        case HVML_DOM_XPATH_AXIS_SLASH: {
            do {
                if (dom->dt!=MKDOT(D_TAG)) break;
                dom = DOM_OWNER(dom);
                r = hvml_doms_append_dom(&in, dom);
                if (r) break;
                if (out) {
                    *out = in;
                    in = null_doms;
                }
            } while (0);
            hvml_doms_cleanup(&in);
            return r;
        } break;
        case HVML_DOM_XPATH_AXIS_SLASH2: {
            do {
                if (dom->dt!=MKDOT(D_TAG)) break;
                dom = hvml_dom_root(dom);
                r = hvml_doms_append_dom(&in, dom);
                if (r) break;
                r = hvml_doms_append_descentant(&in, dom);
                if (r) break;
                if (out) {
                    *out = in;
                    in = null_doms;
                }
            } while (0);
            hvml_doms_cleanup(&in);
            return r;
        } break;
        default: {
            A(0, "internal logic error");
            r = -1;
        } break;
    }

    if (r) {
        hvml_doms_cleanup(&in);
        return r;
    }

    hvml_doms_t tmp = {0};

    for (size_t i=0; i<in.ndoms && r==0; ++i) {
        hvml_dom_t *d = in.doms[i];
        if (!d) continue;

        hvml_dom_t *e = NULL;

        r = do_hvml_dom_eval_node_test(d, &axis->node_test, &e);
        if (r) break;
        if (e==NULL) continue;
        A(d==e, "internal logic error");

        e = NULL;
        r = do_hvml_dom_eval_exprs(d, i, in.ndoms, &axis->exprs, &e);
        if (r) break;
        if (e==NULL) continue;
        A(d==e, "internal logic error");

        r = hvml_doms_append_dom(&tmp, d);
        if (r) break;
    }

    if (r==0) {
        if (out) {
            *out = tmp;
            tmp = null_doms;
        }
    }

    hvml_doms_cleanup(&tmp);
    hvml_doms_cleanup(&in);

    return r;
}

static int do_hvml_dom_eval_step(hvml_dom_t *dom, size_t position, size_t range, hvml_dom_xpath_step_t *step, hvml_doms_t *out) {
    A(dom,               "internal logic error");
    A(step,              "internal logic error");
    A(out,               "internal logic error");
    A(step->is_cleanedup==0, "internal logic error");

    int r = 0;
    hvml_doms_t o = {0};

    do {
        if (step->is_axis) {
            r = do_hvml_dom_eval_axis(dom, &step->axis, &o);
            break;
        }

        switch (step->abbre) {
            case HVML_DOM_XPATH_AS_UNSPECIFIED: {
                A(0, "internal logic error");
            } break;
            case HVML_DOM_XPATH_AS_PARENT: {
                A(dom->dt!=MKDOT(D_ATTR), "internal logic error");
                dom = DOM_OWNER(dom);
                if (!dom) break;
                r = hvml_doms_append_dom(&o, dom);
            } break;
            case HVML_DOM_XPATH_AS_GRANDPARENT: {
                A(dom->dt!=MKDOT(D_ATTR), "internal logic error");
                dom = DOM_OWNER(dom);
                if (!dom) break;
                dom = DOM_OWNER(dom);
                if (!dom) break;
                r = hvml_doms_append_dom(&o, dom);
            } break;
            default: {
                A(0, "internal logic error");
                r = -1;
            } break;
        }
    } while (0);

    if (r==0) {
        if (out) {
            *out = o;
            o = null_doms;
        }
    }

    hvml_doms_cleanup(&o);

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

        r = do_hvml_dom_eval_step(dom, i, doms->ndoms, step, &tmp);
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

        hvml_doms_t tmp = {0};

        for (size_t i=0; i<steps->nsteps; ++i) {
            r = do_hvml_doms_eval_step(&in, steps->steps+i, &tmp);
            if (r) break;
            hvml_doms_cleanup(&in);
            in = tmp;
            tmp = null_doms;
        }

        hvml_doms_cleanup(&tmp);
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
            *doms = out;
            out = null_doms;
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
    if (hvml_string_set(&v->tag.name, tag, strlen(tag))) {
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
                    hvml_dom_t *sibling = DOM_NEXT(dom);
                    if (sibling) {
                        dom = sibling;
                        A(dom->dt==MKDOT(D_TAG) || dom->dt==MKDOT(D_TEXT) || dom->dt==MKDOT(D_JSON), "");
                        pop = 0;
                        continue;
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
    int pop = 0; // 1:pop from attr, 2:pop from child
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

