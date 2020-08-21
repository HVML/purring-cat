#include "hvml_log.h"

#include <libgen.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>


static __thread char               thread_name[64] = {0};

void hvml_log_set_thread_type(const char *type) {
  snprintf(thread_name, sizeof(thread_name), "[%ld/%s]", syscall(__NR_gettid), type);
}

__attribute__ ((format (printf, 6, 7)))
void hvml_log_printf(const char *cfile, int cline, const char *cfunc, FILE *out, const char level, const char *fmt, ...) {
  if (thread_name[0]=='\0') hvml_log_set_thread_type("unknown");

  char   buf[4096];
  int    bytes = sizeof(buf);
  char  *p     = buf;
  int    n;

  struct timeval tv = {0};
  gettimeofday(&tv, NULL);
  struct tm tm = {0};
  localtime_r(&tv.tv_sec, &tm);

  do {
    n = snprintf(p, bytes, "%c %02d:%02d:%02d.%06ld@%s: ", level,
                           tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec, thread_name);

    bytes -= n;
    p     += n;
    if (bytes<=0) break;
    va_list arg;
    va_start(arg, fmt);
    n = vsnprintf(p, bytes, fmt, arg);
    va_end(arg);

  } while (0);

  fprintf(out, "%s =%s[%d]%s()=\n", buf, basename((char*)cfile), cline, cfunc);
}

