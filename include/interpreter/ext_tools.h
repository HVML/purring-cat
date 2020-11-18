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

#ifndef _ext_tools_h_
#define _ext_tools_h_

#include "hvml/hvml_string.h"
#include <string.h>
#include <stddef.h>
#include <ctype.h>

#ifndef __cplusplus
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif //__cplusplus

#ifdef __cplusplus
extern "C"
{
#endif

const char *file_ext(const char *file);
int strnicmp(const char *s1, const char *s2, size_t len);
const char *find_mustache(const char *s, size_t *ret_len);
const char *get_mustache_inner(const char *s, size_t *ret_len);
char *str_trim(char *s);
hvml_string_t replace_string(hvml_string_t replaced_s,
                             hvml_string_t after_replaced_s,
                             const char* orig_str);

#ifdef __cplusplus
}
#endif

#endif //_ext_tools_h_