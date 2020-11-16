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

const char *file_ext(const char *file)
{
    const char *p = strrchr(file, '.');
    return p ? p : "";
}

int strnicmp(const char *s1, const char *s2, size_t len)
{
    unsigned char c1, c2;
    c1 = c2 = 0;
    if (len) {
        do {
            c1 = *s1;
            c2 = *s2;
            s1++;
            s2++;
            if (!c1 || !c2) break;
            if (c1 == c2) continue;
            if (tolower(c1) != tolower(c2)) break;
        } while (--len);
    }

    return (int)c1 - (int)c2;
}

const char *find_mustache(const char *s, size_t *ret_len)
{
    const char *ret = strstr(s, "{{");
    if (ret) {
        const char *end = strstr(ret, "}}");
        if (end) {
            if (ret_len) {
                *ret_len = (end - ret);
            }
            return ret;
        }
    }
    return NULL;
}