// This file is a part of Purring Cat, a reference implementation of HVML.
//
// Copyright (C) 2020, <liuxinouc@126.com>.
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

#include "interpreter/adverb_property.h"
#include <string.h>

static char *adverb_property_flag[] = {
    "synchronously",
    "ascendingly",
    "descendingly",
    "asynchronously",
    "exclusively",
    "uniquely",
};

static char *adverb_abbreviation_flag[] = {
    "sync",
    "asc",
    "desc",
    "async",
    "excl",
    "uniq",
};

#define ADVERB_STRING_LEN_MAX 15
#define ADVERB_ABBREV_LEN_MAX 6
#define ADVERB_PROPERTY_COUNT (sizeof(adverb_property_flag) / sizeof(adverb_property_flag[0]))

ADVERB_PROPERTY get_adverb_type(const char *str)
{
    int i;
    for (i = 0; i < ADVERB_PROPERTY_COUNT; i ++) {
        if (0 == strncmp(adverb_abbreviation_flag[i], str, ADVERB_STRING_LEN_MAX)) {
            return i;
        }

        if (0 == strncmp(adverb_property_flag[i], str, ADVERB_ABBREV_LEN_MAX)) {
            return i;
        }
    }

    return adv_UNKNOWN;
}

const char* adverb_to_string(ADVERB_PROPERTY type)
{
    if (type < 0 || type >= adv_UNKNOWN) return NULL;
    return adverb_property_flag[type];
}

const char* adverb_to_abbreviation(ADVERB_PROPERTY type)
{
    if (type < 0 || type >= adv_UNKNOWN) return NULL;
    return adverb_abbreviation_flag[type];
}
