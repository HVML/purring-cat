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

#ifndef _interpreter_base_h_
#define _interpreter_base_h_

#include "hvml/hvml_printf.h"
#include "hvml/hvml_parser.h"
#include "hvml/hvml_dom.h"
#include "hvml/hvml_jo.h"
#include "hvml/hvml_json_parser.h"
#include "hvml/hvml_log.h"
#include "hvml/hvml_utf8.h"

typedef struct dom_printf_s {
    int             lvl;
    FILE           *out;
} dom_printf_t;

class Interpreter_Basic
{
public:
    Interpreter_Basic(FILE *out);
    static void GetOutput(hvml_dom_t *input_dom, FILE *out);

private:
    dom_printf_t m_dom_prtt;
    static void traverse_for_printf(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout);
};

#endif //_interpreter_base_h_