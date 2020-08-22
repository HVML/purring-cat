#ifndef _hvml_json_parser_h_e6dbd1ba_194a_47bb_8d8f_043857fcdd19_
#define _hvml_json_parser_h_e6dbd1ba_194a_47bb_8d8f_043857fcdd19_

#include <stddef.h>
#include <stdint.h>

typedef struct hvml_json_parser_s                hvml_json_parser_t;
typedef struct hvml_json_parser_conf_s           hvml_json_parser_conf_t;

struct hvml_json_parser_conf_s {
  int (*on_begin)(void *arg);
  int (*on_open_array)(void *arg);
  int (*on_close_array)(void *arg);
  int (*on_open_obj)(void *arg);
  int (*on_close_obj)(void *arg);
  int (*on_key)(void *arg, const char *key);
  int (*on_true)(void *arg);
  int (*on_false)(void *arg);
  int (*on_null)(void *arg);
  int (*on_string)(void *arg, const char *val);
  int (*on_integer)(void *arg, const char *origin, int64_t val);
  int (*on_double)(void *arg, const char *origin, double val);
  int (*on_end)(void *arg);

  void *arg;
  int   embedded:1;
  size_t     offset_line;
  size_t     offset_col;
};

hvml_json_parser_t* hvml_json_parser_create(hvml_json_parser_conf_t conf);
void                hvml_json_parser_destroy(hvml_json_parser_t *parser);
void                hvml_json_parser_reset(hvml_json_parser_t *parser);

int                 hvml_json_parser_parse_char(hvml_json_parser_t *parser, const char c);
int                 hvml_json_parser_parse(hvml_json_parser_t *parser, const char *buf, size_t len);
int                 hvml_json_parser_parse_string(hvml_json_parser_t *parser, const char *str);
int                 hvml_json_parser_parse_end(hvml_json_parser_t *parser);

int                 hvml_json_parser_is_begin(hvml_json_parser_t *parser);
int                 hvml_json_parser_is_ending(hvml_json_parser_t *parser);

void                hvml_json_parser_set_offset(hvml_json_parser_t *parser, size_t line, size_t col);

#endif // _hvml_json_parser_h_e6dbd1ba_194a_47bb_8d8f_043857fcdd19_

