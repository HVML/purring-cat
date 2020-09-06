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

#include "hvml/hvml_utf8.h"

#include "hvml/hvml_string.h"

#include <stdlib.h>

#define MKDT(type)  DECODER##type
#define MKDS(type) "DECODER_"#type

typedef enum {
    MKDT(D_INIT),
    MKDT(D_22),
    MKDT(D_32),
    MKDT(D_33),
    MKDT(D_42),
    MKDT(D_43),
    MKDT(D_44)
} DECODER_STATE;

struct hvml_utf8_decoder_s {
    DECODER_STATE             state;
    uint64_t                  cp;
    hvml_string_t             cache;
};


hvml_utf8_decoder_t* hvml_utf8_decoder() {
    hvml_utf8_decoder_t *decoder = (hvml_utf8_decoder_t*)calloc(1, sizeof(*decoder));
    if (!decoder) return NULL;

    return decoder;
}

void hvml_utf8_decoder_destroy(hvml_utf8_decoder_t *decoder) {
    if (!decoder) return;

    hvml_string_clear(&decoder->cache);

    free(decoder);
}

#define do_output()                                    \
do {                                                   \
    if (cp) {                                          \
        *cp = decoder->cp;                             \
    }                                                  \
    decoder->state = MKDT(D_INIT);                     \
    decoder->cp = 0;                                   \
} while (0)

int hvml_utf8_decoder_push(hvml_utf8_decoder_t *decoder, const char c, uint64_t *cp)
{
    if (!decoder) return -1;
    const unsigned char uc = (const unsigned char)c;
    switch (decoder->state) {
        case MKDT(D_INIT):
        {
            hvml_string_reset(&decoder->cache);
            if ((uc&0x80)==0) {
                if (hvml_string_push(&decoder->cache, c)) {
                   return -1;
                }
                decoder->cp = uc;
                do_output();
                return 1;
            }
            if ((uc&0xe0)==0xc0) {
                if (hvml_string_push(&decoder->cache, c)) {
                   return -1;
                }
                decoder->cp = (uc & ~0xe0) << 6;
                decoder->state = MKDT(D_22);
                break;
            }
            if ((uc&0xf0)==0xe0) {
                if (hvml_string_push(&decoder->cache, c)) {
                   return -1;
                }
                decoder->cp = (uc & ~0xf0) << 12;
                decoder->state = MKDT(D_32);
                break;
            }
            if ((uc&0xf4)==0xf0) {
                if (hvml_string_push(&decoder->cache, c)) {
                   return -1;
                }
                decoder->cp = (uc & ~0xf4) << 18;
                decoder->state = MKDT(D_42);
                break;
            }
            return -1;
        } break;
        case MKDT(D_22):
        {
            if ((uc&0xc0)==0x80) {
                if (hvml_string_push(&decoder->cache, c)) {
                    return -1;
                }
                decoder->cp |= (uc & ~0xc0);
                do_output();
                return 1;
            }
            return -1;
        } break;
        case MKDT(D_32):
        {
            if ((uc&0xc0)==0x80) {
                if (hvml_string_push(&decoder->cache, c)) {
                    return -1;
                }
                decoder->cp |= (uc & ~0xc0) << 6;
                decoder->state = MKDT(D_33);
                break;
            }
            return -1;
        } break;
        case MKDT(D_33):
        {
            if ((uc&0xc0)==0x80) {
                if (hvml_string_push(&decoder->cache, c)) {
                    return -1;
                }
                decoder->cp |= (uc & ~0xc0);
                do_output();
                return 1;
            }
            return -1;
        } break;
        case MKDT(D_42):
        {
            if ((uc&0xc0)==0x80) {
                if (hvml_string_push(&decoder->cache, c)) {
                    return -1;
                }
                decoder->cp |= (uc & ~0xc0) << 12;
                decoder->state = MKDT(D_43);
                break;
            }
            return -1;
        } break;
        case MKDT(D_43):
        {
            if ((uc&0xc0)==0x80) {
                if (hvml_string_push(&decoder->cache, c)) {
                    return -1;
                }
                decoder->cp |= (uc & ~0xc0) << 6;
                decoder->state = MKDT(D_44);
                break;
            }
            return -1;
        } break;
        case MKDT(D_44):
        {
            if ((uc&0xc0)==0x80) {
                if (hvml_string_push(&decoder->cache, c)) {
                    return -1;
                }
                decoder->cp |= (uc & ~0xc0);
                do_output();
                return 1;
            }
            return -1;
        } break;
        default:
        {
            return -1;
        } break;
    }

    return 0;
}

int hvml_utf8_decoder_ready(hvml_utf8_decoder_t *decoder) {
    if (!decoder) return 0;
    if (decoder->state != MKDT(D_INIT)) return 0;

    return 1;
}

const char* hvml_utf8_decoder_cache(hvml_utf8_decoder_t *decoder, size_t *len) {
    if (!decoder) return NULL;

    if (len) *len = decoder->cache.len;

    return decoder->cache.str;
}

int hvml_utf8_encode(const uint64_t cp, char *output, size_t *output_len)
{
    if (output && !output_len) return -1;

    if (cp < 0x80) {
        if (output_len) {
            if (*output_len < 1) {
                *output_len = 1;
                return 0;
            }
            *output_len = 1;
        }
        if (output) {
            output[0] = cp;
        }
        return 0;
    }
    if (cp < 0x0800) {
        if (output_len) {
            if (*output_len < 2) {
                *output_len = 2;
                return 0;
            }
            *output_len = 2;
        }
        if (output) {
            output[0] = 0xc0 | ((cp>>6) & ~0xe0);
            output[1] = 0x80 | (cp & ~0xc0);
        }
        return 0;
    }
    if (cp < 0x10000) {
        if (output_len) {
            if (*output_len < 3) {
                *output_len = 3;
                return 0;
            }
            *output_len = 3;
        }
        if (output) {
            output[0] = 0xe0 | ((cp>>12) & ~0xf0);
            output[1] = 0x80 | ((cp>>6) & ~0xc0);
            output[2] = 0x80 | (cp & ~0xc0);
        }
        return 0;
    }
    if (cp < 0x110000) {
        if (output_len) {
            if (*output_len < 4) {
                *output_len = 4;
                return 0;
            }
            *output_len = 4;
        }
        if (output) {
            output[0] = 0xf0 | ((cp>>18) & ~0xf4);
            output[1] = 0x80 | ((cp>>12) & ~0xc0);
            output[2] = 0x80 | ((cp>>6) & ~0xc0);
            output[3] = 0x80 | (cp & ~0xc0);
        }
        return 0;
    }
    return -1;
}

