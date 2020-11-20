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

#include "hvml/hvml_dom.h"
#include "hvml/hvml_jo.h"
#include "hvml/hvml_json_parser.h"
#include "hvml/hvml_log.h"
#include "hvml/hvml_printf.h"
#include "hvml/hvml_utf8.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static int with_clone = 0;

static const char* file_ext(const char *file);
static int process(FILE *in, const char *ext);
static int process_hvml(FILE *in);
static int process_json(FILE *in);
static int process_utf8(FILE *in);

int main(int argc, char *argv[]) {
    if (argc == 1) return 0;

    if (getenv("NEG")) {
        hvml_log_set_output_only(1);
    }

    hvml_log_set_thread_type("main");

    for (int i=1; i<argc; ++i) {
        const char *arg = argv[i];
        if (strcmp(arg, "-c")==0) {
            with_clone = 1;
            continue;
        }
        const char *file = argv[i];
        const char *ext  = file_ext(file);

        FILE *in = fopen(file, "rb");
        if (!in) {
            E("failed to open file: %s", file);
            return 1;
        }

        I("processing file: %s", file);
        int ret = process(in, ext);

        if (in) fclose(in);

        if (ret) return ret;
    }

    return 0;
}

static const char* file_ext(const char *file) {
    const char *p = strrchr(file, '.');
    return p ? p : "";
}

static int process(FILE *in, const char *ext) {
    if (strcmp(ext, ".utf8")==0) {
        return process_utf8(in);
    } else if (strcmp(ext, ".json")==0) {
        return process_json(in);
    } else {
        return process_hvml(in);
    }
}

static int process_hvml(FILE *in) {
    int r = 1;
    hvml_dom_t *dom = hvml_dom_load_from_stream(in);
    do {
        if (!dom) break;

        A(hvml_dom_type(dom)==MKDOT(D_ROOT), "internal logic error");
        if (with_clone) {
            hvml_dom_t *v = hvml_dom_clone(dom);
            if (!v) break;
            A(hvml_dom_type(v)==hvml_dom_type(dom), "internal logic error");
            hvml_dom_destroy(dom);
            dom = v;
        }

        hvml_dom_printf(dom, stdout);
        r = 0;
    } while (0);
    if (dom) hvml_dom_destroy(dom);
    printf("\n");
    return r ? 1 : 0;
}

static int process_json(FILE *in) {
    int r = 1;
    hvml_jo_value_t *jo = hvml_jo_value_load_from_stream(in);
    do {
        if (!jo) break;

        if (with_clone) {
            hvml_jo_value_t *v = hvml_jo_clone(jo);
            if (!v) break;
            hvml_jo_value_free(jo);
            jo = v;
        }

        hvml_jo_value_printf(jo, stdout);
        r = 0;
    } while (0);

    if (jo) hvml_jo_value_free(jo);
    printf("\n");
    return r ? 1 : 0;
}

static int process_utf8(FILE *in) {
    char buf[4096] = {0};
    int  n         = 0;
    int  ret       = 0;

    hvml_utf8_decoder_t *decoder = hvml_utf8_decoder();
    if (!decoder) {
        E("failed to open utf8 decoder");
        return 1;
    }

    while ( (n=fread(buf, 1, sizeof(buf), in))>0) {
        for (int i=0; i<n; ++i) {
            uint64_t cp = 0;
            ret = hvml_utf8_decoder_push(decoder, buf[i], &cp);
            if (ret==0) continue;
            if (ret==1) {
                char utf8[5] = {0};
                size_t len = sizeof(utf8);
                ret = hvml_utf8_encode(cp, utf8, &len);
                if (ret) {
                    ret = -1;
                    break;
                }
                if (len<0 || len>4) {
                    E("internal logic error");
                    ret = -1;
                    break;
                }
                utf8[len] = '\0';
                fprintf(stdout, "%s", utf8);
                ret = 0;
                continue;
            }
            ret = -1;
            break;
        }
        if (ret==-1) break;
    }

    if (ret==0) {
        if (!hvml_utf8_decoder_ready(decoder)) {
            ret = -1;
        }
    }
    hvml_utf8_decoder_destroy(decoder);
    decoder = NULL;

    return ret ? 1 : 0;
}

