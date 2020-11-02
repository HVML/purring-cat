// This file is a part of Purring Cat, a reference implementation of HVML.
//
// Copyright (C) 2020, <liuxinouc@126.com>.
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

static const char* file_ext(const char *file);
static int process(FILE *in, const char *ext);
static int process_hvml(FILE *in, FILE *out);
static int process_json(FILE *in);
static int process_utf8(FILE *in);

int main(int argc, char *argv[])
{
    if (argc != 3) {
	E("arguments error");
        return 0;
    }

    if (getenv("NEG")) {
        hvml_log_set_output_only(1);
    }

    hvml_log_set_thread_type("main");

    const char *file_in = argv[1];
    const char *file_out = argv[2];
    const char *fin_ext = file_ext(file_in);
    const char *fout_ext = file_ext(file_out);

    FILE *in = fopen(file_in, "rb");
    if (! in) {
        E("failed to open input file: %s", file_in);
        return 1;
    }

    FILE *out = fopen(file_out, "wb");
    if (! out) {
        E("failed to create output file: %s", file_out);
        if (out) fclose(out);
        return 1;
    }

    I("processing file: %s", file_in);
    int ret = process_hvml(in, out);

    if (in) fclose(in);
    if (out) fclose(out);

    if (ret) return ret;
    
    return 0;
}

static const char* file_ext(const char *file)
{
    const char *p = strrchr(file, '.');
    return p ? p : "";
}

static int process_hvml(FILE *in, FILE *out)
{
    hvml_dom_t *dom = hvml_dom_load_from_stream(in);
    if (dom) {
        hvml_dom_printf(dom, stdout);
        // hvml_dom_to_html(dom, out);
        hvml_dom_destroy(dom);
        printf("\n");
        return 0;
    }
    return 1;
}
