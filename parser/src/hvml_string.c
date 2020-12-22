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

#include "hvml/hvml_string.h"

#include "hvml/hvml_log.h"

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>


void hvml_string_reset(hvml_string_t *str) {
    str->len = 0;
    if (!str->str) {
        A(str->len==0, "internal logic error");
        return;
    }
    str->str[0] = '\0';
}

void hvml_string_clear(hvml_string_t *str) {
    if (str->str) {
        free(str->str);
        str->str = NULL;
    }
    str->len = 0;
}

int hvml_string_push(hvml_string_t *str, const char c) {
    char *s = (char*)realloc(str->str, str->len + 2); // one extra null terminator
    if (!s) return -1;

    s[str->len] = c;
    str->str    = s;
    str->len   += 1;
    s[str->len] = '\0';

    return 0;
}

int hvml_string_pop(hvml_string_t *str, char *c) {
    if (!str->str || str->len<=0) return -1;

    if (c) *c = str->str[str->len-1];

    str->len -= 1;
    str->str[str->len] = '\0';

    return 0;
}

int hvml_string_append(hvml_string_t *str, const char *s) {
    size_t slen = strlen(s);
    size_t len  = str->len + slen;
    if (len<str->len) return -1;

    char *ss    = (char*)realloc(str->str, len+1);
    if (!ss) return -1;

    strcpy(ss+str->len, s);
    str->str    = ss;
    str->len    = len;

    return 0;
}

int hvml_string_get(hvml_string_t *str, char **buf, size_t *len) {
    if (buf) *buf = str->str;
    if (len) *len = str->len;

    return 0;
}

int hvml_string_set(hvml_string_t *str, const char *buf, size_t len) {
    char *s = (char*)realloc(str->str, len + 1); // one extra null terminator
    if (!s) return -1;

    memcpy(s, buf, len);
    s[len] = 0;
    str->str    = s;
    str->len    = len;

    return 0;
}


#ifdef __GNUC__
__attribute__ ((format (printf, 2, 3)))
#endif
int hvml_string_printf(hvml_string_t *str, const char *fmt, ...) {
    int n = 0;

    va_list arg;
    va_start(arg, fmt);
    n = vsnprintf(NULL, 0, fmt, arg);
    va_end(arg);

    if (n<0) return -1;

    char *s = (char*)realloc(str->str, n+1);
    if (!s) return -1;

    va_start(arg, fmt);
    vsnprintf(s, n+1, fmt, arg);
    va_end(arg);

    str->str = s;
    str->len = n;

    return str->len;
}

#ifdef __GNUC__
__attribute__ ((format (printf, 2, 3)))
#endif
int hvml_string_append_printf(hvml_string_t *str, const char *fmt, ...) {
    int n = 0;

    va_list arg;
    va_start(arg, fmt);
    n = vsnprintf(NULL, 0, fmt, arg);
    va_end(arg);

    if (n<0) return -1;

    char *s = (char*)realloc(str->str, str->len + n + 1);
    if (!s) return -1;

    va_start(arg, fmt);
    vsnprintf(s+str->len, n+1, fmt, arg);
    va_end(arg);

    str->str  = s;
    str->len += n + 1;

    return str->len;
}

int hvml_string_to_number(const char *s, long double *v) {
    if (!s) return -1;

    long double ldbl = 0.;
    int bytes = 0;
    int n = sscanf(s, "%Lf%n", &ldbl, &bytes);
    if (n!=1) return -1;
    if (bytes>=0 && (size_t)bytes!=strlen(s)) return -1;

    *v = ldbl;

    return 0;
}

int hvml_string_is_empty(hvml_string_t *str) {
    if (!str) return -1;
    return (0 == str->len);
}

int hvml_string_getline(const char *str, const char **end, const char **next) {
    if (!str) return -1;
    if (!end) return -1;
    if (!next) return -1;

    if (!*str) {
        *end  = str;
        *next = NULL;
        return 0;
    }

    const char *p = str;
    while (*p && *p!='\r' && *p!='\n') ++p;
    *end  = p;
    if (!*p) {
        *next = NULL;
    } else if (*p=='\n') {
        *next = p+1;
    } else {
        ++p;
        if (*p=='\n') {
            *next = p+1;
        } else {
            *next = p;
        }
    }

    return 0;
}

int hvml_string_to_dos(hvml_string_t *str) {
    if (!str) return -1;
    if (!str->str) return 0;

    hvml_string_t s = {0};
    char *start = str->str;
    char *p = start;
    int r = 0;
    while (1) {
        while (*p && *p!='\r' && *p!='\n') ++p;
        if (!*p) {
            if (!s.str) return 0;
            r = hvml_string_append(&s, start);
            if (r) break;
            hvml_string_clear(str);
            *str = s;
            return 0;
        }

        const char c = *p;
        *p = '\0';
        r = hvml_string_append(&s, start);
        *p = c;
        if (r) break;
        r = hvml_string_append(&s, "\r\n");
        if (r) break;

        if (*p=='\n') {
            ++p;
            start = p;
            continue;
        }

        ++p;
        if (*p=='\n') {
            ++p;
        }
        start = p;
    }

    hvml_string_clear(&s);
    return -1;
}

int hvml_string_to_unix(hvml_string_t *str) {
    if (!str) return -1;
    if (!str->str) return -1;

    hvml_string_t s = {0};
    char *start = str->str;
    char *p = start;
    int r = 0;
    while (1) {
        while (*p && *p!='\r') ++p;
        if (!*p) {
            if (!s.str) return 0;
            r = hvml_string_append(&s, start);
            if (r) break;
            hvml_string_clear(str);
            *str = s;
            return 0;
        }

        if (p[1]!='\n') {
            *p = '\n';
            ++p;
            continue;
        }

        *p = '\0';
        r = hvml_string_append(&s, start);
        *p = '\r';
        if (r) break;
        r = hvml_string_append(&s, "\n");
        if (r) break;

        ++p;
        start = p;
    }

    hvml_string_clear(&s);
    return -1;
}






static int hvml_string_append_vprintf(hvml_string_t *str, size_t total, const char *fmt, va_list ap) {
    A(str, "internal logic error");
    A(total>0, "internal logic error");

    char *s = (char*)realloc(str->str, str->len + total + 1);
    if (!s) return -1;

    int n = vsnprintf(s+str->len, total+1, fmt, ap);

    A(n>=0 && (size_t)n==total, "internal logic error");

    str->str  = s;
    str->len += total + 1;

    return str->len;
}

typedef int (*hvml_stream_vprintf_func)(void *target, size_t total, const char *fmt, va_list ap);
typedef int (*hvml_stream_destroy_func)(void *target);

struct hvml_stream_s {
    unsigned int                   sc:1;             // safe check
    void                          *arg;
    hvml_stream_vprintf_func       vprintf_func;
    hvml_stream_destroy_func       destroy_func;
};

static int call_vfprintf(void *arg, size_t total, const char *fmt, va_list ap) {
    A(arg, "internal logic error");
    A(total==0, "internal logic error");
    FILE *out = (FILE*)arg;
    return vfprintf(out, fmt, ap);
}

static int call_hvml_string_append_vprintf(void *arg, size_t total, const char *fmt, va_list ap) {
    A(arg, "internal logic error");
    A(total>0, "internal logic error");

    hvml_string_t *str = (hvml_string_t*)arg;

    return hvml_string_append_vprintf(str, total, fmt, ap);
}

static int call_fclose(void *arg) {
    A(arg, "internal logic error");
    FILE *out = (FILE*)arg;
    return fclose(out);
}

hvml_stream_t* hvml_stream_bind_file(FILE *out, int take_ownership) {
    A(out, "internal logic error");

    hvml_stream_t *stream = (hvml_stream_t*)calloc(1, sizeof(*stream));
    if (!stream) return NULL;

    stream->sc               = 0;
    stream->arg              = out;
    stream->vprintf_func     = call_vfprintf;
    if (!take_ownership) return stream;
    stream->destroy_func     = call_fclose;

    return stream;
}

hvml_stream_t* hvml_stream_bind_string(hvml_string_t *str) {
    A(str, "internal logic error");

    hvml_stream_t *stream = (hvml_stream_t*)calloc(1, sizeof(*stream));
    if (!stream) return NULL;

    stream->sc               = 1;
    stream->arg              = str;
    stream->vprintf_func     = call_hvml_string_append_vprintf;

    return stream;
}

void hvml_stream_destroy(hvml_stream_t *stream) {
    if (!stream) return;
    if (!stream->arg) return;

    if (stream->destroy_func) {
        stream->destroy_func(stream->arg);
    }
    stream->arg           = NULL;
    stream->vprintf_func  = NULL;
    stream->destroy_func  = NULL;

    free(stream);
}

int hvml_stream_printf(hvml_stream_t *stream, const char *fmt, ...) {
    A(stream, "internal logic error");
    A(stream->arg, "internal logic error");
    A(stream->vprintf_func, "internal logic error");

    int n = 0;

    if (stream->sc) {
        va_list arg;
        va_start(arg, fmt);
        n = vsnprintf(NULL, 0, fmt, arg);
        va_end(arg);

        if (n<=0) return n;
    }

    va_list arg;
    va_start(arg, fmt);
    int r = stream->vprintf_func(stream->arg, n, fmt, arg);
    va_end(arg);

    return r;
}

