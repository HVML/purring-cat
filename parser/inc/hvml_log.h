#ifndef _hvml_log_66655750_9759_49c0_bd59_07dc55eb09ce_
#define _hvml_log_66655750_9759_49c0_bd59_07dc55eb09ce_

#include <stdio.h>
#include <stdlib.h>

void hvml_log_set_thread_type(const char *type);

void hvml_log_printf(const char *cfile, int cline, const char *cfunc, FILE *out, const char level, const char *fmt, ...)
__attribute__ ((format (printf, 6, 7)));


#define D(fmt, ...) hvml_log_printf(__FILE__, __LINE__, __FUNCTION__, stderr, 'D', fmt, ##__VA_ARGS__)
#define I(fmt, ...) hvml_log_printf(__FILE__, __LINE__, __FUNCTION__, stderr, 'I', fmt, ##__VA_ARGS__)
#define W(fmt, ...) hvml_log_printf(__FILE__, __LINE__, __FUNCTION__, stderr, 'W', fmt, ##__VA_ARGS__)
#define E(fmt, ...) hvml_log_printf(__FILE__, __LINE__, __FUNCTION__, stderr, 'E', fmt, ##__VA_ARGS__)
#define V(fmt, ...) hvml_log_printf(__FILE__, __LINE__, __FUNCTION__, stderr, 'V', fmt, ##__VA_ARGS__)
#define A(statement, fmt, ...)                                                \
do {                                                                          \
  if (statement) break;                                                       \
  hvml_log_printf(__FILE__, __LINE__, __FUNCTION__, stderr, 'A',              \
                  "Assert failure:[%s];"fmt, #statement, ##__VA_ARGS__);      \
  abort();                                                                    \
} while (0)

#endif //_hvml_log_66655750_9759_49c0_bd59_07dc55eb09ce_

