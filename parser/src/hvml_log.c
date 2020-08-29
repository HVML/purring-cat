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

#include "hvml/hvml_log.h"

#include <inttypes.h>
#include <libgen.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static __thread char               thread_name[64] = {0};
static int                         output_only     = 0;

void hvml_log_set_thread_type(const char *type) {
    uint64_t tid = 0;
#ifdef __APPLE__
    pthread_threadid_np(NULL, &tid);
#endif
#ifdef __linux__
    tid = syscall(__NR_gettid);
#endif
    snprintf(thread_name, sizeof(thread_name), "[%"PRId64"/%s]", tid, type);
}

// if set, no prefix/postfix info
void hvml_log_set_output_only(int set) {
    output_only = set;
}

__attribute__ ((format (printf, 6, 7)))
void hvml_log_printf(const char *cfile, int cline, const char *cfunc, FILE *out, const char level, const char *fmt, ...) {
    if (thread_name[0]=='\0') hvml_log_set_thread_type("unknown");

    char   buf[4096];
    int    bytes = sizeof(buf);
    char  *p     = buf;
    int    n;

    struct timeval tv      = {0};
    struct tm      tm      = {0};
    long           tv_usec = 0;

    int output_only_set    = output_only;

    do {
        if (!output_only_set) {
            gettimeofday(&tv, NULL);
            localtime_r(&tv.tv_sec, &tm);
            tv_usec = tv.tv_usec;
            n = snprintf(p, bytes, "%c %02d:%02d:%02d.%06ld@%s: ", level,
                    tm.tm_hour, tm.tm_min, tm.tm_sec, tv_usec, thread_name);

            bytes -= n;
            p     += n;
            if (bytes<=0) break;
        }

        va_list arg;
        va_start(arg, fmt);
        n = vsnprintf(p, bytes, fmt, arg);
        va_end(arg);

    } while (0);

    if (!output_only_set) {
        fprintf(out, "%s =%s[%d]%s()=\n", buf, basename((char*)cfile), cline, cfunc);
    } else {
        fprintf(out, "%s\n", buf);
    }
}

