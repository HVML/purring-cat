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

#ifndef _hvml_string_h
#define _hvml_string_h

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hvml_string_s           hvml_string_t;
struct hvml_string_s {
    char           *str;
    size_t          len;
};

#define hvml_string_str(s) ((s)->str)
#define hvml_string_len(s) ((s)->len)

// there is no `init`-like function here since hvml_string_t is public
// and self-descripted
void hvml_string_reset(hvml_string_t *str);
void hvml_string_clear(hvml_string_t *str);

int  hvml_string_push(hvml_string_t *str, const char c);
int  hvml_string_pop(hvml_string_t *str, char *c);

int  hvml_string_append(hvml_string_t *str, const char *s);

int  hvml_string_get(hvml_string_t *str, char **buf, size_t *len);
int  hvml_string_set(hvml_string_t *str, const char *buf, size_t len);

#ifdef __GNUC__
__attribute__ ((format (printf, 2, 3)))
#endif
int  hvml_string_printf(hvml_string_t *str, const char *fmt, ...);

#ifdef __GNUC__
__attribute__ ((format (printf, 2, 3)))
#endif
int  hvml_string_append_printf(hvml_string_t *str, const char *fmt, ...);

int  hvml_string_to_number(const char *s, long double *v);

int  hvml_string_is_empty(hvml_string_t *str);


typedef struct hvml_stream_s            hvml_stream_t;

hvml_stream_t* hvml_stream_bind_file(FILE *out, int take_ownership);
hvml_stream_t* hvml_stream_bind_string(hvml_string_t *str);
void           hvml_stream_destroy(hvml_stream_t *stream);

int hvml_stream_printf(hvml_stream_t *stream, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // _hvml_string_h

