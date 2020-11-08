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
#include "hvml/hvml_utf8.h"

#include "interpreter/ext_tools.h"
#include "interpreter/interpreter_basic.h"
#include "interpreter/interpreter_two_part.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <algorithm>
using namespace std;

static int process(FILE *in, const char *ext);
static int process_hvml(FILE *in,
                        FILE *out,
                        FILE *init_part_f,
                        FILE *observe_part_f);
static int process_json(FILE *in);
static int process_utf8(FILE *in);

#define PATH_MAX    512
static char output_filename[PATH_MAX+1];
static char init_part_filename[PATH_MAX+1];
static char observe_part_filename[PATH_MAX+1];

int main(int argc, char *argv[])
{
    if (argc != 2) {
        E("arguments error");
        return 0;
    }

    if (getenv("NEG")) {
        hvml_log_set_output_only(1);
    }

    hvml_log_set_thread_type("main");

    const char *file_in = argv[1];
    const char *fin_ext = file_ext(file_in);

    if (0 != strnicmp(fin_ext, ".hvml", 5)) {
        E("input file name error");
        return 0;
    }

    size_t fname_len = min ((size_t)(PATH_MAX - 6), strlen(file_in) - 5);
    strncpy(output_filename, file_in, fname_len);
    strncpy(init_part_filename, file_in, fname_len);
    strncpy(observe_part_filename, file_in, fname_len);
    strncat(output_filename, ".output.hvml", PATH_MAX);
    strncat(init_part_filename, ".init_part.xml", PATH_MAX);
    strncat(observe_part_filename, ".observe_part.xml", PATH_MAX);

    I("output file: %s", output_filename);

    FILE *in = fopen(file_in, "rb");
    if (! in) {
        E("failed to open input file: %s", file_in);
        return 1;
    }

    FILE *out = fopen(output_filename, "wb");
    if (! out) {
        E("failed to create output file: %s", output_filename);
        fclose(in);
        return 1;
    }

    FILE *init_part_f = fopen(init_part_filename, "wb");
    if (! init_part_f) {
        E("failed to create file: %s", init_part_filename);
        fclose(in);
        fclose(out);
        return 1;
    }

    FILE *observe_part_f = fopen(observe_part_filename, "wb");
    if (! observe_part_f) {
        E("failed to create file: %s", observe_part_filename);
        fclose(in);
        fclose(out);
        fclose(init_part_f);
        return 1;
    }

    I("processing file: %s", file_in);
    int ret = process_hvml(in, 
                           out,
                           init_part_f,
                           observe_part_f);

    fclose(in);
    fclose(out);
    fclose(init_part_f);
    fclose(observe_part_f);

    if (ret) return ret;
    return 0;
}

static int process_hvml(FILE *in,
                        FILE *output_hvml_f,
                        FILE *init_part_f,
                        FILE *observe_part_f)
{
    hvml_dom_t *dom = hvml_dom_load_from_stream(in);
    if (dom)
    {
        // This is a test, print as origin file is.
        //Interpreter_Basic::GetOutput(dom, output_hvml_f);

        // Test hvml_dom_clone
        // hvml_dom_t *clone_dom = hvml_dom_clone(dom);

        // Interpreter_Basic::GetOutput(clone_dom, output_hvml_f);
        // hvml_dom_destroy(clone_dom);
        // return 0;

        InitGroup_t    init_part;
        ObserveGroup_t observe_part;

        D("--------------- GetOutput --------");
        Interpreter_TwoPart::GetOutput(dom,
                                       &init_part,
                                       &observe_part);

        // Unfinished function
        //Interpreter_TwoPart::DomToHtml(dom,
        //                               output_hvml_f);
        D("--------------- DumpInitPart --------");
        Interpreter_TwoPart::DumpInitPart(&init_part,
                                          init_part_f);

        D("--------------- DumpObservePart --------");
        Interpreter_TwoPart::DumpObservePart(&observe_part,
                                             observe_part_f);

        D("--------------- ReleaseTwoPart --------");
        Interpreter_TwoPart::ReleaseTwoPart(&init_part,
                                            &observe_part);

        hvml_dom_destroy(dom);
        printf("\n");
        return 0;
    }
    return 1;
}
