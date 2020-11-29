#ifndef _hvml_string_h
#define _hvml_string_h

#include <inttypes.h>
#include <stddef.h>

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

int  hvml_string_printf(hvml_string_t *str, const char *fmt, ...)
__attribute__ ((format (printf, 2, 3)));
int  hvml_string_append_printf(hvml_string_t *str, const char *fmt, ...)
__attribute__ ((format (printf, 2, 3)));


int  hvml_string_to_number(const char *s, long double *v);

int  hvml_string_is_empty(hvml_string_t *str);

#ifdef __cplusplus
}
#endif

#endif // _hvml_string_h

