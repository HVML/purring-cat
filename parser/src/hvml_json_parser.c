#include "hvml_json_parser.h"

#include "hvml_log.h"

#include <ctype.h>
#include <string.h>

#define MKSTATE(state) HVML_JSON_PARSER_STATE_##state
#define MKSTR(state)  "HVML_JSON_PARSER_STATE_"#state

typedef enum {
  MKSTATE(BEGIN),
  MKSTATE(END),
} HVML_JSON_PARSER_STATE;

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

struct hvml_json_parser_s {
  hvml_json_parser_conf_t        conf;
  HVML_JSON_PARSER_STATE        *ar_states;
  size_t                         states;
  string_t                       cache;

  string_t                       curr;

  size_t                         line;
  size_t                         col;
};

static int                    hvml_json_parser_push_state(hvml_json_parser_t *parser, HVML_JSON_PARSER_STATE state);
static HVML_JSON_PARSER_STATE hvml_json_parser_pop_state(hvml_json_parser_t *parser);
static HVML_JSON_PARSER_STATE hvml_json_parser_peek_state(hvml_json_parser_t *parser);
static HVML_JSON_PARSER_STATE hvml_json_parser_chg_state(hvml_json_parser_t *parser, HVML_JSON_PARSER_STATE state);
static void                   dump_states(hvml_json_parser_t *parser);

#define get_line(parser) (parser->conf.offset_line + parser->line + 1)
#define get_col(parser)  (parser->conf.offset_col  + parser->col  + 1)


hvml_json_parser_t* hvml_json_parser_create(hvml_json_parser_conf_t conf) {
  hvml_json_parser_t *parser = (hvml_json_parser_t*)calloc(1, sizeof(*parser));
  if (!parser) return NULL;

  parser->conf = conf;
  hvml_json_parser_push_state(parser, MKSTATE(BEGIN));

  return parser;
}

void hvml_json_parser_destroy(hvml_json_parser_t *parser) {
  string_clear(&parser->cache);
  string_clear(&parser->curr);
  free(parser->ar_states); parser->ar_states = NULL;
  free(parser);
}

void hvml_json_parser_reset(hvml_json_parser_t *parser) {
  string_clear(&parser->cache);
  string_clear(&parser->curr);
  parser->states = 0;
  hvml_json_parser_push_state(parser, MKSTATE(BEGIN));
  parser->line   = 0;
  parser->col    = 0;
}

static int hvml_json_parser_at_begin(hvml_json_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    default: {
      if (parser->conf.embedded) return -1;
      E("==%s%c==: unexpected [0x%02x/%c]@[%ldr/%ldc] in state: [%s]", string_get(&parser->curr), c, c, c, get_line(parser), get_col(parser), str_state);
      return -1;
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_end(hvml_json_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    default: {
      if (parser->conf.embedded) return -1;
      E("==%s%c==: unexpected [0x%02x/%c]@[%ldr/%ldc] in state: [%s]", string_get(&parser->curr), c, c, c, get_line(parser), get_col(parser), str_state);
      return -1;
    } break;
  }
  return 0;
}

static int do_hvml_json_parser_parse_char(hvml_json_parser_t *parser, const char c) {
  HVML_JSON_PARSER_STATE state = hvml_json_parser_peek_state(parser);
  switch (state) {
    case MKSTATE(BEGIN): {
      return hvml_json_parser_at_begin(parser, c, MKSTR(BEGIN));
    } break;
    case MKSTATE(END): {
      return hvml_json_parser_at_end(parser, c, MKSTR(END));
    } break;
    default: {
      E("not implemented for state: [%d]; curr: %s", state, parser->curr.str);
      return -1;
    } break;
  }
  return 0;
}

int hvml_json_parser_parse_char(hvml_json_parser_t *parser, const char c) {
  int ret = 1;
  do {
    ret = do_hvml_json_parser_parse_char(parser, c);
  } while (ret==1); // ret==1: to retry
  if (ret==0) {
    if (c=='\n') {
      string_reset(&parser->curr);
      ++parser->line;
      parser->conf.offset_col = 0;
      parser->col = 0;
    } else {
      string_append(&parser->curr, c);
      ++parser->col;
    }
  }
  return ret;
}

int hvml_json_parser_parse(hvml_json_parser_t *parser, const char *buf, size_t len) {
  for (size_t i=0; i<len; ++i) {
    int ret = hvml_json_parser_parse_char(parser, buf[i]);
    if (ret) return ret;
  }
  return 0;
}

int hvml_json_parser_parse_string(hvml_json_parser_t *parser, const char *str) {
  return hvml_json_parser_parse(parser, (const char *)str, strlen(str));
}

int hvml_json_parser_parse_end(hvml_json_parser_t *parser) {
  if (parser->states != 1 || hvml_json_parser_peek_state(parser)==MKSTATE(END) || hvml_json_parser_peek_state(parser)==MKSTATE(BEGIN)) {
    D("states: %ld/%d", parser->states, hvml_json_parser_peek_state(parser));
    return -1;
  }
  return 0;
}

int hvml_json_parser_is_begin(hvml_json_parser_t *parser) {
  if (parser->states == 1 && hvml_json_parser_peek_state(parser)==MKSTATE(BEGIN)) return 1;
  return 0;
}

int hvml_json_parser_is_ending(hvml_json_parser_t *parser) {
  if (parser->states == 1 && hvml_json_parser_peek_state(parser)==MKSTATE(END)) return 1;
  return 0;
}

void hvml_json_parser_set_offset(hvml_json_parser_t *parser, size_t line, size_t col) {
  parser->conf.offset_line = line;
  parser->conf.offset_col  = col;
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

static int hvml_json_parser_push_state(hvml_json_parser_t *parser, HVML_JSON_PARSER_STATE state) {
  HVML_JSON_PARSER_STATE *st = (HVML_JSON_PARSER_STATE*)realloc(parser->ar_states, (parser->states + 1) * sizeof(*st));
  if (!st) return -1; 

  st[parser->states] = state;
  parser->states    += 1;
  parser->ar_states  = st;

  return 0;
}

static HVML_JSON_PARSER_STATE hvml_json_parser_pop_state(hvml_json_parser_t *parser) {
  A(parser->states>1, "parser's internal ar_states stack not initialized or underflowed or would be underflowed");

  HVML_JSON_PARSER_STATE st  = parser->ar_states[parser->states - 1];
  parser->states       -= 1;

  return st;
}

static HVML_JSON_PARSER_STATE hvml_json_parser_peek_state(hvml_json_parser_t *parser) {
  A(parser->states>0, "parser's internal ar_states stack not initialized or underflowed");

  HVML_JSON_PARSER_STATE st = parser->ar_states[parser->states - 1];

  return st;
}

static HVML_JSON_PARSER_STATE hvml_json_parser_chg_state(hvml_json_parser_t *parser, HVML_JSON_PARSER_STATE state) {
  A(parser->states>0, "parser's internal ar_states stack not initialized or underflowed");

  HVML_JSON_PARSER_STATE st                  = parser->ar_states[parser->states - 1];
  parser->ar_states[parser->states - 1] = state;

  return st;
}

static void dump_states(hvml_json_parser_t *parser) {
  D("states:");
  for (size_t i=0; i<parser->states; ++i) {
    D("state: %ld, %d", i, parser->ar_states[i]);
  }
  D("==");
}

