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

#include "hvml/hvml_json_parser.h"

#include "hvml/hvml_log.h"

#include <ctype.h>
#include <string.h>

#define MKSTATE(state) HVML_JSON_PARSER_STATE_##state
#define MKSTR(state)  "HVML_JSON_PARSER_STATE_"#state

typedef enum {
  MKSTATE(BEGIN),
    MKSTATE(OPEN_OBJ),
    MKSTATE(KEY_DONE),
      MKSTATE(STR),
    MKSTATE(COLON),
    MKSTATE(VAL_DONE),
    MKSTATE(OBJ_COMMA),
    MKSTATE(OPEN_ARRAY),
    MKSTATE(ITEM_DONE),
    MKSTATE(ARRAY_COMMA),
    MKSTATE(TFN),
    MKSTATE(NUMBER),
    MKSTATE(MINUS),
    MKSTATE(INTEGER),
    MKSTATE(ZERO),
    MKSTATE(DECIMAL),
    MKSTATE(ESYM),
    MKSTATE(EXPONENT),
  MKSTATE(END),
} HVML_JSON_PARSER_STATE;

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

#define EPARSE()                                                                         \
  E("==%s%c==: unexpected [0x%02x/%c]@[%ldr/%ldc] in state: [%s]",                       \
    string_get(&parser->curr), c, c, c,                                                  \
    get_line(parser), get_col(parser),                                                   \
    str_state)

#define number_found()                                                                     \
do {                                                                                       \
  if ((parser->cache.len==0) ||                                                            \
      (parser->cache.str[parser->cache.len-1]=='+') ||                                     \
      (parser->cache.str[parser->cache.len-1]=='-'))                                       \
  {                                                                                        \
    EPARSE();                                                                              \
    ret = -1;                                                                              \
    break;                                                                                 \
  }                                                                                        \
  if (parser->conf.on_double) {                                                            \
    double v;                                                                              \
    int len = 0;                                                                           \
    int n = sscanf(string_get(&parser->cache), "%lg%n", &v, &len);                         \
    ret = -1;                                                                              \
    if (n==1 && len == parser->cache.len) {                                                \
      ret = parser->conf.on_double(parser->conf.arg, string_get(&parser->cache), v);       \
    }                                                                                      \
  }                                                                                        \
  string_reset(&parser->cache);                                                            \
} while (0)

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
    case '{': // '}'
    {
      hvml_json_parser_chg_state(parser, MKSTATE(END));
      hvml_json_parser_push_state(parser, MKSTATE(OPEN_OBJ));
      int ret = 0;
      if (parser->conf.on_begin) {
        ret = parser->conf.on_begin(parser->conf.arg);
      }
      if (ret==0 && parser->conf.on_open_obj) {
        ret = parser->conf.on_open_obj(parser->conf.arg);
      }
      if (ret) return ret;
    } break;
    case '[': // ']'
    {
      hvml_json_parser_chg_state(parser, MKSTATE(END));
      hvml_json_parser_push_state(parser, MKSTATE(OPEN_ARRAY));
      int ret = 0;
      if (parser->conf.on_begin) {
        ret = parser->conf.on_begin(parser->conf.arg);
      }
      if (ret==0 && parser->conf.on_open_array) {
        ret = parser->conf.on_open_array(parser->conf.arg);
      }
      if (ret) return ret;
    } break;
    case '"': // '"'
    {
      hvml_json_parser_chg_state(parser, MKSTATE(END));
      hvml_json_parser_push_state(parser, MKSTATE(STR));
      int ret = 0;
      if (parser->conf.on_begin) {
        ret = parser->conf.on_begin(parser->conf.arg);
      }
      if (ret) return ret;
    } break;
    case 't':
    case 'f':
    case 'n': {
      hvml_json_parser_chg_state(parser, MKSTATE(END));
      hvml_json_parser_push_state(parser, MKSTATE(TFN));
      int ret = 0;
      if (parser->conf.on_begin) {
        ret = parser->conf.on_begin(parser->conf.arg);
      }
      if (ret) return ret;
      return 1; // retry
    } break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-': {
      hvml_json_parser_chg_state(parser, MKSTATE(END));
      hvml_json_parser_push_state(parser, MKSTATE(NUMBER));
      int ret = 0;
      if (parser->conf.on_begin) {
        ret = parser->conf.on_begin(parser->conf.arg);
      }
      if (ret) return ret;
      return 1; // retry
    } break;
    default: {
      if (parser->conf.embedded) return -1;
      EPARSE();
      return -1;
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_open_obj(hvml_json_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    case '"': {
      hvml_json_parser_chg_state(parser, MKSTATE(KEY_DONE));
      hvml_json_parser_push_state(parser, MKSTATE(STR));
    } break;
    default: {
      EPARSE();
      return -1;
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_key_done(hvml_json_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    case ':': {
      hvml_json_parser_chg_state(parser, MKSTATE(COLON));
    } break;
    default: {
      EPARSE();
      return -1;
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_str(hvml_json_parser_t *parser, const char c, const char *str_state) {
  switch (c) {
    case '"': {
      hvml_json_parser_pop_state(parser);
      int state = hvml_json_parser_peek_state(parser);
      switch (state) {
        case MKSTATE(KEY_DONE): {
          int ret = 0;
          if (parser->conf.on_key) {
            ret = parser->conf.on_key(parser->conf.arg, string_get(&parser->cache));
          }
          string_reset(&parser->cache);
          if (ret) return ret;
        } break;
        case MKSTATE(VAL_DONE): {
          int ret = 0;
          if (parser->conf.on_string) {
            ret = parser->conf.on_string(parser->conf.arg, string_get(&parser->cache));
          }
          string_reset(&parser->cache);
          if (ret) return ret;
        } break;
        case MKSTATE(END): {
          int ret = 0;
          if (parser->conf.on_string) {
            ret = parser->conf.on_string(parser->conf.arg, string_get(&parser->cache));
          }
          string_reset(&parser->cache);
          if (ret) return ret;
        } break;
        default: {
          E("not implemented for state: [%d]; curr: %s", state, parser->curr.str);
          return -1;
        } break;
      }
    } break;
    default: {
      string_append(&parser->cache, c);
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_colon(hvml_json_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    case '"': {
      hvml_json_parser_chg_state(parser, MKSTATE(VAL_DONE));
      hvml_json_parser_push_state(parser, MKSTATE(STR));
    } break;
    case 't':
    case 'f':
    case 'n': {
      hvml_json_parser_chg_state(parser, MKSTATE(VAL_DONE));
      hvml_json_parser_push_state(parser, MKSTATE(TFN));
      return 1; // retry
    } break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-': {
      hvml_json_parser_chg_state(parser, MKSTATE(VAL_DONE));
      hvml_json_parser_push_state(parser, MKSTATE(NUMBER));
      return 1; // retry
    } break;
    default: {
      EPARSE();
      return -1;
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_val_done(hvml_json_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    // '{'
    case '}': {
      hvml_json_parser_pop_state(parser);
      int ret = 0;
      if (parser->conf.on_close_obj) {
        ret = parser->conf.on_close_obj(parser->conf.arg);
      }
      if (ret) return ret;
    } break;
    case ',': {
      hvml_json_parser_chg_state(parser, MKSTATE(OBJ_COMMA));
    } break;
    default: {
      EPARSE();
      return -1;
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_obj_comma(hvml_json_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    case '"': {
      hvml_json_parser_chg_state(parser, MKSTATE(KEY_DONE));
      hvml_json_parser_push_state(parser, MKSTATE(STR));
    } break;
    case ',': {
      // just eat it
    } break;
    // '{'
    case '}': {
      // followed by , is valid
      hvml_json_parser_chg_state(parser, MKSTATE(VAL_DONE));
      // retry
      return 1;
    } break;
    default: {
      EPARSE();
      return -1;
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_open_array(hvml_json_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    case '{': // '}'
    {
      hvml_json_parser_chg_state(parser, MKSTATE(ITEM_DONE));
      hvml_json_parser_push_state(parser, MKSTATE(OPEN_OBJ));
      int ret = 0;
      if (parser->conf.on_open_obj) {
        ret = parser->conf.on_open_obj(parser->conf.arg);
      }
      if (ret) return ret;
    } break;
    default: {
      EPARSE();
      return -1;
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_item_done(hvml_json_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    case ',': {
      hvml_json_parser_chg_state(parser, MKSTATE(ARRAY_COMMA));
    } break;
    // '['
    case ']': {
      hvml_json_parser_pop_state(parser);
      int ret = 0;
      if (parser->conf.on_close_array) {
        ret = parser->conf.on_close_array(parser->conf.arg);
      }
      if (ret) return ret;
    } break;
    default: {
      EPARSE();
      return -1;
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_array_comma(hvml_json_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    case '{': // '}'
    {
      hvml_json_parser_chg_state(parser, MKSTATE(ITEM_DONE));
      hvml_json_parser_push_state(parser, MKSTATE(OPEN_OBJ));
      int ret = 0;
      if (parser->conf.on_open_obj) {
        ret = parser->conf.on_open_obj(parser->conf.arg);
      }
      if (ret) return ret;
    } break;
    case 't':
    case 'f':
    case 'n': {
      hvml_json_parser_chg_state(parser, MKSTATE(ITEM_DONE));
      hvml_json_parser_push_state(parser, MKSTATE(TFN));
      return 1; // retry
    } break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '+':
    case '-': {
      hvml_json_parser_chg_state(parser, MKSTATE(ITEM_DONE));
      hvml_json_parser_push_state(parser, MKSTATE(NUMBER));
      return 1; // retry
    } break;
    case ',': {
      // just eat it
    } break;
    // '['
    case ']': {
      // followed by , is valid
      hvml_json_parser_chg_state(parser, MKSTATE(ITEM_DONE));
      // retry
      return 1;
    } break;
    default: {
      EPARSE();
      return -1;
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_tfn(hvml_json_parser_t *parser, const char c, const char *str_state) {
  const char tfn_true[]  = "true";
  const char tfn_false[] = "false";
  const char tfn_null[]  = "null";
  if (parser->cache.len==0) {
    switch (c) {
      case 't':
      case 'f':
      case 'n':{
        string_append(&parser->cache, c);
      } break;
      default: {
        EPARSE();
        return -1;
      } break;
    }
    return 0;
  }
  if (parser->cache.str[0] == 't') {
    if (tfn_true[parser->cache.len] == c) {
      string_append(&parser->cache, c);
      if (parser->cache.len < sizeof(tfn_true) - 1) return 0;
      int ret = 0;
      if (parser->conf.on_true) {
        ret = parser->conf.on_true(parser->conf.arg);
      }
      string_reset(&parser->cache);
      hvml_json_parser_pop_state(parser);
      if (ret) return ret;
      return 0;
    }
  } else if (parser->cache.str[0] == 'f') {
    if (tfn_false[parser->cache.len] == c) {
      string_append(&parser->cache, c);
      if (parser->cache.len < sizeof(tfn_false) - 1) return 0;
      int ret = 0;
      if (parser->conf.on_false) {
        ret = parser->conf.on_false(parser->conf.arg);
      }
      string_reset(&parser->cache);
      hvml_json_parser_pop_state(parser);
      if (ret) return ret;
      return 0;
    }
  } else if (parser->cache.str[0] == 'n') {
    if (tfn_null[parser->cache.len] == c) {
      string_append(&parser->cache, c);
      if (parser->cache.len < sizeof(tfn_null) - 1) return 0;
      int ret = 0;
      if (parser->conf.on_null) {
        ret = parser->conf.on_null(parser->conf.arg);
      }
      string_reset(&parser->cache);
      hvml_json_parser_pop_state(parser);
      if (ret) return ret;
      return 0;
    }
  }
  EPARSE();
  return -1;
}

static int hvml_json_parser_at_number(hvml_json_parser_t *parser, const char c, const char *str_state) {
  switch (c) {
    case '+': // not in json standard
    case '-': {
      string_append(&parser->cache, c);
      hvml_json_parser_chg_state(parser, MKSTATE(MINUS));
      return 0;
    } break;
    case '0': {
      string_append(&parser->cache, c);
      hvml_json_parser_chg_state(parser, MKSTATE(ZERO));
      return 0;
    } break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      string_append(&parser->cache, c);
      hvml_json_parser_chg_state(parser, MKSTATE(INTEGER));
      return 0;
    } break;
    default: {
    } break;
  }
  EPARSE();
  return -1;
}

static int hvml_json_parser_at_minus(hvml_json_parser_t *parser, const char c, const char *str_state) {
  switch (c) {
    case '0': {
      string_append(&parser->cache, c);
      hvml_json_parser_chg_state(parser, MKSTATE(ZERO));
      return 0;
    } break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      string_append(&parser->cache, c);
      hvml_json_parser_chg_state(parser, MKSTATE(INTEGER));
      return 0;
    } break;
    default: {
      EPARSE();
      return -1;
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_zero(hvml_json_parser_t *parser, const char c, const char *str_state) {
  switch (c) {
    case '.': {
      string_append(&parser->cache, c);
      hvml_json_parser_chg_state(parser, MKSTATE(DECIMAL));
    } break;
    case 'e':
    case 'E': {
      string_append(&parser->cache, c);
      hvml_json_parser_chg_state(parser, MKSTATE(ESYM));
    } break;
    default: {
      hvml_json_parser_pop_state(parser);
      int ret = 0;
      number_found();
      if (ret) return ret;
      return 1; // retry
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_integer(hvml_json_parser_t *parser, const char c, const char *str_state) {
  switch (c) {
    case '.': {
      string_append(&parser->cache, c);
      hvml_json_parser_chg_state(parser, MKSTATE(DECIMAL));
    } break;
    case 'e':
    case 'E': {
      string_append(&parser->cache, c);
      hvml_json_parser_chg_state(parser, MKSTATE(ESYM));
    } break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      string_append(&parser->cache, c);
    } break;
    default: {
      hvml_json_parser_pop_state(parser);
      int ret = 0;
      number_found();
      if (ret) return ret;
      return 1; // retry
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_decimal(hvml_json_parser_t *parser, const char c, const char *str_state) {
  switch (c) {
    case 'e':
    case 'E': {
      string_append(&parser->cache, c);
      hvml_json_parser_chg_state(parser, MKSTATE(ESYM));
    } break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      string_append(&parser->cache, c);
    } break;
    default: {
      hvml_json_parser_pop_state(parser);
      int ret = 0;
      number_found();
      if (ret) return ret;
      return 1; // retry
      EPARSE();
      return -1;
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_esym(hvml_json_parser_t *parser, const char c, const char *str_state) {
  switch (c) {
    case '+':
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      string_append(&parser->cache, c);
      hvml_json_parser_chg_state(parser, MKSTATE(EXPONENT));
    } break;
    default: {
      EPARSE();
      return -1;
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_exponent(hvml_json_parser_t *parser, const char c, const char *str_state) {
  switch (c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      string_append(&parser->cache, c);
    } break;
    default: {
      hvml_json_parser_pop_state(parser);
      int ret = 0;
      number_found();
      if (ret) return ret;
      return 1; // retry
    } break;
  }
  return 0;
}

static int hvml_json_parser_at_end(hvml_json_parser_t *parser, const char c, const char *str_state) {
  if (isspace(c)) return 0;
  switch (c) {
    default: {
      int ret = 0;
      if (parser->conf.on_end) {
        ret = parser->conf.on_end(parser->conf.arg);
      }
      if (ret) return ret;
      if (parser->conf.embedded) return -1;
      EPARSE();
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
    case MKSTATE(OPEN_OBJ): {
      return hvml_json_parser_at_open_obj(parser, c, MKSTR(OPEN_OBJ));
    } break;
    case MKSTATE(KEY_DONE): {
      return hvml_json_parser_at_key_done(parser, c, MKSTR(KEY_DONE));
    } break;
    case MKSTATE(STR): {
      return hvml_json_parser_at_str(parser, c, MKSTR(STR));
    } break;
    case MKSTATE(COLON): {
      return hvml_json_parser_at_colon(parser, c, MKSTR(COLON));
    } break;
    case MKSTATE(VAL_DONE): {
      return hvml_json_parser_at_val_done(parser, c, MKSTR(VAL_DONE));
    } break;
    case MKSTATE(OBJ_COMMA): {
      return hvml_json_parser_at_obj_comma(parser, c, MKSTR(OBJ_COMMA));
    } break;
    case MKSTATE(OPEN_ARRAY): {
      return hvml_json_parser_at_open_array(parser, c, MKSTR(OPEN_ARRAY));
    } break;
    case MKSTATE(ITEM_DONE): {
      return hvml_json_parser_at_item_done(parser, c, MKSTR(ITEM_DONE));
    } break;
    case MKSTATE(ARRAY_COMMA): {
      return hvml_json_parser_at_array_comma(parser, c, MKSTR(ARRAY_COMMA));
    } break;
    case MKSTATE(TFN): {
      return hvml_json_parser_at_tfn(parser, c, MKSTR(TFN));
    } break;
    case MKSTATE(NUMBER): {
      return hvml_json_parser_at_number(parser, c, MKSTR(NUMBER));
    } break;
    case MKSTATE(MINUS): {
      return hvml_json_parser_at_minus(parser, c, MKSTR(MINUS));
    } break;
    case MKSTATE(ZERO): {
      return hvml_json_parser_at_zero(parser, c, MKSTR(ZERO));
    } break;
    case MKSTATE(INTEGER): {
      return hvml_json_parser_at_integer(parser, c, MKSTR(INTEGER));
    } break;
    case MKSTATE(DECIMAL): {
      return hvml_json_parser_at_decimal(parser, c, MKSTR(DECIMAL));
    } break;
    case MKSTATE(ESYM): {
      return hvml_json_parser_at_esym(parser, c, MKSTR(ESYM));
    } break;
    case MKSTATE(EXPONENT): {
      return hvml_json_parser_at_exponent(parser, c, MKSTR(EXPONENT));
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

