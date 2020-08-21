#include "hvml_parser.h"

#include "hvml_log.h"

#include <stdio.h>

static int on_open_tag(hvml_parser_t *parser, const char *tag) {
  printf("open tag: %s\n", tag);
  return 0;
}

static int on_attr_key(hvml_parser_t *parser, const char *key) {
  printf("attr key: %s\n", key);
  return 0;
}

static int on_attr_val(hvml_parser_t *parser, const char *val) {
  printf("attr val: %s\n", val);
  return 0;
}

static int on_close_tag(hvml_parser_t *parser) {
  printf("close tag\n");
  return 0;
}

static int on_text(hvml_parser_t *parser, const char *txt) {
  printf("text: %s\n", txt);
  return 0;
}

static int process(FILE *in) {
  char buf[4096] = {0};
  int n = 0;
  int ret = 0;
  hvml_parser_conf_t conf = {0};

  conf.on_open_tag      = on_open_tag;
  conf.on_attr_key      = on_attr_key;
  conf.on_attr_val      = on_attr_val;
  conf.on_close_tag     = on_close_tag;
  conf.on_text          = on_text;

  if (!in) in = stdin;

  hvml_log_set_thread_type("main");

  hvml_parser_t *parser = hvml_parser_create(conf);
  if (!parser) return 1;
  while ( (n=fread(buf, 1, sizeof(buf), in))>0) {
    ret = hvml_parser_parse(parser, buf, n);
    if (ret) break;
  }
  if (ret==0) ret = hvml_parser_parse_end(parser);
  hvml_parser_destroy(parser);
  if (ret) return 1;
  return 0;
}

int main(int argc, char *argv[]) {
  FILE *in = NULL;
  if (argc >= 2) {
    in = fopen(argv[1], "rb");
    if (!in) {
      E("failed to open file: %s", argv[1]);
      return 1;
    }
  }
  int ret = process(in ? in : stdin);
  if (in) fclose(in);
  return ret;
}

