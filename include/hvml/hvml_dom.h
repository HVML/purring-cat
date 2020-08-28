#ifndef _hvml_dom_h_
#define _hvml_dom_h_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MKDT(type)  HVML_DOM_##type
#define MKDS(type) "HVML_DOM_"#type

typedef enum {
    MKDT(J_ROOT),
    MKDT(J_TAG),
    MKDT(J_ATTR),
        MKDT(J_ATTR_V),
    MKDT(J_TEXT)
} HVML_DOM_TYPE;

typedef struct hvml_dom_s          hvml_dom_t;
typedef struct hvml_dom_conf_s     hvml_dom_conf_t;

struct hvml_dom_conf_s {
};

hvml_dom_t* hvml_dom_create(hvml_dom_conf_t conf);
void        hvml_dom_destroy(hvml_dom_t *dom);

hvml_dom_t* hvml_dom_set_attr(hvml_dom_t *dom,
                              const char *key, size_t key_len,
                              const char *val, size_t val_len);
hvml_dom_t* hvml_dom_append_context(hvml_dom_t *dom, const char *txt, size_t len);
hvml_dom_t* hvml_dom_add_tag(hvml_dom_t *dom, const char *tag, size_t len);

hvml_dom_t* hvml_dom_root(hvml_dom_t *dom);
hvml_dom_t* hvml_dom_parent(hvml_dom_t *dom);
hvml_dom_t* hvml_dom_next(hvml_dom_t *dom);
hvml_dom_t* hvml_dom_prev(hvml_dom_t *dom);

hvml_dom_t* hvml_dom_detach(hvml_dom_t *dom);

hvml_dom_t* hvml_dom_select(hvml_dom_t *dom, const char *selector);

#ifdef __cplusplus
}
#endif

#endif // _hvml_dom_h_

