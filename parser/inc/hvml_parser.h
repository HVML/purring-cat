#ifndef _hvml_parser_h_e6dbd1ba_194a_47bb_8d8f_043857fcdd19_
#define _hvml_parser_h_e6dbd1ba_194a_47bb_8d8f_043857fcdd19_

#include <stddef.h>

typedef struct hvml_parser_s                hvml_parser_t;
typedef struct hvml_parser_conf_s           hvml_parser_conf_t;

struct hvml_parser_conf_s {
  int (*on_open_tag)(hvml_parser_t *parser, const char *tag);
  int (*on_attr_key)(hvml_parser_t *parser, const char *key);
  int (*on_attr_val)(hvml_parser_t *parser, const char *val);
  int (*on_close_tag)(hvml_parser_t *parser);
  int (*on_text)(hvml_parser_t *parser, const char *txt);
};

hvml_parser_t* hvml_parser_create(hvml_parser_conf_t conf);
void           hvml_parser_destroy(hvml_parser_t *parser);

int            hvml_parser_parse_char(hvml_parser_t *parser, const char c);
int            hvml_parser_parse(hvml_parser_t *parser, const char *buf, size_t len);
int            hvml_parser_parse_string(hvml_parser_t *parser, const char *str);
int            hvml_parser_parse_end(hvml_parser_t *parser);


#endif // _hvml_parser_h_e6dbd1ba_194a_47bb_8d8f_043857fcdd19_

