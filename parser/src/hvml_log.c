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

#ifdef _MSC_VER
  #include <Windows.h>
  #include <Shlwapi.h>
#else  
  #include <libgen.h>
  #include <pthread.h>
  #include <sys/syscall.h>
  #include <sys/time.h>
  #include <unistd.h>
#endif

#include <inttypes.h>
#include <stdarg.h>
#include <time.h>

#ifdef _MSC_VER

#pragma comment(lib, "shlwapi.lib")

static 
char * basename(char * path)
{
    char * p = PathFindFileNameA(path);
    return p == path ? NULL : p;
}

#endif // _MSC_VER


#ifdef __GNUC__
  static __thread char               thread_name[64] = {0};
#elif defined(_MSC_VER)
  __declspec(thread) static char thread_name[64] = {0};
#else
  #error Please look for an approach to declare tls variable in this compiler
#endif

static int                         output_only     = 0;

void hvml_log_set_thread_type(const char *type) {
    uint64_t tid = 0;
#ifdef __APPLE__
    pthread_threadid_np(NULL, &tid);
#endif
#ifdef __linux__
    tid = syscall(__NR_gettid);
#endif
#ifdef _MSC_VER
    tid = GetCurrentThreadId();
#endif
    snprintf(thread_name, sizeof(thread_name), "[%"PRId64"/%s]", tid, type);
}

// if set, no prefix/postfix info
void hvml_log_set_output_only(int set) {
    output_only = set;
}

#ifdef __GNUC__
__attribute__ ((format (printf, 6, 7)))
#endif
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
#ifdef _WIN32
            SYSTEMTIME localtime;
            GetLocalTime(&localtime);
            tm.tm_hour = localtime.wHour;
            tm.tm_min = localtime.wMinute;
            tm.tm_sec = localtime.wSecond;
            tv_usec = localtime.wMilliseconds * 1000L; 
#else            
            gettimeofday(&tv, NULL);
            localtime_r(&tv.tv_sec, &tm);
            tv_usec = tv.tv_usec;
#endif            
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

