#include "interpreter/ext_tools.h"

const char *file_ext(const char *file)
{
    const char *p = strrchr(file, '.');
    return p ? p : "";
}

int strnicmp(const char *s1, const char *s2, size_t len)
{
    /* Yes, Virginia, it had better be unsigned */
    unsigned char c1, c2;
    c1 = c2 = 0;
    if (len) {
        do {
            c1 = *s1;
            c2 = *s2;
            s1++;
            s2++;
            if (!c1) break;
            if (!c2) break;
            if (c1 == c2) continue;

            c1 = tolower(c1);
            c2 = tolower(c2);
            if (c1 != c2) break;
        } while (--len);
    }

    return (int)c1 - (int)c2;
}