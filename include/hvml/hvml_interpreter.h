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

#ifndef _hvml_interpreter_h_
#define _hvml_interpreter_h_

#include "hvml/hvml_jdo.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct hvml_interpreter_s              hvml_interpreter_t;

typedef struct hvml_interpreter_conf_s         hvml_interpreter_conf_t;

struct hvml_interpreter_conf_s {
};

struct hvml_interpreter_tag_reg_s              hvml_interpreter_tag_reg_t;
struct hvml_interpreter_tag_reg_s {
    const char       *name;                        // "CLASS: xxx" or "FUNC: yyy"
    const char       **attr_names;
    int (*executor)(hvml_interpreter_t *hi,
                    hvml_dom_t *dom,               // context ?
                    const char *tag_name,          // eg.: "choose"
                    const char *exec_name,         // eg.: "CLASS: CTimer",
                    const char **attr_names, ...); // a list of hvml_jdo_t*
};

hvml_interpreter_t* hvml_interpreter_create(hvml_interpreter_conf_t conf);
int                 hvml_interpreter_register_tag(hvml_interpreter_t *hi, hvml_interpreter_tag_reg_t reg);
int                 hvml_interpreter_load(hvml_interpreter_t *hi, FILE *in, hvml_dom_t **dom);
void                hvml_interpreter_destroy(hvml_interpreter_t *hi);




#ifdef __cplusplus
}
#endif

#endif // _hvml_interpreter_h_

