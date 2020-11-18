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

#include "interpreter/ext_tools.h"
#include "interpreter/str_tools.h"
#include <algorithm> // for_each

hvml_string_t create_string(const char* str, size_t len) {
    hvml_string_t ret_s = {NULL, 0};
    hvml_string_set(&ret_s, str, len);
    return ret_s;
}

hvml_string_t create_trim_string(const char* str, size_t len) {
    hvml_string_t ret_s = {NULL, 0};
    hvml_string_set(&ret_s, str, len);
    str_trim(ret_s.str);
    ret_s.len = strlen(ret_s.str);
    return ret_s;
}

void clear_StringArray(StringArray_t& sa) {
    for_each(sa.begin(),
             sa.end(),
             [](hvml_string_t& item)->void{
                 hvml_string_clear(&item);
             });
}

size_t split_string(StringArray_t& sa,
                    hvml_string_t src_s,
                    hvml_string_t delimiter_s)
{
    int n = src_s.len;
    char* s = src_s.str;
    sa.clear();
    if (delimiter_s.len <= 0) {
        sa.push_back(create_trim_string(s, n));
        return sa.size();
    }
    while (s < (src_s.str + n)) {
        char* p = strstr(s, delimiter_s.str);
        if (! p) {
            sa.push_back(create_trim_string(s, n));
            return sa.size();
        }
        sa.push_back(create_trim_string(s, (p-s)));
        s += delimiter_s.len;
    }
    return sa.size();
}