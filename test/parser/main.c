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

#include "hvml/hvml_jo.h"
#include "hvml/hvml_log.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static const char* file_ext(const char *file);
static int process(FILE *in, const char *ext);
static int process_hvml(FILE *in);
static int process_json(FILE *in);

int main(int argc, char *argv[]) {
    if (argc == 1) return 0;

    hvml_log_set_thread_type("main");

    const char *file = argv[1];
    const char *ext  = file_ext(file);

    FILE *in = fopen(argv[1], "rb");
    if (!in) {
        E("failed to open file: %s", argv[1]);
        return 1;
    }

    int ret = process(in, ext);

    if (in) fclose(in);

    return ret;
}

static const char* file_ext(const char *file) {
    const char *p = strrchr(file, '.');
    return p ? p : "";
}

static int process(FILE *in, const char *ext) {
    if (strcmp(ext, ".json")==0) {
        return process_json(in);
    } else {
        return process_hvml(in);
    }
}

static int on_open_tag(void *arg, const char *tag) {
    printf("open tag: %s\n", tag);
    return 0;
}

static int on_attr_key(void *arg, const char *key) {
    printf("attr key: %s\n", key);
    return 0;
}

static int on_attr_val(void *arg, const char *val) {
    printf("attr val: %s\n", val);
    return 0;
}

static int on_close_tag(void *arg) {
    printf("close tag\n");
    return 0;
}

static int on_text(void *arg, const char *txt) {
    printf("text: %s\n", txt);
    return 0;
}

// json callbacks
static int on_begin(void *arg) {
    printf("json begin:\n");
    return 0;
}

static int on_open_array(void *arg) {
    printf("json open array:\n");
    return 0;
}

static int on_close_array(void *arg) {
    printf("json close array:\n");
    return 0;
}

static int on_open_obj(void *arg) {
    printf("json open obj:\n");
    return 0;
}

static int on_close_obj(void *arg) {
    printf("json close obj:\n");
    return 0;
}

static int on_key(void *arg, const char *key) {
    printf("json key: %s\n", key);
    return 0;
}

static int on_true(void *arg) {
    printf("json true:\n");
    return 0;
}

static int on_false(void *arg) {
    printf("json false:\n");
    return 0;
}

static int on_null(void *arg) {
    printf("json null:\n");
    return 0;
}

static int on_string(void *arg, const char *val) {
    printf("json string: %s\n", val);
    return 0;
}

static int on_integer(void *arg, const char *origin, int64_t val) {
    printf("json integer: %s\t%"PRId64"\n", origin, val);
    return 0;
}

static int on_double(void *arg, const char *origin, double val) {
    printf("json double: %s\t[f]%f\t[e]%e\t[g]%g\n", origin, val, val, val);
    return 0;
}

static int on_end(void *arg) {
    printf("json end:\n");
    return 0;
}

static int process_hvml(FILE *in) {
    char buf[4096] = {0};
    int n = 0;
    int ret = 0;
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

    if (!in) in = stdin;

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

static int process_json(FILE *in) {
    hvml_jo_value_t *jo = hvml_jo_value_load_from_stream(in);
    if (jo) {
        hvml_jo_value_printf(jo, 0, stdout);
        hvml_jo_value_free(jo);
        printf("\n");
        return 0;
    }
    return 1;
}

