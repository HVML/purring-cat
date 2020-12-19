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

const char *file_ext(const char *file) {
    const char *p = strrchr(file, '.');
    return p ? p : "";
}

#if 0
int strnicmp(const char *s1, const char *s2, size_t len) {
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
#endif // 0

const char *find_mustache(const char *s, size_t *ret_len) {
    const char *ret = strstr(s, "{{");
    if (ret) {
        const char *end = strstr(ret, "}}");
        if (end) {
            if (ret_len) {
                *ret_len = (end - ret + 2);
            }
            return ret;
        }
    }
    return NULL;
}

const char *get_mustache_inner(const char *s, size_t *ret_len) {
    const char *ret = strstr(s, "{{");
    if (ret) {
        ret += 2;
        while (*ret == ' ' || *ret == '\t' && *ret != '\0') ret++;
        if (*ret == '\0') return NULL;
        const char *end = strstr(ret, "}}");
        if (end) {
            while (*end == ' ' || *end == '\t') end--;
            if (ret_len) {
                *ret_len = (end - ret + 1);
            }
            return ret;
        }
    }
    return NULL;
}

char *str_trim(char *s) {
    char *p = s;
    while (*p == ' ' || *p == '\t' && *p != '\0') p++;
    if (*p == '\0') {
        *s = '\0';
        return s;
    }
    char *q = s;
    do {
        *q = *p;
        q++;
        p++;
    } while (*p);
    q --;
    while (*q == ' ' || *q == '\t') q--;
    *(q+1) ='\0';
    return s;
}

hvml_string_t replace_string(hvml_string_t replaced_s,
                             hvml_string_t after_replaced_s,
                             const char* orig_str)
{
    hvml_string_t ret_s = {NULL, 0};
    size_t orign_len = strlen(orig_str);
    size_t new_length = orign_len
                        + after_replaced_s.len
                        - replaced_s.len;
    if (new_length <= 0) return ret_s;

    hvml_string_set(&ret_s, orig_str, max (new_length, orign_len));
    char* p = strstr(ret_s.str, replaced_s.str);
    if (! p) return ret_s;

    const char* q = orig_str 
                    + (p - ret_s.str)
                    + replaced_s.len;
    memcpy ((void*)p, after_replaced_s.str, after_replaced_s.len);
    p += after_replaced_s.len;
    size_t tail_length = new_length - (p - ret_s.str);
    if (tail_length > 0) {
        memcpy (p, q, tail_length);
    }
    ret_s.str[new_length] = '\0';
    ret_s.len = new_length;
    return ret_s;
}