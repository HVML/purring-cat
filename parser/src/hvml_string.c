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


__attribute__ ((format (printf, 2, 3)))
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


__attribute__ ((format (printf, 2, 3)))
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
    if (bytes!=strlen(s)) return -1;

    *v = ldbl;

    return 0;
}

int  hvml_string_is_empty(hvml_string_t *str) {
    if (!str) return -1;
    return (0 == str->len);
}

