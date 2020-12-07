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

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static int with_clone = 0;
static int with_antlr4 = 0;

static const char* file_ext(const char *file);
static int process(FILE *in, const char *ext, hvml_dom_t *hvml);
static int process_hvml(FILE *in);
static int process_json(FILE *in);
static int process_utf8(FILE *in);
static int process_xpath(FILE *in, hvml_dom_t *hvml);

int main(int argc, char *argv[]) {
    if (argc == 1) return 0;

    if (getenv("NEG")) {
        hvml_log_set_output_only(1);
    }

    hvml_dom_t *hvml = NULL;


    hvml_log_set_thread_type("main");

    int ok = 1;
    for (int i=1; i<argc && ok; ++i) {
        const char *arg = argv[i];
        if (strcmp(arg, "-c")==0) {
            with_clone = 1;
            continue;
        }
        if (strcmp(arg, "--hvml")==0) {
            ++i;
            if (i>=argc) {
                E("expecting <hvml>, but got nothing");
                ok = 0;
                break;
            }
            continue;
        }
        if (strcmp(arg, "--antlr4")==0) {
            with_antlr4 = 1;
            continue;
        }
        const char *file = argv[i];
        const char *ext  = file_ext(file);

        FILE *in = fopen(file, "rb");
        if (!in) {
            E("failed to open file: %s", file);
            ok = 0;
            break;
        }

        if (strcmp(ext, ".xpath")==0) {
            char buf[4096];
            snprintf(buf, sizeof(buf), "%s.hvml", argv[i]);
            FILE *in = fopen(buf, "rb");
            do {
                if (!in) {
                    E("failed to open file: %s", buf);
                    ok = 0;
                    break;
                }
                if (hvml) {
                    hvml_dom_destroy(hvml);
                    hvml = NULL;
                }
                hvml = hvml_dom_load_from_stream(in);
                fclose(in); in = NULL;
                if (!hvml) {
                    E("failed to load hvml from file: %s", buf);
                    ok = 0;
                    break;
                }
            } while (0);
        }

        if (ok) {
            I("processing file: %s", file);
            int ret = process(in, ext, hvml);
            ok = ret ? 0 : 1;
        }

        if (in) fclose(in);

        break;
    }

    if (hvml) hvml_dom_destroy(hvml);

    return ok ? 0 : 1;
}

static const char* file_ext(const char *file) {
    const char *p = strrchr(file, '.');
    return p ? p : "";
}

static int process(FILE *in, const char *ext, hvml_dom_t *hvml) {
    if (strcmp(ext, ".utf8")==0) {
        return process_utf8(in);
    } else if (strcmp(ext, ".json")==0) {
        return process_json(in);
    }else if (strcmp(ext, ".xpath")==0) {
        return process_xpath(in, hvml);
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

static int process_xpath(FILE *in, hvml_dom_t *hvml) {
    char   *line     = NULL;
    size_t  n        = 0;

    int r = 0;
    int i = 0;
    while (!feof(in)) {
        ssize_t len = getline(&line, &n, in);
        if (len<0) break;
        ++i;
        if (len==0) continue;
        if (line[0]=='#') continue;
        if (line[len-1]=='\n') line[len-1] = '\0';
        const char *p = line;
        while (*p && isspace(*p)) ++p;
        if (strlen(p)==0) continue;
        fprintf(stderr, "parsing xpath @[%d]: [%s]\n", i, p);
        hvml_doms_t doms = {0};
        if (with_antlr4) {
            r = hvml_dom_qry(hvml, p, &doms);
        } else {
            r = hvml_dom_query(hvml, p, &doms);
        }
        if (r) {
            fprintf(stderr, "parsing xpath: failed\n");
            break;
        }
        fprintf(stdout, "==================\n");
        fprintf(stdout, "parsing xpath: @[%d]: [%s] => # of nodes [%" PRId64 "]\n", i, p, doms.ndoms);
        for (size_t i=0; i<doms.ndoms; ++i) {
            hvml_dom_t *d = doms.doms[i];
            const char *title = 0;
            switch (hvml_dom_type(d)) {
                case MKDOT(D_ROOT): title = "Document";   break;
                case MKDOT(D_TAG):  title = "Element";    break;
                case MKDOT(D_ATTR): title = "Attribute";  break;
                case MKDOT(D_TEXT): title = "Text";       break;
                case MKDOT(D_JSON): title = "Json";       break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
            fprintf(stdout, "%" PRId64 ":[%s]=", i, title);
            hvml_dom_printf(d, stdout);
            fprintf(stdout, "\n");
        }
        hvml_doms_cleanup(&doms);
    }

    if (line) free(line);

    return r;
}

