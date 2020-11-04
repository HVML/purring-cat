#ifndef _ext_tools_h_
#define _ext_tools_h_

#include <string.h>
#include <stddef.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C"
{
#endif

const char *file_ext(const char *file);
int strnicmp(const char *, const char *, size_t);

#ifdef __cplusplus
}
#endif

#endif //_ext_tools_h_