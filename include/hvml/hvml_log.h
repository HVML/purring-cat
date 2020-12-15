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

#ifndef _hvml_log_h_
#define _hvml_log_h_

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// set typename of the calling thread, which would be printed in log funcs
void hvml_log_set_thread_type(const char *type);
// if set, no prefix/surfix part would be printed in log funcs
void hvml_log_set_output_only(int set);

void hvml_log_printf(const char *cfile, int cline, const char *cfunc, FILE *out, const char level, const char *fmt, ...)
__attribute__ ((format (printf, 6, 7)));


#define DX(_x3098, fmt, ...) hvml_log_printf(__FILE__, __LINE__, __func__, stderr, 'D', "%s" fmt "", "", ##__VA_ARGS__)
#define IX(_x3098, fmt, ...) hvml_log_printf(__FILE__, __LINE__, __func__, stderr, 'I', "%s" fmt "", "", ##__VA_ARGS__)
#define WX(_x3098, fmt, ...) hvml_log_printf(__FILE__, __LINE__, __func__, stderr, 'W', "%s" fmt "", "", ##__VA_ARGS__)
#define EX(_x3098, fmt, ...) hvml_log_printf(__FILE__, __LINE__, __func__, stderr, 'E', "%s" fmt "", "", ##__VA_ARGS__)
#define VX(_x3098, fmt, ...) hvml_log_printf(__FILE__, __LINE__, __func__, stderr, 'V', "%s" fmt "", "", ##__VA_ARGS__)
#define AX(_x3098, statement, fmt, ...)                                         \
do {                                                                            \
    if (statement) break;                                                       \
    hvml_log_printf(__FILE__, __LINE__, __func__, stderr, 'A',                  \
                    "Assert failure:[%s];" fmt "", #statement, ##__VA_ARGS__);  \
    abort();                                                                    \
} while (0)

// tweak: ISO C99 requires at least one argument for the "..." in a variadic macro
#define D(...) DX("", ##__VA_ARGS__)
#define I(...) IX("", ##__VA_ARGS__)
#define W(...) WX("", ##__VA_ARGS__)
#define E(...) EX("", ##__VA_ARGS__)
#define V(...) VX("", ##__VA_ARGS__)
#define A(...) AX("", ##__VA_ARGS__)


#ifdef __cplusplus
}
#endif

#endif //_hvml_log_h_

