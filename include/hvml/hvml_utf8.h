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

#ifndef _hvml_utf8_h_
#define _hvml_utf8_h_

#include <inttypes.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hvml_utf8_decoder_s         hvml_utf8_decoder_t;
typedef struct hvml_utf8_encoder_s         hvml_utf8_encoder_t;

hvml_utf8_decoder_t* hvml_utf8_decoder();
void                 hvml_utf8_decoder_destroy(hvml_utf8_decoder_t *decoder);

// 0:  ok, but not complete
// 1:  ok, and complete
// -1: error
int                  hvml_utf8_decoder_push(hvml_utf8_decoder_t *decoder, const char c, uint64_t *cp);
// 1:  there's not cached utf8 fragment
// 0:  there's cached utf8 fragment or internal failure
int                  hvml_utf8_decoder_ready(hvml_utf8_decoder_t *decoder);

const char*          hvml_utf8_decoder_cache(hvml_utf8_decoder_t *decoder, size_t *len);


int                  hvml_utf8_encode(const uint64_t cp, char *output, size_t *output_len);




#ifdef __cplusplus
}
#endif

#endif // _hvml_utf8_h_

