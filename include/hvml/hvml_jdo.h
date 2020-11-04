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

#ifndef _hvml_jdo_h_
#define _hvml_jdo_h_

#ifdef __cplusplus
extern "C" {
#endif

#define MKJDOT(type)  HVML_JDO_##type
#define MKJDOS(type) "HVML_JDO_"#type

typedef enum {
    HVML_JDO_VAL,
    HVML_JDO_FUNC
} HVML_JDO_TYPE;

typedef struct hvml_jdo_s                 hvml_jdo_t;
typedef struct hvml_jdo_func_f            hvml_jdo_func_t;
typedef struct hvml_jdo_closure_s         hvml_jdo_closure_t;

typedef hvml_jdo_t* (*hvml_jdo_func_f)(hvml_jdo_t **args); // null-tereminated-args-list

struct hvml_jdo_closure_s {
    hvml_jdo_func_t           func;
    hvml_jdo_t               *args;
};

struct hvml_jdo_s {
    HVML_JDO_TYPE             jt;
    union {
        hvml_jo_t            *jo;
        hvml_jdo_cloure_t     closure;
    } u;
};

hvml_jdo_t* hvml_jdo_from_jo(hvml_jo_t *jo);
hvml_jdo_t* hvml_jdo_from_func(hvml_jdo_func_t func);
void        hvml_jdo_destroy(hvml_jdo_t *jdo);

char*       hvml_jdo_eval(hvml_jdo_t *jdo);

#ifdef __cplusplus
}
#endif

#endif // _hvml_jdo_h_

