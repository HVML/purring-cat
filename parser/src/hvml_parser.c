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

#include "hvml/hvml_parser.h"

#include "hvml/hvml_log.h"
#include "hvml/hvml_json_parser.h"

#include <ctype.h>
#include <string.h>

#define MKSTATE(state) HVML_PARSER_STATE_##state
#define MKSTR(state)  "HVML_PARSER_STATE_"#state

#define EPARSE()                                                       \
    E("==%s%c==: unexpected [0x%02x/%c]@[%ldr/%ldc] in state: [%s]",   \
      string_get(&parser->curr), c, c, c,                              \
      parser->line+1, parser->col+1, str_state)

typedef enum {
    MKSTATE(BEGIN),
        MKSTATE(MARKUP),
            MKSTATE(EXCLAMATION),
            MKSTATE(IN_DECL),
            MKSTATE(HVML),
            MKSTATE(STAG),
            MKSTATE(EMPTYTAG),
            MKSTATE(ATTR_OR_END),
            MKSTATE(ATTR),
            MKSTATE(ATTR_DONE),
            MKSTATE(ATTR_VAL),
                MKSTATE(STR),
                MKSTATE(STR1),
                    MKSTATE(ESCAPE),
            MKSTATE(ELEMENT),
            MKSTATE(ETAG),
            MKSTATE(EXP_GREATER),
            MKSTATE(COMMENT),
    MKSTATE(END),
} HVML_PARSER_STATE;

#define IS_NAMESTART(c)  (c==':' || c=='_' || isalpha(c))
#define IS_NAME(c)       (c==':' || c=='_' || c=='-' || c=='.' || isalnum(c))


typedef struct string_s                             string_t;
struct string_s {
    char                  *str;
    size_t                 len;
};
static int         string_append(string_t *str, const char c);
static void        string_reset(string_t *str);
static void        string_clear(string_t *str);
static const char* string_get(string_t *str); // if not initialized, return null string rather than null pointer

struct hvml_parser_s {
    hvml_parser_conf_t             conf;
    HVML_PARSER_STATE             *ar_states;
    size_t                         states;
    string_t                       cache;

    string_t                       curr;

    char                         **ar_tags;
    size_t                         tags;

    unsigned int                   declared:2; // 0:undefined;1:defining;2:defined;3:notexist
    unsigned int                   commenting:1;
    unsigned int                   rooted:1;

    size_t                         line;
    size_t                         col;

    hvml_json_parser_t            *jp;
};

static int               hvml_parser_push_state(hvml_parser_t *parser, HVML_PARSER_STATE state);
static HVML_PARSER_STATE hvml_parser_pop_state(hvml_parser_t *parser);
static HVML_PARSER_STATE hvml_parser_peek_state(hvml_parser_t *parser);
static HVML_PARSER_STATE hvml_parser_chg_state(hvml_parser_t *parser, HVML_PARSER_STATE state);
static void              dump_states(hvml_parser_t *parser);

static int         hvml_parser_push_tag(hvml_parser_t *parser, const char *tag);
static void        hvml_parser_pop_tag(hvml_parser_t *parser);
static const char* hvml_parser_peek_tag(hvml_parser_t *parser);

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

hvml_parser_t* hvml_parser_create(hvml_parser_conf_t conf) {
    hvml_parser_t *parser = (hvml_parser_t*)calloc(1, sizeof(*parser));
    if (!parser) return NULL;

    hvml_json_parser_conf_t jp_conf = {0};
    jp_conf.embedded            = 1;
    jp_conf.arg                 = parser;
    jp_conf.on_begin            = on_begin;
    jp_conf.on_open_array       = on_open_array;
    jp_conf.on_close_array      = on_close_array;
    jp_conf.on_open_obj         = on_open_obj;
    jp_conf.on_close_obj        = on_close_obj;
    jp_conf.on_key              = on_key;
    jp_conf.on_true             = on_true;
    jp_conf.on_false            = on_false;
    jp_conf.on_null             = on_null;
    jp_conf.on_string           = on_string;
    jp_conf.on_integer          = on_integer;
    jp_conf.on_double           = on_double;
    jp_conf.on_end              = on_end;

    parser->jp   = hvml_json_parser_create(jp_conf);
    if (!parser->jp) {
        free(parser);
        return NULL;
    }

    parser->conf = conf;
    hvml_parser_push_state(parser, HVML_PARSER_STATE_BEGIN);

    return parser;
}

void hvml_parser_destroy(hvml_parser_t *parser) {
    string_clear(&parser->cache);
    string_clear(&parser->curr);
    for (size_t i=0; i<parser->tags; ++i) {
        free(parser->ar_tags[i]);
    }
    free(parser->ar_tags);   parser->ar_tags   = NULL;
    free(parser->ar_states); parser->ar_states = NULL;
    hvml_json_parser_destroy(parser->jp); parser->jp = NULL;
    free(parser);
}

static int hvml_parser_at_begin(hvml_parser_t *parser, const char c, const char *str_state) {
    if (isspace(c)) return 0;
    switch (c) {
        case '<':
        {
            hvml_parser_push_state(parser, MKSTATE(MARKUP));
        } break;
        default:
        {
            EPARSE();
            return -1;
        } break;
    }
    return 0;
}

static int hvml_parser_at_markup(hvml_parser_t *parser, const char c, const char *str_state) {
    if (IS_NAMESTART(c)) {
        if (parser->declared==0) parser->declared = 3;
        hvml_parser_pop_state(parser);
        if (parser->tags == 0) {
            hvml_parser_chg_state(parser, MKSTATE(END));
        }
        hvml_parser_push_state(parser, MKSTATE(STAG));
        return 1; // retry
    }
    switch (c) {
        case '!':
            {
                hvml_parser_chg_state(parser, MKSTATE(EXCLAMATION));
            } break;
        case '/':
        {
            if (parser->tags == 0) {
                EPARSE();
                return -1;
            }
            hvml_parser_pop_state(parser);
            if (hvml_parser_peek_state(parser)==MKSTATE(ELEMENT)) {
                hvml_parser_pop_state(parser);
            }
            hvml_parser_push_state(parser, MKSTATE(ETAG));
        } break;
        default:
        {
            EPARSE();
            return -1;
        } break;
    }
    return 0;
}

static int hvml_parser_at_exclamation(hvml_parser_t *parser, const char c, const char *str_state) {
    static const char doctype[] = "DOCTYPE";
    switch (c) {
        case 'D':
        case 'O':
        case 'C':
        case 'T':
        case 'Y':
        case 'P':
        case 'E':
        {
            if (parser->commenting) {
                EPARSE();
                return -1;
            }
            switch (parser->declared) {
                case 0: parser->declared = 1; break;
                case 1: break;
                default:
                {
                    EPARSE();
                    return -1;
                } break;
            }
            if (parser->cache.len>=sizeof(doctype)-1 || doctype[parser->cache.len]!=c) {
                EPARSE();
                return -1;
            }
            string_append(&parser->cache, c);
            if (parser->cache.len<sizeof(doctype)-1) break;
            parser->declared = 2;
            hvml_parser_chg_state(parser, MKSTATE(IN_DECL));
        } break;
        case '-':
        {
            if (parser->declared==1) {
                EPARSE();
                return -1;
            }
            parser->commenting = 1;
            string_append(&parser->cache, c);
            if (parser->cache.len<2) break;
            parser->commenting = 0;
            hvml_parser_chg_state(parser, MKSTATE(COMMENT));
            string_reset(&parser->cache);
        } break;
        default:
        {
            EPARSE();
            return -1;
        } break;
    }
    return 0;
}

static int hvml_parser_at_in_decl(hvml_parser_t *parser, const char c, const char *str_state) {
    if (isspace(c)) return 0;
    switch (c) {
        case 'h':
        {
            hvml_parser_push_state(parser, MKSTATE(HVML));
            string_reset(&parser->cache);
            return 1; // retry
        } break;
        case '>':
        {
            hvml_parser_pop_state(parser);
        } break;
        default:
        {
            EPARSE();
            return -1;
        } break;
    }
    return 0;
}

static int hvml_parser_at_hvml(hvml_parser_t *parser, const char c, const char *str_state) {
    static const char hvml[] = "hvml";
    switch (c) {
        case 'h':
        case 'v':
        case 'm':
        case 'l':
        {
            string_append(&parser->cache, c);
            if (strstr(hvml, string_get(&parser->cache))!=hvml) {
                EPARSE();
                return -1;
            }
            if (parser->cache.len<sizeof(hvml)-1) break;
            hvml_parser_pop_state(parser);
            string_reset(&parser->cache);
        } break;
        default:
        {
            EPARSE();
            return -1;
        } break;
    }
    return 0;
}

static int hvml_parser_at_stag(hvml_parser_t *parser, const char c, const char *str_state) {
    if (IS_NAME(c)) {
        string_append(&parser->cache, c);
        return 0;
    }
    if (isspace(c) || c=='/' || c=='>') {
        int ret = 0;
        if (parser->conf.on_open_tag) {
            ret = parser->conf.on_open_tag(parser->conf.arg, string_get(&parser->cache));
        }
        hvml_parser_push_tag(parser, string_get(&parser->cache));
        string_reset(&parser->cache);
        if (ret) return ret;
    }
    if (isspace(c)) {
        hvml_parser_chg_state(parser, MKSTATE(ATTR_OR_END));
        return 0;
    }
    if (c=='/') {
        hvml_parser_chg_state(parser, MKSTATE(EMPTYTAG));
        return 0;
    }
    if (c=='>') {
        hvml_parser_pop_state(parser);
        hvml_parser_push_state(parser, MKSTATE(ELEMENT));
        string_reset(&parser->cache);
        hvml_json_parser_reset(parser->jp);
        hvml_json_parser_set_offset(parser->jp, parser->line, parser->col + 1);
        return 0;
    }
    switch (c) {
        default: {
                     EPARSE();
                     return -1;
                 } break;
    }
    return 0;
}

static int hvml_parser_at_emptytag(hvml_parser_t *parser, const char c, const char *str_state) {
    switch (c) {
        case '>':
        {
            int ret = 0;
            if (parser->conf.on_close_tag) {
                ret = parser->conf.on_close_tag(parser->conf.arg);
            }
            hvml_parser_pop_state(parser);
            hvml_parser_pop_tag(parser);
            if (ret) return ret;
        } break;
        default:
        {
            EPARSE();
            return -1;
        } break;
    }
    return 0;
}

static int hvml_parser_at_attr_or_end(hvml_parser_t *parser, const char c, const char *str_state) {
    if (isspace(c)) return 0;
    if (IS_NAMESTART(c)) {
        hvml_parser_chg_state(parser, MKSTATE(ATTR));
        return 1; // retry
    }
    if (c=='/') {
        hvml_parser_chg_state(parser, MKSTATE(EMPTYTAG));
        return 0;
    }
    if (c=='>') {
        hvml_parser_pop_state(parser);
        hvml_parser_push_state(parser, MKSTATE(ELEMENT));
        string_reset(&parser->cache);
        hvml_json_parser_reset(parser->jp);
        hvml_json_parser_set_offset(parser->jp, parser->line, parser->col + 1);
        return 0;
    }
    switch (c) {
        default:
        {
            EPARSE();
            return -1;
        } break;
    }
    return 0;
}

static int hvml_parser_at_attr(hvml_parser_t *parser, const char c, const char *str_state) {
    if (IS_NAME(c)) {
        string_append(&parser->cache, c);
        return 0;
    }
    if (isspace(c) || c=='=' || c=='/' || c=='>') {
        int ret = 0;
        if (parser->conf.on_attr_key) {
            ret = parser->conf.on_attr_key(parser->conf.arg, string_get(&parser->cache));
        }
        string_reset(&parser->cache);
        if (ret) return ret;
    }
    if (isspace(c)) {
        hvml_parser_chg_state(parser, MKSTATE(ATTR_DONE));
        return 0;
    }
    if (c=='=') {
        hvml_parser_chg_state(parser, MKSTATE(ATTR_VAL));
        return 0;
    }
    if (c=='/') {
        hvml_parser_chg_state(parser, MKSTATE(EMPTYTAG));
        return 0;
    }
    if (c=='>') {
        hvml_parser_pop_state(parser);
        hvml_parser_push_state(parser, MKSTATE(ELEMENT));
        hvml_json_parser_reset(parser->jp);
        hvml_json_parser_set_offset(parser->jp, parser->line, parser->col + 1);
        return 0;
    }
    switch (c) {
        default: {
                     EPARSE();
                     return -1;
                 } break;
    }
    return 0;
}

static int hvml_parser_at_attr_done(hvml_parser_t *parser, const char c, const char *str_state) {
    if (isspace(c)) return 0;
    if (c=='=') {
        hvml_parser_chg_state(parser, MKSTATE(ATTR_VAL));
        return 0;
    }
    if (c=='/') {
        hvml_parser_chg_state(parser, MKSTATE(EMPTYTAG));
        return 0;
    }
    if (c=='>') {
        hvml_parser_pop_state(parser);
        hvml_parser_push_state(parser, MKSTATE(ELEMENT));
        hvml_json_parser_reset(parser->jp);
        hvml_json_parser_set_offset(parser->jp, parser->line, parser->col + 1);
        return 0;
    }
    if (IS_NAMESTART(c)) {
        hvml_parser_chg_state(parser, MKSTATE(ATTR));
        string_reset(&parser->cache);
        string_append(&parser->cache, c);
        return 0;
    }
    switch (c) {
        default:
        {
            EPARSE();
            return -1;
        } break;
    }
    return 0;
}

static int hvml_parser_at_attr_val(hvml_parser_t *parser, const char c, const char *str_state) {
    if (isspace(c)) return 0;
    switch (c) {
        case '"':
        { // '"'
            hvml_parser_chg_state(parser, MKSTATE(ATTR_OR_END));
            hvml_parser_push_state(parser, MKSTATE(STR));
            string_reset(&parser->cache);
        } break;
        case '\'':
        {
            hvml_parser_chg_state(parser, MKSTATE(ATTR_OR_END));
            hvml_parser_push_state(parser, MKSTATE(STR1));
            string_reset(&parser->cache);
        } break;
        default:
        {
            EPARSE();
            return -1;
        } break;
    }
    return 0;
}

static int hvml_parser_at_str(hvml_parser_t *parser, const char c, const char *str_state) {
    switch (c) {
        case '"':
        { // '"'
            int ret = 0;
            if (parser->conf.on_attr_val) {
                ret = parser->conf.on_attr_val(parser->conf.arg, string_get(&parser->cache));
            }
            string_reset(&parser->cache);
            hvml_parser_pop_state(parser);
            if (ret) return ret;
        } break;
        case '\\':
        {
            hvml_parser_push_state(parser, MKSTATE(ESCAPE));
        } break;
        default:
        {
            string_append(&parser->cache, c);
        } break;
    }
    return 0;
}

static int hvml_parser_at_str1(hvml_parser_t *parser, const char c, const char *str_state) {
    switch (c) {
        case '\'':
        {
            int ret = 0;
            if (parser->conf.on_attr_val) {
                ret = parser->conf.on_attr_val(parser->conf.arg, string_get(&parser->cache));
            }
            string_reset(&parser->cache);
            hvml_parser_pop_state(parser);
            if (ret) return ret;
        } break;
        case '\\':
        {
            hvml_parser_push_state(parser, MKSTATE(ESCAPE));
        } break;
        default:
        {
            string_append(&parser->cache, c);
        } break;
    }
    return 0;
}

static int hvml_parser_at_escape(hvml_parser_t *parser, const char c, const char *str_state) {
    switch (c) {
        case 'b':
        {
            string_append(&parser->cache, '\b');
            hvml_parser_pop_state(parser);
        } break;
        case 't':
        {
            string_append(&parser->cache, '\t');
            hvml_parser_pop_state(parser);
        } break;
        case 'f':
        {
            string_append(&parser->cache, '\f');
            hvml_parser_pop_state(parser);
        } break;
        case 'r':
        {
            string_append(&parser->cache, '\r');
            hvml_parser_pop_state(parser);
        } break;
        case 'n':
        {
            string_append(&parser->cache, '\n');
            hvml_parser_pop_state(parser);
        } break;
        case '\\':
        {
            string_append(&parser->cache, '\\');
            hvml_parser_pop_state(parser);
        } break;
        case '\'':
        {
            string_append(&parser->cache, '\'');
            hvml_parser_pop_state(parser);
        } break;
        case '"':
        { // '"'
            string_append(&parser->cache, '"');
            hvml_parser_pop_state(parser);
        } break;
        default:
        {
            EPARSE();
            return -1;
        } break;
    }
    return 0;
}

static int hvml_parser_at_element(hvml_parser_t *parser, const char c, const char *str_state) {
    const char* tag = hvml_parser_peek_tag(parser);
    const int parse_json = (strcmp(tag,"init")==0) || (strcmp(tag, "archedata")==0);
    if (parse_json) {
        int ret = hvml_json_parser_parse_char(parser->jp, c);
        if (ret==0) return 0;
        if (!hvml_json_parser_is_begin(parser->jp) &&
                !hvml_json_parser_is_ending(parser->jp) )
        {
            // error message has already been printf'd in hvml_json_parser module
            return -1;
        }
    }
    switch (c) {
        case '<':
        {
            if (!parse_json) {
                int ret = 0;
                if (parser->cache.len>0) {
                    if (parser->conf.on_text) {
                        ret = parser->conf.on_text(parser->conf.arg, string_get(&parser->cache));
                    }
                    string_reset(&parser->cache);
                }
                if (ret) return ret;
                hvml_parser_push_state(parser, MKSTATE(MARKUP));
            } else {
                hvml_json_parser_reset(parser->jp);
                hvml_parser_push_state(parser, MKSTATE(MARKUP));
            }
        } break;
        default:
        {
            if (!parse_json) {
                string_append(&parser->cache, c);
            } else {
                EPARSE();
                return -1;
            }
        } break;
    }
    return 0;
}

static int hvml_parser_at_etag(hvml_parser_t *parser, const char c, const char *str_state) {
    if (IS_NAME(c)) {
        string_append(&parser->cache, c);
        return 0;
    }
    if (isspace(c) || c=='>') {
        const char *stag = hvml_parser_peek_tag(parser);
        if (strcmp(stag, string_get(&parser->cache))) {
            EPARSE();
            return -1;
        }
        int ret = 0;
        if (parser->conf.on_close_tag) {
            ret = parser->conf.on_close_tag(parser->conf.arg);
        }
        string_reset(&parser->cache);
        hvml_parser_pop_tag(parser);
        if (ret) return ret;
    }
    if (isspace(c)) {
        hvml_parser_chg_state(parser, MKSTATE(EXP_GREATER));
        return 0;
    }
    if (c=='>') {
        hvml_parser_pop_state(parser);
        return 0;
    }
    switch (c) {
        default: {
                     EPARSE();
                     return -1;
                 } break;
    }
    return 0;
}

static int hvml_parser_at_exp_greater(hvml_parser_t *parser, const char c, const char *str_state) {
    if (isspace(c)) return 0;
    switch (c) {
        case '>':
        {
            hvml_parser_pop_state(parser);
        } break;
        default:
        {
            EPARSE();
            return -1;
        } break;
    }
    return 0;
}

static int hvml_parser_at_comment(hvml_parser_t *parser, const char c, const char *str_state) {
    switch (c) {
        case '>':
        {
            if (parser->cache.len >= 2 && parser->cache.str[parser->cache.len-1]=='-' && parser->cache.str[parser->cache.len-2]=='-') {
                hvml_parser_pop_state(parser);
                string_reset(&parser->cache);
                break;
            }
            // fall throught
        }
        default:
        {
            string_append(&parser->cache, c);
        } break;
    }
    return 0;
}

static int hvml_parser_at_end(hvml_parser_t *parser, const char c, const char *str_state) {
    if (isspace(c)) return 0;
    switch (c) {
        default:
        {
            EPARSE();
            return -1;
        } break;
    }
    return 0;
}

static int do_hvml_parser_parse_char(hvml_parser_t *parser, const char c) {
    HVML_PARSER_STATE state = hvml_parser_peek_state(parser);
    switch (state) {
        case MKSTATE(BEGIN):
        {
            return hvml_parser_at_begin(parser, c, MKSTR(BEGIN));
        } break;
        case MKSTATE(MARKUP):
        {
            return hvml_parser_at_markup(parser, c, MKSTR(MARKUP));
        } break;
        case MKSTATE(EXCLAMATION):
        {
            return hvml_parser_at_exclamation(parser, c, MKSTR(EXCLAMATION));
        } break;
        case MKSTATE(IN_DECL):
        {
            return hvml_parser_at_in_decl(parser, c, MKSTR(IN_DECL));
        } break;
        case MKSTATE(HVML):
        {
            return hvml_parser_at_hvml(parser, c, MKSTR(HVML));
        } break;
        case MKSTATE(STAG):
        {
            return hvml_parser_at_stag(parser, c, MKSTR(STAG));
        } break;
        case MKSTATE(EMPTYTAG):
        {
            return hvml_parser_at_emptytag(parser, c, MKSTR(EMPTYTAG));
        } break;
        case MKSTATE(ATTR_OR_END):
        {
            return hvml_parser_at_attr_or_end(parser, c, MKSTR(ATTR_OR_END));
        } break;
        case MKSTATE(ATTR):
        {
            return hvml_parser_at_attr(parser, c, MKSTR(ATTR));
        } break;
        case MKSTATE(ATTR_DONE):
        {
            return hvml_parser_at_attr_done(parser, c, MKSTR(ATTR_DONE));
        } break;
        case MKSTATE(ATTR_VAL):
        {
            return hvml_parser_at_attr_val(parser, c, MKSTR(ATTR_VAL));
        } break;
        case MKSTATE(STR):
        {
            return hvml_parser_at_str(parser, c, MKSTR(STR));
        } break;
        case MKSTATE(STR1):
        {
            return hvml_parser_at_str1(parser, c, MKSTR(STR1));
        } break;
        case MKSTATE(ESCAPE):
        {
            return hvml_parser_at_escape(parser, c, MKSTR(ESCAPE));
        } break;
        case MKSTATE(ELEMENT):
        {
            return hvml_parser_at_element(parser, c, MKSTR(ELEMENT));
        } break;
        case MKSTATE(ETAG):
        {
            return hvml_parser_at_etag(parser, c, MKSTR(ETAG));
        } break;
        case MKSTATE(EXP_GREATER):
        {
            return hvml_parser_at_exp_greater(parser, c, MKSTR(EXP_GREATER));
        } break;
        case MKSTATE(COMMENT):
        {
            return hvml_parser_at_comment(parser, c, MKSTR(COMMENT));
        } break;
        case MKSTATE(END):
        {
            return hvml_parser_at_end(parser, c, MKSTR(END));
        } break;
        default:
        {
            E("not implemented for state: [%d]; curr: %s", state, parser->curr.str);
            return -1;
        } break;
    }
    return 0;
}

int hvml_parser_parse_char(hvml_parser_t *parser, const char c) {
    int ret = 1;
    do {
        ret = do_hvml_parser_parse_char(parser, c);
    } while (ret==1); // ret==1: to retry
    if (ret==0) {
        if (c=='\n') {
            string_reset(&parser->curr);
            ++parser->line;
            parser->col = 0;
        } else {
            string_append(&parser->curr, c);
            ++parser->col;
        }
    }
    return ret;
}

int hvml_parser_parse(hvml_parser_t *parser, const char *buf, size_t len) {
    for (size_t i=0; i<len; ++i) {
        int ret = hvml_parser_parse_char(parser, buf[i]);
        if (ret) return ret;
    }
    return 0;
}

int hvml_parser_parse_string(hvml_parser_t *parser, const char *str) {
    return hvml_parser_parse(parser, (const char *)str, strlen(str));
}

int hvml_parser_parse_end(hvml_parser_t *parser) {
    if (parser->tags != 0) {
        E("open tag [%s] not closed", parser->ar_tags[parser->tags - 1]);
        return -1;
    }
    if (parser->states != 1) {
        D("states [%ld/%d] too high", parser->states, parser->ar_states[parser->states - 1]);
        return -1;
    }
    if (hvml_parser_peek_state(parser)!=MKSTATE(END) && hvml_parser_peek_state(parser)!=MKSTATE(BEGIN)) {
        A(0, "internal logic error");
        return -1;
    }
    return 0;
}






static int string_append(string_t *str, const char c) {
    // one extra null-terminator
    // actually, won't realloc new mem everytime
    char *s = (char*)realloc(str->str, (str->len + 2) * sizeof(*s));
    if (!s) return -1;
    s[str->len]   = c;
    s[str->len+1] = '\0';
    str->str      = s;
    str->len     += 1;
    return 0;
}

static void string_reset(string_t *str) {
    if (str->len==0) return;
    str->str[0] = '\0';
    str->len    = 0;
}

static void string_clear(string_t *str) {
    free(str->str);
    str->str = NULL;
    str->len = 0;
}

static const char* string_get(string_t *str) {
    if (str->len==0) return "";
    return str->str;
}

static int hvml_parser_push_state(hvml_parser_t *parser, HVML_PARSER_STATE state) {
    HVML_PARSER_STATE *st = (HVML_PARSER_STATE*)realloc(parser->ar_states, (parser->states + 1) * sizeof(*st));
    if (!st) return -1;

    st[parser->states] = state;
    parser->states    += 1;
    parser->ar_states  = st;

    return 0;
}

static HVML_PARSER_STATE hvml_parser_pop_state(hvml_parser_t *parser) {
    A(parser->states>1, "parser's internal ar_states stack not initialized or underflowed or would be underflowed");

    HVML_PARSER_STATE st  = parser->ar_states[parser->states - 1];
    parser->states       -= 1;

    return st;
}

static HVML_PARSER_STATE hvml_parser_peek_state(hvml_parser_t *parser) {
    A(parser->states>0, "parser's internal ar_states stack not initialized or underflowed");

    HVML_PARSER_STATE st = parser->ar_states[parser->states - 1];

    return st;
}

static HVML_PARSER_STATE hvml_parser_chg_state(hvml_parser_t *parser, HVML_PARSER_STATE state) {
    A(parser->states>0, "parser's internal ar_states stack not initialized or underflowed");

    HVML_PARSER_STATE st                  = parser->ar_states[parser->states - 1];
    parser->ar_states[parser->states - 1] = state;

    return st;
}

static void dump_states(hvml_parser_t *parser) {
    D("states:");
    for (size_t i=0; i<parser->states; ++i) {
        D("state: %ld, %d", i, parser->ar_states[i]);
    }
    D("==");
}


static int hvml_parser_push_tag(hvml_parser_t *parser, const char *tag) {
    char *s   = strdup(tag);
    if (!s)  return -1;
    char **ar = (char**)realloc(parser->ar_tags, (parser->tags + 1) * sizeof(*ar));
    if (!ar) {
        free(s);
        return -1;
    }

    ar[parser->tags] = s;
    parser->tags    += 1;
    parser->ar_tags  = ar;

    return 0;
}

static void hvml_parser_pop_tag(hvml_parser_t *parser) {
    A(parser->tags>0, "parser's internal ar_tags stack not initialized or underflowed");

    char *tag      = parser->ar_tags[parser->tags - 1];
    parser->tags  -= 1;
    free(tag);
}

static const char* hvml_parser_peek_tag(hvml_parser_t *parser) {
    A(parser->tags>0, "parser's internal ar_tags stack not initialized or underflowed");

    char *tag      = parser->ar_tags[parser->tags - 1];

    return tag;
}

// json callbacks
static int on_begin(void *arg) {
    hvml_parser_t *parser = (hvml_parser_t*)arg;
    int ret = 0;
    if (parser->conf.on_begin) {
        ret = parser->conf.on_begin(parser->conf.arg);
    }
    return ret;
}

static int on_open_array(void *arg) {
    hvml_parser_t *parser = (hvml_parser_t*)arg;
    int ret = 0;
    if (parser->conf.on_open_array) {
        ret = parser->conf.on_open_array(parser->conf.arg);
    }
    return ret;
}

static int on_close_array(void *arg) {
    hvml_parser_t *parser = (hvml_parser_t*)arg;
    int ret = 0;
    if (parser->conf.on_close_array) {
        ret = parser->conf.on_close_array(parser->conf.arg);
    }
    return ret;
}

static int on_open_obj(void *arg) {
    hvml_parser_t *parser = (hvml_parser_t*)arg;
    int ret = 0;
    if (parser->conf.on_open_obj) {
        ret = parser->conf.on_open_obj(parser->conf.arg);
    }
    return ret;
}

static int on_close_obj(void *arg) {
    hvml_parser_t *parser = (hvml_parser_t*)arg;
    int ret = 0;
    if (parser->conf.on_close_obj) {
        ret = parser->conf.on_close_obj(parser->conf.arg);
    }
    return ret;
}

static int on_key(void *arg, const char *key) {
    hvml_parser_t *parser = (hvml_parser_t*)arg;
    int ret = 0;
    if (parser->conf.on_key) {
        ret = parser->conf.on_key(parser->conf.arg, key);
    }
    return ret;
}

static int on_true(void *arg) {
    hvml_parser_t *parser = (hvml_parser_t*)arg;
    int ret = 0;
    if (parser->conf.on_true) {
        ret = parser->conf.on_true(parser->conf.arg);
    }
    return ret;
}

static int on_false(void *arg) {
    hvml_parser_t *parser = (hvml_parser_t*)arg;
    int ret = 0;
    if (parser->conf.on_false) {
        ret = parser->conf.on_false(parser->conf.arg);
    }
    return ret;
}

static int on_null(void *arg) {
    hvml_parser_t *parser = (hvml_parser_t*)arg;
    int ret = 0;
    if (parser->conf.on_null) {
        ret = parser->conf.on_null(parser->conf.arg);
    }
    return ret;
}

static int on_string(void *arg, const char *val) {
    hvml_parser_t *parser = (hvml_parser_t*)arg;
    int ret = 0;
    if (parser->conf.on_string) {
        ret = parser->conf.on_string(parser->conf.arg, val);
    }
    return ret;
}

static int on_integer(void *arg, const char *origin, int64_t val) {
    hvml_parser_t *parser = (hvml_parser_t*)arg;
    int ret = 0;
    if (parser->conf.on_integer) {
        ret = parser->conf.on_integer(parser->conf.arg, origin, val);
    }
    return ret;
}

static int on_double(void *arg, const char *origin, double val) {
    hvml_parser_t *parser = (hvml_parser_t*)arg;
    int ret = 0;
    if (parser->conf.on_double) {
        ret = parser->conf.on_double(parser->conf.arg, origin, val);
    }
    return ret;
}

static int on_end(void *arg) {
    hvml_parser_t *parser = (hvml_parser_t*)arg;
    int ret = 0;
    if (parser->conf.on_end) {
        ret = parser->conf.on_end(parser->conf.arg);
    }
    return ret;
}

