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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static const char* file_ext(const char *file);
static int process(FILE *in, const char *ext);
static int process_hvml(FILE *in);
static int process_json(FILE *in);

int main(int argc, char *argv[]) {
    if (argc == 1) return 0;

    if (getenv("NEG")) {
        hvml_log_set_output_only(1);
    }

    hvml_log_set_thread_type("main");

    for (int i=1; i<argc; ++i) {
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
    if (strcmp(ext, ".json")==0) {
        return process_json(in);
    } else {
        return process_hvml(in);
    }
}

static int process_hvml(FILE *in) {
    hvml_dom_t *dom = hvml_dom_load_from_stream(in);
    if (dom) {
        hvml_dom_printf(dom, stdout);
        hvml_dom_destroy(dom);
        printf("\n");
        return 0;
    }
    return 1;
}

static int process_json(FILE *in) {
    hvml_jo_value_t *jo = hvml_jo_value_load_from_stream(in);
    if (jo) {
        hvml_jo_value_printf(jo, stdout);
        hvml_jo_value_free(jo);
        printf("\n");
        return 0;
    }
    return 1;
}

