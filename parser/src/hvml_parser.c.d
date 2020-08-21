#include "hvml_parser.h"

#include "hvml_log.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  HVML_PARSER_STATE_BEGIN,
  HVML_PARSER_STATE_MARKUP,
  HVML_PARSER_STATE_MARKUP_EXCLAMATION,
  HVML_PARSER_STATE_BEGIN_DECL,
  HVML_PARSER_STATE_BEGIN_DECL_TAG_DONE,
  HVML_PARSER_STATE_BEGIN_DECL_DOCTYPE_HVML,
  HVML_PARSER_STATE_BEGIN_DECL_DOCTYPE_HVML_DONE,
  HVML_PARSER_STATE_DECL_DONE,
  HVML_PARSER_STATE_BEGIN_TAG,
  HVML_PARSER_STATE_IN_OPEN_TAG,
  HVML_PARSER_STATE_ATTR_KEY,
  HVML_PARSER_STATE_EQ,
  HVML_PARSER_STATE_ATTR_VAL,
  HVML_PARSER_STATE_STR,
  HVML_PARSER_STATE_TAG_OPENED,
  HVML_PARSER_STATE_TEXT,
  HVML_PARSER_STATE_LESS,
  HVML_PARSER_STATE_TAG,
  HVML_PARSER_STATE_SPACES,
  HVML_PARSER_STATE_END_TAG,
  HVML_PARSER_STATE_END
} HVML_PARSER_STATE;

#define MKSTR(state) #state

typedef struct string_s                             string_t;
struct string_s {
  char                  *str;
  size_t                 len;
};
static int  string_append(string_t *str, const char c);
static void string_reset(string_t *str);
static void string_clear(string_t *str);

struct hvml_parser_s {
  hvml_parser_conf_t             conf;
  HVML_PARSER_STATE             *ar_states;
  size_t                         states;
  string_t                       cache;

  string_t                       curr;

  char                         **ar_tags;
  size_t                         tags;

  int                            declared:2; // 0:undefined;1:defined;2:notdefined
};

static int               hvml_parser_push_state(hvml_parser_t *parser, HVML_PARSER_STATE state);
static HVML_PARSER_STATE hvml_parser_pop_state(hvml_parser_t *parser);
static HVML_PARSER_STATE hvml_parser_peek_state(hvml_parser_t *parser);
static HVML_PARSER_STATE hvml_parser_chg_state(hvml_parser_t *parser, HVML_PARSER_STATE state);

static int         hvml_parser_push_tag(hvml_parser_t *parser, const char *tag);
static char*       hvml_parser_pop_tag(hvml_parser_t *parser);
static const char* hvml_parser_peek_tag(hvml_parser_t *parser);

hvml_parser_t* hvml_parser_create(hvml_parser_conf_t conf) {
  hvml_parser_t *parser = (hvml_parser_t*)calloc(1, sizeof(*parser));
  if (!parser) return NULL;

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
  free(parser);
}

static int hvml_parser_parse_begin(hvml_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    case '<': {
      hvml_parser_chg_state(parser, HVML_PARSER_STATE_END);
      hvml_parser_push_state(parser, HVML_PARSER_STATE_MARKUP);
    } break;
    default: {
      E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
      return -1;
    } break;
  }
  return 0;
}

static int hvml_parser_parse_markup(hvml_parser_t *parser, const char c, const char *str_state) {
  if (isalpha(c) || c==':' || c=='_') {
    hvml_parser_push_state(parser, HVML_PARSER_STATE_IN_MARKUP);
    hvml_parser_push_state(parser, HVML_PARSER_STATE_TAG);
    string_reset(&parser->cache);
    string_append(&parser->cache, c);
    return 0;
  }
  switch (c) {
    case '/': {
      hvml_parser_push_state(parser, HVML_PARSER_STATE_END_MARKUP);
      hvml_parser_push_state(parser, HVML_PARSER_STATE_TAG);
      string_reset(&parser->cache);
    } break;
    case '!': {
      hvml_parser_push_state(parser, HVML_PARSER_STATE_MARKUP_EXCLAMATION);
      string_reset(&parser->cache);
    } break;
    default: {
      E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
      return -1;
    } break;
  }
  return 0;
}

static int hvml_parser_parse_markup_exclamation(hvml_parser_t *parser, const char c, const char *str_state) {
  if (isalpha(c) || c==':' || c=='_') {
    hvml_parser_push_state(parser, HVML_PARSER_STATE_IN_MARKUP);
    hvml_parser_push_state(parser, HVML_PARSER_STATE_TAG);
    string_reset(&parser->cache);
    string_append(&parser->cache, c);
    return 0;
  }
  switch (c) {
    case '/': {
      hvml_parser_push_state(parser, HVML_PARSER_STATE_END_MARKUP);
      hvml_parser_push_state(parser, HVML_PARSER_STATE_TAG);
      string_reset(&parser->cache);
    } break;
    case '!': {
      hvml_parser_push_state(parser, HVML_PARSER_STATE_MARKUP_EXCLAMATION);
      string_reset(&parser->cache);
    } break;
    default: {
      E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
      return -1;
    } break;
  }
  return 0;
}

static int hvml_parser_parse_begin_decl(hvml_parser_t *parser, const char c, const char *str_state) {
  static const char doctype[] = "DOCTYPE";
  switch (c) {
    case 'D':
    case 'O':
    case 'C':
    case 'T':
    case 'Y':
    case 'P':
    case 'E': {
      string_append(&parser->cache, c);
      if (strstr(doctype, parser->cache.str)!=doctype) {
        E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
        return -1;
      }
      if (parser->cache.len < sizeof(doctype)-1) break;
    } break;
    default: {
      if (isspace(c)) {
        if (strcmp(doctype, parser->cache.str)==0) {
          hvml_parser_chg_state(parser, HVML_PARSER_STATE_BEGIN_DECL_TAG_DONE);
          break;
        }
        E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
        return -1;
      }
      E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
      return -1;
    } break;
  }
  return 0;
}

static int hvml_parser_parse_begin_decl_tag_done(hvml_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    case 'h': {
      hvml_parser_chg_state(parser, HVML_PARSER_STATE_BEGIN_DECL_DOCTYPE_HVML);
      string_reset(&parser->cache);
      string_append(&parser->cache, c);
    } break;
    default: {
      E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
      return -1;
    } break;
  }
  return 0;
}

static int hvml_parser_parse_begin_decl_doctype_hvml(hvml_parser_t *parser, const char c, const char *str_state) {
  static const char hvml[] = "hvml";
  switch (c) {
    case 'v':
    case 'm':
    case 'l': {
      string_append(&parser->cache, c);
      if (strstr(hvml, parser->cache.str)!=hvml) {
        E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
        return -1;
      }
      if (parser->cache.len < sizeof(hvml)-1) break;
      hvml_parser_chg_state(parser, HVML_PARSER_STATE_BEGIN_DECL_DOCTYPE_HVML_DONE);
    } break;
    default: {
      E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
      return -1;
    } break;
  }
  return 0;
}

static int hvml_parser_parse_begin_decl_doctype_hvml_done(hvml_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    case '>': {
      hvml_parser_chg_state(parser, HVML_PARSER_STATE_DECL_DONE);
    } break;
    default: {
      E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
      return -1;
    } break;
  }
  return 0;
}

static int hvml_parser_parse_decl_done(hvml_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    case '<': {
      hvml_parser_chg_state(parser, HVML_PARSER_STATE_END);
      hvml_parser_push_state(parser, HVML_PARSER_STATE_BEGIN_TAG);
      string_reset(&parser->cache);
    } break;
    default: {
      E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
      return -1;
    } break;
  }
  return 0;
}

static int hvml_parser_parse_begin_tag(hvml_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) {
    if (parser->cache.len==0) return 0;
    if (parser->conf.on_open_tag) {
      int ret = parser->conf.on_open_tag(parser, parser->cache.str);
      if (ret) return -1;
    }
    hvml_parser_push_tag(parser, parser->cache.str);
    hvml_parser_pop_state(parser);
    hvml_parser_push_state(parser, HVML_PARSER_STATE_IN_OPEN_TAG);
    string_reset(&parser->cache);
    return 0;
  }
  if (c=='>') {
    if (parser->cache.len==0) {
      E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
      return -1;
    }
    if (parser->conf.on_open_tag) {
      int ret = parser->conf.on_open_tag(parser, parser->cache.str);
      if (ret) return -1;
    }
    hvml_parser_push_tag(parser, parser->cache.str);
    hvml_parser_chg_state(parser, HVML_PARSER_STATE_TAG_OPENED);
    string_reset(&parser->cache);
    return 0;
  }
  int valid = 0;
  if (parser->cache.len==0) {
    if (isalpha(c) || c=='_') valid = 1;
  } else {
    if (isalnum(c) || c=='_') valid = 1;
  }
  if (!valid) {
    E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
    return -1;
  }
  string_append(&parser->cache, c);
  return 0;
}

static int hvml_parser_parse_in_open_tag(hvml_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  if (isalpha(c)) {
    hvml_parser_chg_state(parser, HVML_PARSER_STATE_ATTR_KEY);
    string_reset(&parser->cache);
    string_append(&parser->cache, c);
    return 0;
  }
  switch (c) {
    case '>': {
      hvml_parser_chg_state(parser, HVML_PARSER_STATE_TAG_OPENED);
      string_reset(&parser->cache);
    } break;
    default: {
      E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
      return -1;
    } break;
  }
  return 0;
}

static int hvml_parser_parse_attr_key(hvml_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c) || c=='=') {
    if (parser->cache.len==0) return 0;
    if (parser->conf.on_attr_key) {
      int ret = parser->conf.on_attr_key(parser, parser->cache.str);
      if (ret) return -1;
    }
    if (c=='=') {
      hvml_parser_chg_state(parser, HVML_PARSER_STATE_ATTR_VAL);
    } else {
      hvml_parser_chg_state(parser, HVML_PARSER_STATE_EQ);
    }
    return 0;
  }
  int valid = 0;
  if (parser->cache.len==0) {
    if (isalpha(c) || c=='_') valid = 1;
  } else {
    if (isalnum(c) || c=='_') valid = 1;
  }
  if (!valid) {
    E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
    return -1;
  }
  string_append(&parser->cache, c);
  return 0;
}

static int hvml_parser_parse_attr_val(hvml_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    case '"': { // '"'
      hvml_parser_chg_state(parser, HVML_PARSER_STATE_IN_OPEN_TAG);
      hvml_parser_push_state(parser, HVML_PARSER_STATE_STR);
      string_reset(&parser->cache);
    } break;
    default: {
      E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
      return -1;
    } break;
  }
  return 0;
}

static int hvml_parser_parse_str(hvml_parser_t *parser, const char c, const char *str_state) {
  switch (c) {
    case '"': { // '"'
      if (parser->conf.on_attr_val) {
        int ret = parser->conf.on_attr_val(parser, parser->cache.str);
        if (ret) return -1;
      }
      hvml_parser_pop_state(parser);
      string_reset(&parser->cache);
    } break;
    default: {
      string_append(&parser->cache, c);
    } break;
  }
  return 0;
}

static int hvml_parser_parse_tag_opened(hvml_parser_t *parser, const char c, const char *str_state) {
  switch (c) {
    case '<': {
      E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
      return -1;
    } break;
    default: {
      hvml_parser_push_state(parser, HVML_PARSER_STATE_TEXT);
      string_reset(&parser->cache);
      string_append(&parser->cache, c);
    } break;
  }
  return 0;
}

static int hvml_parser_parse_text(hvml_parser_t *parser, const char c, const char *str_state) {
  switch (c) {
    case '<': {
      if (parser->conf.on_text) {
        int ret = parser->conf.on_text(parser, parser->cache.str);
        if (ret) return -1;
      }
      hvml_parser_chg_state(parser, HVML_PARSER_STATE_LESS);
      string_reset(&parser->cache);
    } break;
    default: {
      string_append(&parser->cache, c);
    } break;
  }
  return 0;
}

static int hvml_parser_parse_less(hvml_parser_t *parser, const char c, const char *str_state) {
  if (isalpha(c) || c=='_') {
    hvml_parser_chg_state(parser, HVML_PARSER_STATE_TAG_OPENED);
    hvml_parser_push_state(parser, HVML_PARSER_STATE_BEGIN_TAG);
    string_reset(&parser->cache);
    string_append(&parser->cache, c);
    return 0;
  }
  switch (c) {
    case '/': {
      hvml_parse_chg_state(parser, HVML_PARSER_STATE_TAG);
      string_reset(&parser->cache);
    } break;
    default: {
      E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
      return -1;
    } break;
  }
  return 0;
}

static int hvml_parser_parse_tag(hvml_parser_t *parser, const char c, const char *str_state) {
  if (isalpha(c) || c=='_') {
    hvml_parser_chg_state(parser, HVML_PARSER_STATE_TAG_OPENED);
    hvml_parser_push_state(parser, HVML_PARSER_STATE_BEGIN_TAG);
    string_reset(&parser->cache);
    string_append(&parser->cache, c);
    return 0;
  }
  switch (c) {
    case '/': {
      hvml_parse_chg_state(parser, HVML_PARSER_STATE_TAG);
      string_reset(&parser->cache);
    } break;
    default: {
      E("==%s%c==: unexpected [0x%02x/%c] in state: [%s]", parser->curr.str, c, c, c, str_state);
      return -1;
    } break;
  }
  return 0;
}

static int hvml_parser_parse_spaces(hvml_parser_t *parser, const char c, const char *str_state) {
  HVML_PARSER_STATE state = hvml_parser_peek_state(parser);
  if (isspace(c)) return 0;
  return 0;
}

static int do_hvml_parser_parse_char(hvml_parser_t *parser, const char c) {
  HVML_PARSER_STATE state = hvml_parser_peek_state(parser);
  switch (state) {
    case HVML_PARSER_STATE_BEGIN: {
      return hvml_parser_parse_begin(parser, c, MKSTR(HVML_PARSER_STATE_BEGIN));
    } break;
    case HVML_PARSER_STATE_MARKUP: {
      return hvml_parser_parse_markup(parser, c, MKSTR(HVML_PARSER_STATE_MARKUP));
    } break;
    case HVML_PARSER_STATE_MARKUP_EXCLAMATION: {
      return hvml_parser_parse_markup_exclamation(parser, c, MKSTR(HVML_PARSER_STATE_EXCLAMATION));
    } break;
    case HVML_PARSER_STATE_BEGIN_DECL: {
      return hvml_parser_parse_begin_decl(parser, c, MKSTR(HVML_PARSER_STATE_BEGIN_DECL));
    } break;
    case HVML_PARSER_STATE_BEGIN_DECL_TAG_DONE: {
      return hvml_parser_parse_begin_decl_tag_done(parser, c, MKSTR(HVML_PARSER_STATE_BEGIN_DECL_TAG_DONE));
    } break;
    case HVML_PARSER_STATE_BEGIN_DECL_DOCTYPE_HVML: {
      return hvml_parser_parse_begin_decl_doctype_hvml(parser, c, MKSTR(HVML_PARSER_STATE_BEGIN_DECL_DOCTYPE_HVML));
    } break;
    case HVML_PARSER_STATE_BEGIN_DECL_DOCTYPE_HVML_DONE: {
      return hvml_parser_parse_begin_decl_doctype_hvml_done(parser, c, MKSTR(HVML_PARSER_STATE_BEGIN_DECL_DOCTYPE_HVML_DONE));
    } break;
    case HVML_PARSER_STATE_DECL_DONE: {
      return hvml_parser_parse_decl_done(parser, c, MKSTR(HVML_PARSER_STATE_DECL_DONE));
    } break;
    case HVML_PARSER_STATE_BEGIN_TAG: {
      return hvml_parser_parse_begin_tag(parser, c, MKSTR(HVML_PARSER_STATE_BEGIN_TAG));
    } break;
    case HVML_PARSER_STATE_IN_OPEN_TAG: {
      return hvml_parser_parse_in_open_tag(parser, c, MKSTR(HVML_PARSER_STATE_IN_OPEN_TAG));
    } break;
    case HVML_PARSER_STATE_ATTR_KEY: {
      return hvml_parser_parse_attr_key(parser, c, MKSTR(HVML_PARSER_STATE_ATTR_KEY));
    } break;
    case HVML_PARSER_STATE_ATTR_VAL: {
      return hvml_parser_parse_attr_val(parser, c, MKSTR(HVML_PARSER_STATE_ATTR_VAL));
    } break;
    case HVML_PARSER_STATE_STR: {
      return hvml_parser_parse_str(parser, c, MKSTR(HVML_PARSER_STATE_STR));
    } break;
    case HVML_PARSER_STATE_TAG_OPENED: {
      return hvml_parser_parse_tag_opened(parser, c, MKSTR(HVML_PARSER_STATE_TAG_OPENED));
    } break;
    case HVML_PARSER_STATE_TEXT: {
      return hvml_parser_parse_text(parser, c, MKSTR(HVML_PARSER_STATE_TEXT));
    } break;
    case HVML_PARSER_STATE_LESS: {
      return hvml_parser_parse_less(parser, c, MKSTR(HVML_PARSER_STATE_LESS));
    } break;
    case HVML_PARSER_STATE_TAG: {
      return hvml_parser_parse_tag(parser, c, MKSTR(HVML_PARSER_STATE_TAG));
    } break;
    case HVML_PARSER_STATE_SPACES: {
      return hvml_parser_parse_spaces(parser, c, MKSTR(HVML_PARSER_STATE_SPACES));
    } break;
    default: {
      E("not implemented for state: [%d]; curr: %s", state, parser->curr.str);
      return -1;
    } break;
  }
  return 0;
}

int hvml_parser_parse_char(hvml_parser_t *parser, const char c) {
  int ret = do_hvml_parser_parse_char(parser, c);
  if (ret==0) {
    if (c=='\n') {
      string_reset(&parser->curr);
    } else {
      string_append(&parser->curr, c);
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
  return -1;
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

static int hvml_parser_push_state(hvml_parser_t *parser, HVML_PARSER_STATE state) {
  HVML_PARSER_STATE *st = (HVML_PARSER_STATE*)realloc(parser->ar_states, (parser->states + 1) * sizeof(*st));
  if (!st) return -1; 

  st[parser->states] = state;
  parser->states    += 1;
  parser->ar_states  = st;

  return 0;
}

static HVML_PARSER_STATE hvml_parser_pop_state(hvml_parser_t *parser) {
  A(parser->states>0, "parser's internal ar_states stack not initialized or underflowed");

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

static char* hvml_parser_pop_tag(hvml_parser_t *parser) {
  A(parser->tags>0, "parser's internal ar_tags stack not initialized or underflowed");

  char *tag      = parser->ar_tags[parser->tags - 1];
  parser->tags  -= 1;

  return tag;
}

static const char* hvml_parser_peek_tag(hvml_parser_t *parser) {
  A(parser->tags>0, "parser's internal ar_tags stack not initialized or underflowed");

  char *tag      = parser->ar_tags[parser->tags - 1];

  return tag;
}

