#ifndef _hvml_parser_h_e6dbd1ba_194a_47bb_8d8f_043857fcdd19_
#define _hvml_parser_h_e6dbd1ba_194a_47bb_8d8f_043857fcdd19_

#include <stddef.h>
#include <stdint.h>

typedef struct hvml_parser_s                hvml_parser_t;
typedef struct hvml_parser_conf_s           hvml_parser_conf_t;

struct hvml_parser_conf_s {
  int (*on_open_tag)(void *arg, const char *tag);
  int (*on_attr_key)(void *arg, const char *key);
  int (*on_attr_val)(void *arg, const char *val);
  int (*on_close_tag)(void *arg);
  int (*on_text)(void *arg, const char *txt);

  // json callbacks
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
};

hvml_parser_t* hvml_parser_create(hvml_parser_conf_t conf);
void           hvml_parser_destroy(hvml_parser_t *parser);

int            hvml_parser_parse_char(hvml_parser_t *parser, const char c);
int            hvml_parser_parse(hvml_parser_t *parser, const char *buf, size_t len);
int            hvml_parser_parse_string(hvml_parser_t *parser, const char *str);
int            hvml_parser_parse_end(hvml_parser_t *parser);


#endif // _hvml_parser_h_e6dbd1ba_194a_47bb_8d8f_043857fcdd19_

