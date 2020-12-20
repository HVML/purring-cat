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

#include <ctype.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// for easy coding
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
    hvml_json_parser_t       *parser;
};

typedef struct hvml_jo_true_s         hvml_jo_true_t;
typedef struct hvml_jo_false_s        hvml_jo_false_t;
typedef struct hvml_jo_null_s         hvml_jo_null_t;
typedef struct hvml_jo_number_s       hvml_jo_number_t;
typedef struct hvml_jo_string_s       hvml_jo_string_t;
typedef struct hvml_jo_object_s       hvml_jo_object_t;
typedef struct hvml_jo_array_s        hvml_jo_array_t;
typedef struct hvml_jo_object_kv_s    hvml_jo_object_kv_t;

// Empty struct is a GCC extension only. In C99 it is invalid - holly
#ifdef __GNUC__ 
  #define EMPTY_STRUCT_DEF
#else
  #define EMPTY_STRUCT_DEF int placeholder[0];
#endif

struct hvml_jo_true_s { 
    EMPTY_STRUCT_DEF
};

struct hvml_jo_false_s { EMPTY_STRUCT_DEF };

struct hvml_jo_null_s { EMPTY_STRUCT_DEF };

struct hvml_jo_number_s {
    long double    ldbl;
    char          *origin;
};

struct hvml_jo_string_s {
    char   *str;
    size_t  len;
};

struct hvml_jo_object_s {
    EMPTY_STRUCT_DEF
};

struct hvml_jo_array_s {
    EMPTY_STRUCT_DEF
};

struct hvml_jo_object_kv_s {
    char                    *key;
    size_t                   len;
    hvml_jo_value_t         *val;
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
    } u;

    VAL_MEMBERS();
};


#define hvml_jo_value_from_union(ptr) \
    ptr ? (hvml_jo_value_t*)(((char*)ptr)-offsetof(hvml_jo_value_t, jstr)) : NULL

const char *hvml_jo_type_str(HVML_JO_TYPE t) {
    switch (t) {
        case MKJOT(J_TRUE):                return "J_TRUE";
        case MKJOT(J_FALSE):               return "J_FALSE";
        case MKJOT(J_NULL):                return "J_NULL";
        case MKJOT(J_NUMBER):              return "J_NUMBER";
        case MKJOT(J_STRING):              return "J_STRING";
        case MKJOT(J_OBJECT):              return "J_OBJECT";
        case MKJOT(J_ARRAY):               return "J_ARRAY";
        case MKJOT(J_OBJECT_KV):           return "J_OBJECT_KV";
        default: return "J_UNKNOWN";
    }
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

hvml_jo_value_t* hvml_jo_number(const long double v, const char *origin) {
    hvml_jo_value_t *jo = (hvml_jo_value_t*)calloc(1, sizeof(*jo));
    if (!jo) return NULL;

    jo->jot             = MKJOT(J_NUMBER);
    jo->u.jnum.ldbl     = v;
    jo->u.jnum.origin   = strdup(origin);

    if (!jo->u.jnum.origin) {
        hvml_jo_value_free(jo);
        return NULL;
    }

    return jo;
}

hvml_jo_value_t* hvml_jo_string(const char *v, size_t len) {
    hvml_jo_value_t *jo = (hvml_jo_value_t*)calloc(1, sizeof(*jo));
    if (!jo) return NULL;

    jo->jot        = MKJOT(J_STRING);
    jo->u.jstr.str = (char*)malloc(len+1);
    if (!jo->u.jstr.str) {
        free(jo);
        return NULL;
    }
    memcpy(jo->u.jstr.str, v, len);
    jo->u.jstr.str[len] = '\0';
    jo->u.jstr.len      = len;

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

hvml_jo_value_t* hvml_jo_object_kv(const char *key, size_t len) {
    hvml_jo_value_t *jo = (hvml_jo_value_t*)calloc(1, sizeof(*jo));
    if (!jo) return NULL;

    jo->jot       = MKJOT(J_OBJECT_KV);
    jo->u.jkv.key = (char*)malloc(len+1);
    if (!jo->u.jkv.key) {
        free(jo);
        return NULL;
    }
    memcpy(jo->u.jkv.key, key, len);
    jo->u.jkv.key[len] = '\0';
    jo->u.jkv.len      = len;

    return jo;
}

int hvml_jo_value_push(hvml_jo_value_t *jo, hvml_jo_value_t *val) {
    if (!val) return -1;
    if (!VAL_IS_ORPHAN(val)) {
        E("val[%p/%s] is NOT orphan", (void*)val, hvml_jo_value_type_str(val));
        return -1;
    }

    if (!jo) return 0;

    switch (jo->jot) {
        case MKJOT(J_ARRAY):
        {
            VAL_APPEND(jo, val);
        } break;
        case MKJOT(J_OBJECT):
        {
            if (val->jot != MKJOT(J_OBJECT_KV)) {
                E("val[%p/%s] is NOT object k/v paire", (void*)val, hvml_jo_value_type_str(val));
                return -1;
            }
            VAL_APPEND(jo, val);
        } break;
        case MKJOT(J_OBJECT_KV):
        {
            if (jo->u.jkv.val) {
                A(jo->u.jkv.val != val, "internal logic error");
                hvml_jo_value_free(jo->u.jkv.val);
            }
            jo->u.jkv.val = val;
            VAL_APPEND(jo, val);
        } break;
        default:
        {
            E("jo[%p/%s] can not hold sub-val", (void*)jo, hvml_jo_value_type_str(jo));
            return -1;
        } break;
    }

    return 0;
}

hvml_jo_value_t* hvml_jo_object_get_kv_by_key(hvml_jo_value_t *jo, const char *key, size_t len) {
    A(jo->jot == MKJOT(J_OBJECT), "internal logic error");

    hvml_jo_value_t *kv = VAL_HEAD(jo);
    while (kv) {
        if (kv->u.jkv.len == len && memcmp(kv->u.jkv.key, key, len)==0) break;
        kv = VAL_NEXT(kv);
    }

    if (!kv) return NULL;

    A(kv->jot == MKJOT(J_OBJECT_KV), "internal logic error");
    return kv;
}

hvml_jo_value_t* hvml_jo_object_append_kv(hvml_jo_value_t *jo, hvml_jo_value_t *val) {
    if (jo->jot != MKJOT(J_OBJECT)) {
        E("jo[%p/%s] is NOT object", (void*)jo, hvml_jo_value_type_str(jo));
        return NULL;
    }
    if (!VAL_IS_ORPHAN(val)) {
        E("val[%p/%s] is NOT orphan", (void*)val, hvml_jo_value_type_str(val));
        return NULL;
    }
    if (val->jot != MKJOT(J_OBJECT_KV)) {
        E("val[%p/%s] is NOT object k/v pair", (void*)jo, hvml_jo_value_type_str(val));
        return NULL;
    }

    VAL_APPEND(jo, val);

    return val;
}

void hvml_jo_value_detach(hvml_jo_value_t *jo) {
    hvml_jo_value_t *owner = VAL_OWNER(jo);
    if (!owner) return;

    A(!VAL_IS_ORPHAN(jo), "internal logic error");
    A(!VAL_IS_EMPTY(owner), "internal logic error");

    VAL_REMOVE(jo);

    A(VAL_IS_ORPHAN(jo), "internal logic error");
    A(VAL_OWNER(jo)==NULL, "internal logic error");

    if (owner->jot == MKJOT(J_OBJECT_KV)) {
        owner->u.jkv.val = NULL;
    }
}

void hvml_jo_value_free(hvml_jo_value_t *jo) {
    hvml_jo_value_detach(jo);

    switch (jo->jot) {
        case MKJOT(J_TRUE):
        case MKJOT(J_FALSE):
        case MKJOT(J_NULL):
        case MKJOT(J_NUMBER): {
            if (jo->u.jnum.origin) {
                free(jo->u.jnum.origin);
                jo->u.jnum.origin = NULL;
            }
        } break;
        case MKJOT(J_STRING): {
            free(jo->u.jstr.str);
            jo->u.jstr.str = NULL;
            jo->u.jstr.len = 0;
        } break;
        case MKJOT(J_OBJECT): {
            while (VAL_COUNT(jo)>0) {
                size_t count = VAL_COUNT(jo);
                hvml_jo_value_t *v = VAL_TAIL(jo);
                A(v, "internal logic error");

                hvml_jo_value_free(v);
                A(count - 1 == VAL_COUNT(jo), "internal logic error");
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
            if (jo->u.jkv.key) {
                free(jo->u.jkv.key);
                jo->u.jkv.key = NULL;
                jo->u.jkv.len = 0;
            }
            if (jo->u.jkv.val) {
                hvml_jo_value_free(jo->u.jkv.val);
                jo->u.jkv.val = NULL;
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

    hvml_jo_value_t *val = VAL_OWNER(jo);

    if (val && val->jot == MKJOT(J_OBJECT_KV)) {
        return VAL_OWNER(val);
    }

    return val;
}

hvml_jo_value_t* hvml_jo_value_owner(hvml_jo_value_t *jo) {
    if (jo == NULL) return NULL;

    return VAL_OWNER(jo);
}

hvml_jo_value_t* hvml_jo_value_root(hvml_jo_value_t *jo) {
    if (jo == NULL) return NULL;

    hvml_jo_value_t *val = NULL;

    while (jo && (val=VAL_OWNER(jo))) {
        jo = val;
    }

    return jo;
}

hvml_jo_value_t* hvml_jo_value_child(hvml_jo_value_t *jo) {
    if (jo == NULL) return NULL;

    return VAL_HEAD(jo);
}

hvml_jo_value_t* hvml_jo_value_sibling_next(hvml_jo_value_t *jo) {
    if (jo == NULL) return NULL;

    return VAL_NEXT(jo);
}

hvml_jo_value_t* hvml_jo_value_sibling_prev(hvml_jo_value_t *jo) {
    if (jo == NULL) return NULL;

    return VAL_PREV(jo);
}

int hvml_jo_number_get(hvml_jo_value_t *jo, long double *d, const char **s) {
    if (jo == NULL) return -1;

    if (jo->jot!=MKJOT(J_NUMBER)) return -1;

    if (d) *d = jo->u.jnum.ldbl;

    if (s) *s = jo->u.jnum.origin;

    return 0;
}

int hvml_jo_string_get(hvml_jo_value_t *jo, const char **s) {
    if (jo == NULL) return -1;

    if (jo->jot!=MKJOT(J_STRING)) return -1;

    if (s) *s = jo->u.jstr.str;

    return 0;
}

int hvml_jo_kv_get(hvml_jo_value_t *jo, const char **key, hvml_jo_value_t **val) {
    if (jo == NULL) return -1;

    if (jo->jot!=MKJOT(J_OBJECT_KV)) return -1;

    if (key) *key = jo->u.jkv.key;
    if (val) *val = jo->u.jkv.val;

    return 0;
}


size_t hvml_jo_value_children(hvml_jo_value_t *jo) {
    return VAL_COUNT(jo);
}

typedef struct traverse_s        traverse_t;
struct traverse_s {
    void              *arg;
    jo_traverse_f      cb;
};

static int apply_traverse_callback(hvml_jo_value_t *jo, int lvl, int action, traverse_t *tvs) {
    A(jo, "internal logic error");
    A(lvl>=0, "internal logic error");
    A(tvs, "internal logic error");
    if (!tvs->cb) return 0;

    int breakout = tvs->cb(jo, lvl, action, tvs->arg);
    return breakout;
}

static int do_hvml_jo_value_traverse(hvml_jo_value_t *jo, traverse_t *tvs) {
    int lvl = 0;
    int pop = 0; // MKJOT(xxx) + 1
    int r = 0;
    while (r==0) {
        switch (jo->jot) {
            case MKJOT(J_TRUE):
            case MKJOT(J_FALSE):
            case MKJOT(J_NULL):
            case MKJOT(J_NUMBER):
            case MKJOT(J_STRING): {
                A(pop==0, "internal logic error");
                r = apply_traverse_callback(jo, lvl, 0, tvs);
                if (r) continue;
                // break out switch to traverse sibling and pop
            } break;
            case MKJOT(J_OBJECT): {
                if (pop==0) {
                    r = apply_traverse_callback(jo, lvl, 1, tvs); // push
                    if (r) continue;
                    lvl += 1;
                    hvml_jo_value_t *child = VAL_HEAD(jo);
                    if (child) {
                        A(child->jot == MKJOT(J_OBJECT_KV), "internal logic error");
                        jo  = child;
                        pop = 0;
                        continue;
                    }
                    r = apply_traverse_callback(jo, lvl, -1, tvs); // pop
                    if (r) continue;
                    lvl -= 1;
                    pop  = MKJOT(J_OBJECT) + 1; // pop from obj
                    continue;
                }
                A(pop==(MKJOT(J_OBJECT)+1), "internal logic error");
                pop = 0;
                // break out switch to traverse sibling and pop
            } break;
            case MKJOT(J_OBJECT_KV): {
                if (pop==0) {
                    r = apply_traverse_callback(jo, lvl, 1, tvs); // push
                    if (r) continue;
                    lvl += 1;
                    hvml_jo_value_t *child = VAL_HEAD(jo);
                    if (child) {
                        A(child->jot>=MKJOT(J_TRUE) ||
                          child->jot<MKJOT(J_OBJECT_KV),
                          "internal logic error");
                        jo  = child;
                        pop = 0;
                        continue;
                    }
                    r = apply_traverse_callback(jo, lvl, -1, tvs); // pop
                    if (r) continue;
                    lvl -= 1;
                    pop  = MKJOT(J_OBJECT_KV) + 1; // pop from kv
                    continue;
                }
                A(pop==(MKJOT(J_OBJECT_KV)+1), "internal logic error");
                pop = 0;
                // break out switch to traverse sibling and pop
            } break;
            case MKJOT(J_ARRAY): {
                if (pop==0) {
                    r = apply_traverse_callback(jo, lvl, 1, tvs); // push
                    if (r) continue;
                    lvl += 1;
                    hvml_jo_value_t *child = VAL_HEAD(jo);
                    if (child) {
                        A(child->jot>=MKJOT(J_TRUE) ||
                          child->jot<MKJOT(J_OBJECT_KV),
                          "internal logic error");
                        jo  = child;
                        pop = 0;
                        continue;
                    }
                    r = apply_traverse_callback(jo, lvl, -1, tvs); // pop
                    if (r) continue;
                    lvl -= 1;
                    pop  = MKJOT(J_ARRAY) + 1; // pop from arr
                    continue;
                }
                A(pop==(MKJOT(J_ARRAY)+1), "internal logic error");
                pop = 0;
                // break out switch to traverse sibling and pop
            } break;
            default: {
                A(0, "print json type [%d]: not implemented yet", jo->jot);
            } break;
        }
            A(pop==0, "internal logic error");
            hvml_jo_value_t *sibling = VAL_NEXT(jo);
            if (sibling) {
                A(lvl>0, "internal logic error");
                jo  = sibling;
                continue;
            }
            if (lvl==0) return 0;
            hvml_jo_value_t *parent = VAL_OWNER(jo);
            if (parent) {
                A(lvl>=1, "internal logic error");
                switch (parent->jot) {
                    case MKJOT(J_OBJECT):
                    case MKJOT(J_ARRAY):
                    case MKJOT(J_OBJECT_KV): {
                        r = apply_traverse_callback(parent, lvl, -1, tvs); // pop
                        if (r) continue;
                        lvl -= 1;
                        pop  = parent->jot + 1;
                        jo   = parent;
                        continue;
                    } break;
                    default: {
                        A(0, "internal logic error");
                    } break;
                }
            }
            A(lvl==0, "internal logic error");
            return 0;
    }

    return r ? -1 : 0;
}

int hvml_jo_value_traverse(hvml_jo_value_t *jo, void *arg, jo_traverse_f cb) {
    traverse_t tvs;
    tvs.arg         = arg;
    tvs.cb          = cb;

    return do_hvml_jo_value_traverse(jo, &tvs);
}

typedef struct jo_clone_s            jo_clone_t;
struct jo_clone_s {
    hvml_jo_value_t        *jo;
};

static int traverse_for_clone(hvml_jo_value_t *jo, int lvl, int action, void *arg) {
    (void)lvl;
    jo_clone_t *jc = (jo_clone_t*)arg;
    A(jc, "internal logic error");
    hvml_jo_value_t *v      = NULL;
    HVML_JO_TYPE jot = hvml_jo_value_type(jo);
    switch (jot) {
        case MKJOT(J_TRUE):  {
            v = hvml_jo_true();
            if (!v) return -1;
        } break;
        case MKJOT(J_FALSE): {
            v = hvml_jo_true();
            if (!v) return -1;
        } break;
        case MKJOT(J_NULL): {
            v = hvml_jo_true();
            if (!v) return -1;
        } break;
        case MKJOT(J_NUMBER): {
            A(action==0, "internal logic error");

            long double d;
            const char *s;
            A(0==hvml_jo_number_get(jo, &d, &s), "internal logic error");
            v = hvml_jo_number(d, s);
            if (!v) return -1; // out of memory
        } break;
        case MKJOT(J_STRING): {
            A(action==0, "internal logic error");

            const char *s;
            if (hvml_jo_string_get(jo, &s)) {
                A(0, "internal logic error"); // shouldn't reach here
            }
            v = hvml_jo_string(s, strlen(s));
            if (!v) return -1; // out of memory
        } break;
        case MKJOT(J_OBJECT): {
            switch (action) {
                case 1: {
                    hvml_jo_value_t *v = hvml_jo_object();
                    if (!v) return -1; // out of memory

                    if (hvml_jo_value_push(jc->jo, v)) {
                        hvml_jo_value_free(v);
                        return -1;
                    }
                    jc->jo = v;
                    return 0; // no need to fulfill post-action
                } break;
                case -1: {
                    A(hvml_jo_value_type(jc->jo)==MKJOT(J_OBJECT), "internal logic error");
                    v = VAL_OWNER(jc->jo);
                    if (v) jc->jo = v;
                    return 0; // no need to fulfill post-action
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        case MKJOT(J_OBJECT_KV): {
            switch (action) {
                case 1: {
                    const char      *key;
                    A(0==hvml_jo_kv_get(jo, &key, NULL), "internal logic error");
                    A(key, "internal logic error");
                    hvml_jo_value_t *v = hvml_jo_object_kv(key, strlen(key));
                    if (!v) return -1; // out of memory
                    A(hvml_jo_value_type(jc->jo)==MKJOT(J_OBJECT), "internal logic error");
                    if (hvml_jo_value_push(jc->jo, v)) {
                        hvml_jo_value_free(v);
                        return -1;
                    }
                    jc->jo = v;
                    return 0; // no need to fulfill post-action
                } break;
                case -1: {
                    A(hvml_jo_value_type(jc->jo)==MKJOT(J_OBJECT_KV), "internal logic error");
                    jc->jo = VAL_OWNER(jc->jo);
                    A(hvml_jo_value_type(jc->jo)==MKJOT(J_OBJECT), "internal logic error");
                    return 0; // no need to fulfill post-action
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        case MKJOT(J_ARRAY): {
            switch (action) {
                case 1: {
                    hvml_jo_value_t *v = hvml_jo_array();
                    if (!v) return -1; // out of memory

                    if (hvml_jo_value_push(jc->jo, v)) {
                        hvml_jo_value_free(v);
                        return -1;
                    }
                    jc->jo = v;
                    return 0; // no need to fulfill post-action
                } break;
                case -1: {
                    A(hvml_jo_value_type(jc->jo)==MKJOT(J_ARRAY), "internal logic error");
                    v = jc->jo; // need to fulfill post-action
                    v = VAL_OWNER(jc->jo);
                    if (v) jc->jo = v;
                    return 0; // no need to fulfill post-action
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
    A(v, "internal logic error");
    if (hvml_jo_value_push(jc->jo, v)) {
        hvml_jo_value_free(v);
        return -1;
    }
    hvml_jo_value_t *vp = VAL_OWNER(v);
    jc->jo = vp ? vp : v;
    return 0;
}

hvml_jo_value_t* hvml_jo_clone(hvml_jo_value_t *jo) {
    jo_clone_t arg = {0};
    int r = hvml_jo_value_traverse(jo, &arg, traverse_for_clone);
    if (r) {
        if (arg.jo) {
            free(arg.jo);
            arg.jo = NULL;
        }
        return NULL;
    }
    return hvml_jo_value_root(arg.jo);
}

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
static int on_item_done(void *arg);
static int on_val_done(void *arg);
static int on_end(void *arg);

hvml_jo_gen_t* hvml_jo_gen_create() {
    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)calloc(1, sizeof(*gen));
    if (!gen) return NULL;

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
    conf.on_number              = on_number;
    conf.on_item_done           = on_item_done;
    conf.on_val_done            = on_val_done;
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

    if (jo) hvml_jo_value_free(jo);
    return NULL;
}













static int on_begin(void *arg) {
    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;
    A(gen->jo==NULL, "internal logic error");
    return 0;
}

static int on_open_array(void *arg) {
    hvml_jo_value_t *jo = hvml_jo_array();
    if (!jo) return -1;

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;
    A(gen, "internal logic error");

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    return 0;
}

static int on_close_array(void *arg) {
    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    A(hvml_jo_value_type(gen->jo)==MKJOT(J_ARRAY), "internal logic error");

    return 0;
}

static int on_open_obj(void *arg) {
    hvml_jo_value_t *jo = hvml_jo_object();
    if (!jo) return -1;

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;
    A(gen, "internal logic error");

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    return 0;
}

static int on_close_obj(void *arg) {
    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    A(hvml_jo_value_type(gen->jo)==MKJOT(J_OBJECT), "internal logic error");

    return 0;
}

static int on_key(void *arg, const char *key, size_t len) {
    hvml_jo_gen_t   *gen    = (hvml_jo_gen_t*)arg;
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

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    return 0;
}

static int on_false(void *arg) {
    hvml_jo_value_t *jo = hvml_jo_false();
    if (!jo) return -1;

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    return 0;
}

static int on_null(void *arg) {
    hvml_jo_value_t *jo = hvml_jo_null();
    if (!jo) return -1;

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    return 0;
}

static int on_string(void *arg, const char *val, size_t len) {
    hvml_jo_value_t *jo = hvml_jo_string(val, len);
    if (!jo) return -1;

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    return 0;
}

static int on_number(void *arg, const char *origin, long double val) {
    hvml_jo_value_t *jo = hvml_jo_number(val, origin);
    if (!jo) return -1;

    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    if (hvml_jo_value_push(gen->jo, jo)) {
        hvml_jo_value_free(jo);
        return -1;
    }

    gen->jo = jo;

    return 0;
}

static int on_item_done(void *arg) {
    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    hvml_jo_value_t *parent = hvml_jo_value_parent(gen->jo);
    if (!parent) return 0;

    gen->jo = parent;

    return 0;
}

static int on_val_done(void *arg) {
    hvml_jo_gen_t *gen = (hvml_jo_gen_t*)arg;

    hvml_jo_value_t *parent = hvml_jo_value_parent(gen->jo);
    if (!parent) return 0;

    gen->jo = parent;

    return 0;
}

static int on_end(void *arg) {
    (void)arg;
    return 0;
}

