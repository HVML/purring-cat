#ifndef _hvml_string_h
#define _hvml_string_h

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hvml_string_s           hvml_string_t;
struct hvml_string_s {
    char           *str;
    size_t          len;
};

// there is no `init`-like function here since hvml_string_t is public
// and self-descripted
void hvml_string_reset(hvml_string_t *str);
void hvml_string_clear(hvml_string_t *str);

int  hvml_string_push(hvml_string_t *str, const char c);
int  hvml_string_pop(hvml_string_t *str, const char *c);

int  hvml_string_get(hvml_string_t *str, char **buf, size_t *len);
int  hvml_string_set(hvml_string_t *str, const char *buf, size_t len);

int  hvml_string_printf(hvml_string_t *str, const char *fmt, ...)
__attribute__ ((format (printf, 2, 3)));
int  hvml_string_append_printf(hvml_string_t *str, const char *fmt, ...)
__attribute__ ((format (printf, 2, 3)));

#ifdef __cplusplus
}
#endif

#endif // _hvml_string_h
