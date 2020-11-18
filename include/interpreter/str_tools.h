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

#ifndef _str_tools_h_
#define _str_tools_h_

#include "hvml/hvml_string.h"
#include <string.h>
#include <stddef.h>
#include <ctype.h>

#include <vector>
using namespace std;

typedef vector<hvml_string_t>  StringArray_t;

hvml_string_t create_string(const char* str, size_t len);
hvml_string_t create_trim_string(const char* str, size_t len);
void clear_StringArray(StringArray_t& sa);
size_t split_string(StringArray_t& sa,
                    hvml_string_t src_s,
                    hvml_string_t delimiter_s);

#endif //_str_tools_h_